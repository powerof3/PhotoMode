#include "Screenshots/Manager.h"

#include "Graphics.h"
#include "PhotoMode/Manager.h"
#include "Settings.h"

namespace Screenshot
{
	Paths::Paths(std::uint32_t index) :
		screenshot(std::format("{}/Screenshot{}.dds", screenshotFolder, index)),
		painting(std::format("{}/Screenshot{}.dds", paintingFolder, index))
	{}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		takeScreenshotAsPNG = a_ini.GetBoolValue("Screenshots", "bCustomPhotoFolder", takeScreenshotAsPNG);

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
			photoPath = *directory;
		}

		if (!std::filesystem::exists(photoPath)) {
			std::filesystem::create_directory(photoPath);
		}

		logger::info("\tScreenshot directory : {}", photoPath.string());
		logger::info("\tScreenshot textures : {}", screenshotFolder);
		logger::info("\tPainting textures : {}", paintingFolder);

		constexpr auto get_textures = [](std::string_view a_folder, std::vector<std::string>& a_textures) {
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

						a_textures.push_back(Texture::Sanitize(pathStr));
					}
				}
			}

			for (auto& badTexture : badTextures) {
				logger::info("\tDeleting invalid texture ({})", badTexture);
				std::filesystem::remove(badTexture);
			}
		};

		get_textures(screenshotFolder, screenshots);
		get_textures(paintingFolder, paintings);

		Settings::GetSingleton()->SerializeMCM([this](auto& ini) {
			index = ini.GetLongValue("Screenshots", "iScreenshotIndex", this->index);
			if (index == -1) {
				index = RE::GetINISetting("iScreenShotIndex:Display")->GetSInt();
			}
			ini.SetLongValue("Screenshots", "iScreenshotIndex", this->index);
		});

		logger::info("\t{} screenshots", screenshots.size());
		logger::info("\t{} paintings", paintings.size());
		logger::info("\tscreenshot index : {}", index);
	}

	void Manager::AddScreenshotPaths(Paths& a_paths)
	{
		screenshots.push_back(Texture::Sanitize(a_paths.screenshot));
		paintings.push_back(Texture::Sanitize(a_paths.painting));
	}

	std::uint32_t Manager::GetIndex() const
	{
		return index;
	}

	void Manager::IncrementIndex()
	{
		index++;
		Settings::GetSingleton()->SerializeMCM([this](auto& ini) {
			ini.SetLongValue("Screenshots", "iScreenshotIndex", this->index);
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

	bool Manager::TakeScreenshot(const char* a_fallbackPath)
	{
		bool skipVanillaScreenshot = false;

		const auto renderer = RE::BSGraphics::Renderer::GetRendererData();
		if (!renderer) {
			return skipVanillaScreenshot;
		}

		// capture screenshot
		DirectX::ScratchImage inputImage{};

		const ComPtr<ID3D11Device>        device{ renderer->forwarder };
		const ComPtr<ID3D11DeviceContext> deviceContext{ renderer->context };
		ID3D11Texture2D*                  texture2D{ renderer->renderTargets[RE::RENDER_TARGET::kSCREENSHOT].texture };

		if (auto result = DirectX::CaptureTexture(device.Get(), deviceContext.Get(), texture2D, inputImage); result == S_OK) {
			std::string pngPath = std::format("{}\\Screenshot{}.png", std::filesystem::exists(photoPath) ? photoPath.string() : a_fallbackPath, GetIndex());

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

				if (takeScreenshotAsPNG) {
					skipVanillaScreenshot = true;
					Texture::SaveToPNG(blendedImage, pngPath);
				}

				overlayImage.Release();
				blendedImage.Release();
			} else {
				TakeScreenshotAsTexture(inputImage, inputImage);

				if (takeScreenshotAsPNG) {
					skipVanillaScreenshot = true;
					Texture::SaveToPNG(inputImage, pngPath);
				}
			}

			if (takeScreenshotAsPNG) {
				RE::DebugNotification("$PM_ScreenshotNotif"_T);
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

		Paths      ssPaths(GetIndex());
		const auto renderer = RE::BSGraphics::Renderer::GetSingleton();

		// regular
		if (compressTextures) {
			DirectX::ScratchImage outputImage;

			Texture::CompressTexture(renderer, a_ssImage, outputImage);
			Texture::SaveToDDS(outputImage, ssPaths.screenshot);

			outputImage.Release();
		} else {
			Texture::SaveToDDS(a_ssImage, ssPaths.screenshot);
		}

		// painting
		if (applyPaintFilter) {
			DirectX::ScratchImage outputImage;
			Texture::OilPaintingFilter(a_paintingImage.GetImages(), paintFilter.radius, paintFilter.intensity, outputImage);

			if (compressTextures) {
				DirectX::ScratchImage compressedImage;
				Texture::CompressTexture(renderer, outputImage, compressedImage);  // NOLINT(readability-suspicious-call-argument)
				Texture::SaveToDDS(compressedImage, ssPaths.painting);
				compressedImage.Release();
			} else {
				Texture::SaveToDDS(outputImage, ssPaths.painting);
			}

			outputImage.Release();
		}

		AddScreenshotPaths(ssPaths);
	}

	std::string Manager::GetRandomScreenshot()
	{
		if (screenshots.empty()) {
			return {};
		}

		return screenshots[RNG().generate<std::size_t>(0, screenshots.size() - 1)];
	}

	std::string Manager::GetRandomPainting()
	{
		// fallback to screenshots
		if (paintings.empty() || !MANAGER(Screenshot)->CanApplyPaintFilter()) {
			return GetRandomScreenshot();
		}

		return paintings[RNG().generate<std::size_t>(0, paintings.size() - 1)];
	}
}
