#include "Screenshots/Manager.h"

#include "Graphics.h"
#include "PhotoMode/Manager.h"
#include "Settings.h"
#include "Shared.h"

namespace Screenshot
{
	Image::Image(std::string_view a_path, std::uint32_t a_index) :
		path(std::format("{}/Screenshot_{}.dds", a_path, a_index)),
		index(a_index)
	{}

	Image::Image(std::string& a_path) :
		path(Texture::Sanitize(a_path)),
		index(Shared::GetScreenshotIndex(a_path))
	{}

	void Collection::AddImage(Image a_image)
	{
		images.emplace_back(std::move(a_image));

		RebuildValidImages();
	}

	void Collection::LoadImages(std::string_view a_folder)
	{
		if (auto result = Shared::GetOrCreateDirectory(a_folder); !result) {
			logger::error("Failed to create {} folder: {}", a_folder, result.error().message());
			return;
		}

		ProcessImages(a_folder);

		std::sort(images.begin(), images.end());
	}

	// screenshot69.dds -> Screenshot_69.dds
	void Collection::ProcessImages(std::string_view a_folder)
	{
		struct Screenshot
		{
			std::filesystem::path path;
			std::filesystem::path renameTo;
			bool                  bad{ false };
		};

		std::vector<Screenshot> screenshots;
		Shared::ForEachFile(a_folder, ".dds"sv, [&](const std::filesystem::path& a_path) {
			screenshots.emplace_back(Screenshot{ .path = a_path });
		});

		if (screenshots.empty()) {
			return;
		}

		static const boost::regex oldPattern{ R"(^screenshot_?(\d+)\.dds$)", boost::regex::icase };

		std::for_each(std::execution::par, screenshots.begin(), screenshots.end(), [](Screenshot& a_screenshot) {
			a_screenshot.bad = true;

			DirectX::TexMetadata info;
			const auto           widePath = stl::utf8_to_utf16(a_screenshot.path.string());
			const auto           hr = GetMetadataFromDDSFile(widePath->c_str(), DirectX::DDS_FLAGS_NONE, info);

			a_screenshot.bad = FAILED(hr) || info.width % 4 != 0 || info.height % 4 != 0;
			if (a_screenshot.bad) {
				return;
			}

			const auto    fileName = a_screenshot.path.filename().string();
			boost::smatch matches;
			if (boost::regex_match(fileName, matches, oldPattern)) {
				const auto newName = std::format("Screenshot_{}.dds", string::to_num<std::int32_t>(matches[1].str()));
				if (fileName != newName) {
					a_screenshot.renameTo = a_screenshot.path.parent_path() / newName;
				}
			}
		});

		for (auto& [path, renameTo, bad] : screenshots) {
			std::error_code ec;

			if (bad) {
				logger::info("\tDeleting invalid texture ({})", path.string());
				std::filesystem::remove(path, ec);
				if (ec) {
					logger::warn("\t\tFailed to delete {} ({})", path.string(), ec.message());
				}
				continue;
			}

			if (!renameTo.empty()) {
				if (std::filesystem::exists(renameTo, ec) && !std::filesystem::equivalent(path, renameTo, ec)) {
					logger::warn("\tSkipped renaming {} -> {} (already exists)", path.filename().string(), renameTo.filename().string());
				} else {
					std::filesystem::rename(path, renameTo, ec);
					if (ec) {
						logger::warn("\tFailed to rename {} -> {} ({})", path.filename().string(), renameTo.filename().string(), ec.message());
					} else {
						logger::info("\tRenamed texture {} -> {}", path.filename().string(), renameTo.filename().string());
						path = renameTo;
					}
				}
			}

			auto finalPath = path.string();
			images.push_back(finalPath);
		}
	}

	void Collection::RebuildValidImages()
	{
		validImages.clear();
		std::ranges::copy_if(images, std::back_inserter(validImages), [](const Image& a_image) {
			return !a_image.excludeFromLoadscreen;
		});

		previousIndex = { std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max() };
	}

	std::size_t Collection::GetRandomIndex()
	{
		auto maxIndex = validImages.size();

		if (maxIndex <= 1) {
			previousIndex[0] = 0;
			previousIndex[1] = 0;
			return 0;
		}

		std::size_t idx;
		do {
			idx = RNG().generate<std::size_t>(0, maxIndex - 1);
		} while (idx == previousIndex[0] || (maxIndex > 2 && idx == previousIndex[1]));

		previousIndex[1] = previousIndex[0];
		previousIndex[0] = idx;

		return idx;
	}

	const std::string& Collection::GetRandomPath()
	{
		return validImages[GetRandomIndex()].path;
	}

	std::int32_t Collection::GetHighestIndex() const
	{
		if (images.empty()) {
			return -1;
		}
		return images.back().index + 1;
	}

	void Collection::DeleteImagesWithIndex(std::int32_t a_index, bool a_recycle)
	{
		const static auto root = std::filesystem::current_path();

		std::erase_if(images, [&](const Image& a_image) {
			if (a_image.index != a_index) {
				return false;
			}
			auto finalPath = root / a_image.path;
			if (a_recycle) {
				Shared::RecycleFile(finalPath.wstring());
			} else {
				if (!Shared::RemoveFile(finalPath)) {
					logger::warn("\t\tFailed to delete {}", finalPath.string());
				} else {
					logger::info("\tDeleting texture ({})", finalPath.string());
				}
			}
			return true;
		});

		RebuildValidImages();
	}

	bool Collection::ContainsIndex(std::int32_t a_index) const
	{
		return GetImageWithIndex(a_index) != nullptr;
	}

	Image* Collection::GetImageWithIndex(std::int32_t a_index)
	{
		auto it = std::ranges::find(images, a_index, &Image::index);
		return it != images.end() ? &*it : nullptr;
	}

	const Image* Collection::GetImageWithIndex(std::int32_t a_index) const
	{
		auto it = std::ranges::find(images, a_index, &Image::index);
		return it != images.end() ? &*it : nullptr;
	}

	void Collection::ApplyExclusions(const FlatSet<std::int32_t>& a_excluded)
	{
		for (auto& image : images) {
			image.excludeFromLoadscreen = a_excluded.contains(image.index);
		}
		RebuildValidImages();
	}

	void Collection::ToggleLoadScreenForIndex(std::int32_t a_index)
	{
		if (auto image = GetImageWithIndex(a_index)) {
			image->excludeFromLoadscreen = !image->excludeFromLoadscreen;
			RebuildValidImages();
		}
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		useCustomFolderDirectory = a_ini.GetBoolValue("Screenshots", "bCustomPhotoFolder", useCustomFolderDirectory);
		autoHideMenus = a_ini.GetBoolValue("Screenshots", "bAutoHideMenus", autoHideMenus);
		allowMultiScreenshots = a_ini.GetBoolValue("Screenshots", "bMultiScreenshots", allowMultiScreenshots);
		takeScreenshotAsDDS = a_ini.GetBoolValue("Screenshots", "bLoadScreenPics", takeScreenshotAsDDS);

		applyPaintFilter = a_ini.GetBoolValue("Screenshots", "bPaintFilter", applyPaintFilter);
		paintFilter.intensity = static_cast<float>(a_ini.GetDoubleValue("Screenshots", "fPaintIntensity", paintFilter.intensity));
		paintFilter.radius = a_ini.GetLongValue("Screenshots", "iPaintRadius", paintFilter.radius);

		compressTextures = a_ini.GetBoolValue("Screenshots", "bCompressTextures", compressTextures);
		forceSRGB = a_ini.GetBoolValue("Screenshots", "bForceSRGB", forceSRGB);

		excludedImages.clear();
		std::string exclusions = a_ini.GetValue("Gallery", "sExcludedLoadScreens", "");
		if (!exclusions.empty()) {
			for (const auto& entry : string::split(exclusions, ",")) {
				if (!entry.empty()) {
					excludedImages.insert(string::to_num<std::int32_t>(entry));
				}
			}
		}

		ApplyExclusions();
	}

	void Manager::ApplyExclusions()
	{
		screenshots.ApplyExclusions(excludedImages);
		paintings.ApplyExclusions(excludedImages);
	}

	void Manager::LoadScreenshots()
	{
		logger::info("Loading screenshots...");

		photoDirectory = Shared::GetDocumentsFolder("Photos"sv);
		if (auto result = Shared::GetOrCreateDirectory(photoDirectory); !result) {
			logger::error("Failed to create photo directory: {}", result.error().message());
		}

		thumbnailDirectory = Shared::GetDocumentsFolder("Photos/Thumbnails"sv);
		if (auto result = Shared::GetOrCreateDirectory(thumbnailDirectory); !result) {
			logger::error("Failed to create thumbnail folder: {}", result.error().message());
		}

		logger::info("\tScreenshot directory : {}", photoDirectory.string());
		logger::info("\tScreenshot textures : {}", screenshotFolder);
		logger::info("\tPainting textures : {}", paintingFolder);

		screenshots.LoadImages(screenshotFolder);
		paintings.LoadImages(paintingFolder);

		ApplyExclusions();

		Settings::GetSingleton()->Save(FileType::kMCM, [this](auto& ini) {
			index = ini.GetLongValue("Screenshots", "iScreenshotIndex", index);
			AssignHighestPossibleIndex();
			ini.SetLongValue("Screenshots", "iScreenshotIndex", index);
		});

		logger::info("\t{} screenshots", screenshots.size());
		logger::info("\t{} paintings", paintings.size());
		logger::info("\tscreenshot index : {}", index);
	}

	const std::filesystem::path& Manager::GetPhotoDirectory() const
	{
		return photoDirectory;
	}

	const std::filesystem::path& Manager::GetThumbnailDirectory() const
	{
		return thumbnailDirectory;
	}

	void Manager::ToggleLoadScreenForIndex(std::int32_t a_index)
	{
		if (a_index < 0) {
			return;
		}

		if (!excludedImages.erase(a_index)) {
			excludedImages.insert(a_index);
		}

		screenshots.ToggleLoadScreenForIndex(a_index);
		paintings.ToggleLoadScreenForIndex(a_index);

		// serialize
		std::string joined;
		for (const auto& idx : excludedImages) {
			joined += joined.empty() ? std::format("{}", idx) : std::format(",{}", idx);
		}
		Settings::GetSingleton()->Save(FileType::kMCM, [&](auto& ini) {
			ini.SetValue("Gallery", "sExcludedLoadScreens", joined.c_str());
		});
	}

	bool Manager::IsImageExcludedFromLoadScreen(std::int32_t a_index) const
	{
		return excludedImages.contains(a_index);
	}

	std::uint32_t Manager::GetIndex() const
	{
		return index;
	}

	void Manager::AssignHighestPossibleIndex()
	{
		const auto get_photos_index = [this]() {
			std::int32_t photosIndex = -1;
			Shared::ForEachFile(photoDirectory, ".png"sv, [&](const auto& a_path) {
				photosIndex = std::max(photosIndex, Shared::GetScreenshotIndex(a_path.string()) + 1);
			});
			return photosIndex;
		};

		auto mcmIndex = index;
		auto photosIndex = get_photos_index();
		auto vanillaScreenshotIndex = "iScreenShotIndex:Display"_pref.value_or(-1);
		auto screenshotsIndex = screenshots.GetHighestIndex();
		auto paintingsIndex = paintings.GetHighestIndex();

		logger::info("\tAssigning highest screenshot index...");
		logger::info("\t\tmcm index: {}", mcmIndex);
		logger::info("\t\tphoto directory index: {}", photosIndex);
		logger::info("\t\tvanilla directory index: {}", vanillaScreenshotIndex);
		logger::info("\t\tscreenshot textures index: {}", screenshotsIndex);
		logger::info("\t\tpainting textures index: {}", paintingsIndex);

		index = std::max({ mcmIndex, photosIndex, vanillaScreenshotIndex, screenshotsIndex, paintingsIndex });
	}

	void Manager::IncrementIndex()
	{
		index++;
		Settings::GetSingleton()->Save(FileType::kMCM, [this](auto& ini) {
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

	bool Manager::GetForceSRGB() const
	{
		return forceSRGB;
	}

	const Image* Manager::GetScreenshotWithIndex(std::int32_t a_index) const
	{
		return screenshots.GetImageWithIndex(a_index);
	}

	const Image* Manager::GetPaintingWithIndex(std::int32_t a_index) const
	{
		return paintings.GetImageWithIndex(a_index);
	}

	bool Manager::CanDisplayScreenshotInLoadScreen() const
	{
		return takeScreenshotAsDDS && (screenshots.has_valid_images() || paintings.has_valid_images());
	}

	bool Manager::TakeScreenshot()
	{
		bool skipVanillaScreenshot = false;

		const auto renderer = RE::BSGraphics::Renderer::GetSingleton();
		if (!renderer) {
			return skipVanillaScreenshot;
		}

		// capture screenshot
		DirectX::ScratchImage inputImage{};

		const ComPtr<ID3D11Device>        device{ reinterpret_cast<ID3D11Device*>(renderer->data.forwarder) };
		const ComPtr<ID3D11DeviceContext> deviceContext{ reinterpret_cast<ID3D11DeviceContext*>(renderer->data.context) };
		ID3D11Texture2D*                  texture2D{ renderer->data.renderTargets[RE::RENDER_TARGET::kSCREENSHOT].texture };

		if (auto result = DirectX::CaptureTexture(device.Get(), deviceContext.Get(), texture2D, inputImage); result == S_OK) {
			skipVanillaScreenshot = true;

			std::string pngPath = useCustomFolderDirectory ? std::format("{}\\Screenshot_{}.png", photoDirectory.string(), GetIndex()) :
			                                                 std::format("{}_{}.png", *"sScreenShotBaseName:Display"_pref, GetIndex());

			// apply overlay
			if (const auto [overlay, alpha] = MANAGER(PhotoMode)->GetOverlay(); overlay) {
				DirectX::ScratchImage overlayImage;
				DirectX::ScratchImage blendedImage;

				// Convert PNG B8G8R8 format to R8G8B8
				auto hr = DirectX::Convert(overlay->image->GetImages(), 1,
					overlay->image->GetMetadata(),
					inputImage.GetMetadata().format, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT,
					overlayImage);

				if (SUCCEEDED(hr)) {
					Texture::AlphaBlendImage(inputImage.GetImages(), overlayImage.GetImages(), blendedImage, alpha);

					TakeScreenshotAsTexture(renderer, blendedImage, inputImage);
					Texture::SaveToPNG(blendedImage, pngPath, forceSRGB);
					SaveThumbnail(blendedImage, pngPath);

					overlayImage.Release();
					blendedImage.Release();
				} else {
					TakeScreenshotAsTexture(renderer, inputImage, inputImage);
					Texture::SaveToPNG(inputImage, pngPath, forceSRGB);
					SaveThumbnail(inputImage, pngPath);
				}
			} else {
				TakeScreenshotAsTexture(renderer, inputImage, inputImage);
				Texture::SaveToPNG(inputImage, pngPath, forceSRGB);
				SaveThumbnail(inputImage, pngPath);
			}

			IncrementIndex();
		}

		inputImage.Release();

		return skipVanillaScreenshot;
	}

	void Manager::TakeScreenshotAsTexture(RE::BSGraphics::Renderer* a_renderer, const DirectX::ScratchImage& a_ssImage, const DirectX::ScratchImage& a_paintingImage)
	{
		if (!takeScreenshotAsDDS || a_ssImage.GetMetadata().width % 4 != 0 || a_ssImage.GetMetadata().height % 4 != 0) {
			return;
		}

		// regular
		Image screenshotImage(screenshotFolder, GetIndex());

		bool result = false;
		if (compressTextures) {
			DirectX::ScratchImage outputImage;
			result = Texture::CompressTexture(a_renderer, a_ssImage, outputImage) &&
			         Texture::SaveToDDS(outputImage, screenshotImage.path);
			outputImage.Release();
		} else {
			result = Texture::SaveToDDS(a_ssImage, screenshotImage.path);
		}
		if (result) {
			screenshots.AddImage(screenshotImage);
		} else {
			logger::warn("Screenshots: failed to save {}", screenshotImage.path);
		}

		// painting
		if (applyPaintFilter) {
			Image paintingImage(paintingFolder, GetIndex());

			DirectX::ScratchImage outputImage;
			result = Texture::OilPaintingFilter(a_paintingImage.GetImages(), paintFilter.radius, paintFilter.intensity, outputImage);

			if (result) {
				if (compressTextures) {
					DirectX::ScratchImage compressedImage;
					result = Texture::CompressTexture(a_renderer, outputImage, compressedImage) &&
					         Texture::SaveToDDS(compressedImage, paintingImage.path);
					compressedImage.Release();
				} else {
					result = Texture::SaveToDDS(outputImage, paintingImage.path);
				}
			}

			outputImage.Release();
			if (result) {
				paintings.AddImage(paintingImage);
			} else {
				logger::warn("Screenshots: failed to save {}", paintingImage.path);
			}
		}
	}

	void Manager::SaveThumbnail(const DirectX::ScratchImage& a_ssImage, const std::string& a_pngPath)
	{
		const auto&           meta = a_ssImage.GetMetadata();
		DirectX::ScratchImage thumbnail;
		if (SUCCEEDED(DirectX::Resize(*a_ssImage.GetImage(0, 0, 0), static_cast<std::size_t>(meta.width * 0.25f), static_cast<std::size_t>(meta.height * 0.25f), DirectX::TEX_FILTER_FANT, thumbnail))) {
			const auto thumbPath = Shared::GetThumbnailPath(thumbnailDirectory, a_pngPath);
			Texture::SaveToPNG(thumbnail, thumbPath.string(), forceSRGB);
		}
	}

	std::string Manager::GetRandomScreenshot()
	{
		if (!screenshots.has_valid_images()) {
			return {};
		}

		return screenshots.GetRandomPath();
	}

	std::string Manager::GetRandomPainting()
	{
		// fallback to screenshots
		if (!paintings.has_valid_images() || !MANAGER(Screenshot)->CanApplyPaintFilter()) {
			return GetRandomScreenshot();
		}

		return paintings.GetRandomPath();
	}

	void Manager::DeleteImagesWithIndex(std::int32_t a_index, bool a_recycle)
	{
		excludedImages.erase(a_index);

		screenshots.DeleteImagesWithIndex(a_index, a_recycle);
		paintings.DeleteImagesWithIndex(a_index, a_recycle);
	}
}
