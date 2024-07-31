#include "Screenshots/Manager.h"

#include "Graphics.h"
#include "PhotoMode/Manager.h"
#include "Settings.h"

namespace Screenshot
{
	Image::Image(std::string& a_path, std::uint32_t a_index) :
		path(Texture::Sanitize(a_path)),
		index(a_index)
	{}

	Image::Image(std::string& a_path) :
		path(Texture::Sanitize(a_path))
	{
		srell::smatch       matches;
		static srell::regex screenshotPattern{ R"(screenshot(\d+))" };
		if (srell::regex_search(path, matches, screenshotPattern)) {
			if (matches.size() > 1) {
				index = string::to_num<std::int32_t>(matches[1].str());
			}
		}
	}

	LoadScreenImage::LoadScreenImage(std::uint32_t a_index) :
		index(a_index),
		screenshot(std::format("{}/Screenshot{}.dds", screenshotFolder, a_index)),
		painting(std::format("{}/Screenshot{}.dds", paintingFolder, a_index))
	{}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		useCustomFolderDirectory = a_ini.GetBoolValue("Screenshots", "bCustomPhotoFolder", useCustomFolderDirectory);
		autoHideMenus = a_ini.GetBoolValue("Screenshots", "bAutoHideMenus", autoHideMenus);
		allowMultiScreenshots = a_ini.GetBoolValue("Screenshots", "bMultiScreenshots", allowMultiScreenshots);
		takeScreenshotAsDDS = a_ini.GetBoolValue("Screenshots", "bLoadScreenPics", takeScreenshotAsDDS);

		applyPaintFilter = a_ini.GetBoolValue("Screenshots", "bPaintFilter", applyPaintFilter);
		paintFilter.intensity = a_ini.GetDoubleValue("Screenshots", "fPaintIntensity", paintFilter.intensity);
		paintFilter.radius = a_ini.GetLongValue("Screenshots", "iPaintRadius", paintFilter.radius);

		compressTextures = a_ini.GetBoolValue("Screenshots", "bCompressTextures", compressTextures);
	}

	void Manager::LoadScreenshots()
	{
		logger::info("Loading screenshots...");

		if (auto directory = logger::log_directory()) {
			directory->remove_filename();
			*directory /= "Photos";
			photoDirectory = *directory;
		}

		if (!std::filesystem::exists(photoDirectory)) {
			std::filesystem::create_directory(photoDirectory);
		}

		logger::info("\tScreenshot directory : {}", photoDirectory.string());
		logger::info("\tScreenshot textures : {}", screenshotFolder);
		logger::info("\tPainting textures : {}", paintingFolder);

		constexpr auto get_textures = [](std::string_view a_folder, std::vector<Image>& a_textures) {
			const std::filesystem::directory_entry folder{ a_folder };
			if (!folder.exists()) {
				std::filesystem::create_directory(a_folder);
				return;
			}

			std::vector<std::string> badTextures{};

			for (const auto& entry : std::filesystem::directory_iterator(a_folder)) {
				if (entry.is_regular_file()) {
					if (const auto& path = entry.path(); path.extension() == ".dds") {
						auto pathStr = entry.path().string();

						DirectX::TexMetadata info;
						GetMetadataFromDDSFile(stl::utf8_to_utf16(pathStr)->c_str(), DirectX::DDS_FLAGS_NONE, info);

						if (info.width % 4 != 0 || info.height % 4 != 0) {
							badTextures.push_back(pathStr);
							continue;
						}

						a_textures.push_back(pathStr);
					}
				}
			}

			std::sort(a_textures.begin(), a_textures.end());

			for (auto& badTexture : badTextures) {
				logger::info("\tDeleting invalid texture ({})", badTexture);
				std::filesystem::remove(badTexture);
			}
		};

		get_textures(screenshotFolder, screenshots);
		get_textures(paintingFolder, paintings);

		Settings::GetSingleton()->SerializeMCM([this](auto& ini) {
			index = ini.GetLongValue("Screenshots", "iScreenshotIndex", index);
			AssignHighestPossibleIndex();
			ini.SetLongValue("Screenshots", "iScreenshotIndex", index);
		});

		logger::info("\t{} screenshots", screenshots.size());
		logger::info("\t{} paintings", paintings.size());
		logger::info("\tscreenshot index : {}", index);
	}


	void Manager::AddLoadScreenImages(LoadScreenImage& a_images)
	{
		screenshots.emplace_back(a_images.screenshot, a_images.index);
		paintings.emplace_back(a_images.painting, a_images.index);
	}

	std::uint32_t Manager::GetIndex() const
	{
		return index;
	}

	void Manager::AssignHighestPossibleIndex()
	{
		constexpr auto extract_texture_index = [](const std::vector<Image>& a_files) {
			if (a_files.empty()) {
				return -1;
			}
			return a_files.back().index + 1;
		};

		const auto get_photos_index = [this]() {
			std::vector<Image> photos{};
			for (const auto& entry : std::filesystem::directory_iterator(photoDirectory)) {
				if (entry.is_regular_file()) {
					if (const auto& path = entry.path(); path.extension() == ".png") {
						auto pathStr = entry.path().string();
						photos.push_back(pathStr);
					}
				}
			}
			std::sort(photos.begin(), photos.end());
			return extract_texture_index(photos);
		};

		auto mcmIndex = index;
		auto photosIndex = get_photos_index();
		auto vanillaScreenshotIndex = RE::GetINISetting("iScreenShotIndex:Display")->GetSInt();
		auto screenshotsIndex = extract_texture_index(screenshots);
		auto paintingsIndex = extract_texture_index(paintings);

		logger::info("\tAssigning highest screenshot index...");
		logger::info("\t\tmcm index: {}", mcmIndex);
		logger::info("\t\tphoto directory index: {}", photosIndex);
		logger::info("\t\tvanilla directory index: {}", vanillaScreenshotIndex);
		logger::info("\t\tscreenshot textures index: {}", screenshotsIndex);
		logger::info("\t\tpainting textures index: {}", paintingsIndex);

		std::set<std::int32_t> indices;
		indices.insert({ mcmIndex, photosIndex, vanillaScreenshotIndex, screenshotsIndex, paintingsIndex });

		index = *indices.rbegin();
	}

	void Manager::IncrementIndex()
	{
		index++;
		Settings::GetSingleton()->SerializeMCM([this](auto& ini) {
			ini.SetLongValue("Screenshots", "iScreenshotIndex", index);
		});
	}

	bool Manager::AllowMultiScreenshots() const
	{
		return allowMultiScreenshots;
	}

	bool Manager::CanAutoHideMenus() const
	{
		return autoHideMenus;
	}

	bool Manager::CanApplyPaintFilter() const
	{
		return applyPaintFilter;
	}

	bool Manager::CanDisplayScreenshotInLoadScreen() const
	{
		return takeScreenshotAsDDS && (!screenshots.empty() || !paintings.empty());
	}

	bool Manager::TakeScreenshot()
	{
		bool skipVanillaScreenshot = false;

		const auto renderer = RE::BSGraphics::Renderer::GetRendererData();
		if (!renderer) {
			return skipVanillaScreenshot;
		}

		// capture screenshot
		DirectX::ScratchImage inputImage{};

		const ComPtr<ID3D11Device>        device{ (ID3D11Device*)renderer->forwarder };
		const ComPtr<ID3D11DeviceContext> deviceContext{ (ID3D11DeviceContext*)renderer->context };
		ID3D11Texture2D*                  texture2D{ renderer->renderTargets[RE::RENDER_TARGET::kSCREENSHOT].texture };

		if (auto result = DirectX::CaptureTexture(device.Get(), deviceContext.Get(), texture2D, inputImage); result == S_OK) {
			skipVanillaScreenshot = true;

			auto        vanillaScreenshotIndex = RE::GetINISetting("iScreenShotIndex:Display");
			std::string pngPath = useCustomFolderDirectory ? std::format("{}\\Screenshot{}.png", photoDirectory.string(), GetIndex()) : std::format("{}{}.png", RE::GetINISetting("sScreenShotBaseName:Display")->GetString(), vanillaScreenshotIndex->GetSInt());

			// apply overlay
			if (const auto [overlay, alpha] = MANAGER(PhotoMode)->GetOverlay(); overlay) {
				DirectX::ScratchImage overlayImage;
				DirectX::ScratchImage blendedImage;

				// Convert PNG B8G8R8 format to R8G8B8
				DirectX::Convert(overlay->image->GetImages(), 1,
					overlay->image->GetMetadata(),
					inputImage.GetMetadata().format, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT,
					overlayImage);

				Texture::AlphaBlendImage(inputImage.GetImages(), overlayImage.GetImages(), blendedImage, alpha);

				TakeScreenshotAsTexture(blendedImage, inputImage);
				Texture::SaveToPNG(blendedImage, pngPath);

				overlayImage.Release();
				blendedImage.Release();
			} else {
				TakeScreenshotAsTexture(inputImage, inputImage);
				Texture::SaveToPNG(inputImage, pngPath);
			}

			if (!useCustomFolderDirectory) {
				++vanillaScreenshotIndex->data.i;
			}
			IncrementIndex();
		}

		inputImage.Release();

		return skipVanillaScreenshot;
	}

	void Manager::TakeScreenshotAsTexture(const DirectX::ScratchImage& a_ssImage, const DirectX::ScratchImage& a_paintingImage)
	{
		if (!takeScreenshotAsDDS || a_ssImage.GetMetadata().width % 4 != 0 || a_ssImage.GetMetadata().height % 4 != 0) {
			return;
		}

		LoadScreenImage      ssImages(GetIndex());
		const auto renderer = RE::BSGraphics::Renderer::GetSingleton();

		// regular
		if (compressTextures) {
			DirectX::ScratchImage outputImage;

			Texture::CompressTexture(renderer, a_ssImage, outputImage);
			Texture::SaveToDDS(outputImage, ssImages.screenshot);

			outputImage.Release();
		} else {
			Texture::SaveToDDS(a_ssImage, ssImages.screenshot);
		}

		// painting
		if (applyPaintFilter) {
			DirectX::ScratchImage outputImage;
			Texture::OilPaintingFilter(a_paintingImage.GetImages(), paintFilter.radius, paintFilter.intensity, outputImage);

			if (compressTextures) {
				DirectX::ScratchImage compressedImage;
				Texture::CompressTexture(renderer, outputImage, compressedImage);  // NOLINT(readability-suspicious-call-argument)
				Texture::SaveToDDS(compressedImage, ssImages.painting);
				compressedImage.Release();
			} else {
				Texture::SaveToDDS(outputImage, ssImages.painting);
			}

			outputImage.Release();
		}

		AddLoadScreenImages(ssImages);
	}

	std::string Manager::GetRandomScreenshot()
	{
		if (screenshots.empty()) {
			return {};
		}

		return screenshots[RNG().generate<std::size_t>(0, screenshots.size() - 1)].path;
	}

	std::string Manager::GetRandomPainting()
	{
		// fallback to screenshots
		if (paintings.empty() || !MANAGER(Screenshot)->CanApplyPaintFilter()) {
			return GetRandomScreenshot();
		}

		return paintings[RNG().generate<std::size_t>(0, paintings.size() - 1)].path;
	}
}
