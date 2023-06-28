#include "Screenshots/Manager.h"

#include "Graphics.h"

namespace Screenshot
{
	Paths::Paths(std::uint32_t index) :
		screenshot(fmt::format("{}/Screenshot_{}.dds", screenshotFolder, index)),
		painting(fmt::format("{}/Screenshot_{}.dds", paintingFolder, index))
	{}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		autoHideMenus = a_ini.GetBoolValue("Screenshots", "bAutoHideMenus", autoHideMenus);
		allowMultiScreenshots = a_ini.GetBoolValue("Screenshots", "bMultiScreenshots", allowMultiScreenshots);
		takeScreenshotAsDDS = a_ini.GetBoolValue("Screenshots", "bLoadScreenPics", takeScreenshotAsDDS);

		applyPaintFilter = a_ini.GetBoolValue("Screenshots", "bPaintFilter", applyPaintFilter);
		paintFilter.intensity = a_ini.GetDoubleValue("Screenshots", "bPaintIntensity", paintFilter.intensity);
		paintFilter.radius = a_ini.GetLongValue("Screenshots", "iPaintRadius", paintFilter.radius);

		compressTextures = a_ini.GetBoolValue("Screenshots", "bCompressTextures", compressTextures);
	}

	void Manager::LoadScreenshotTextures()
	{
		logger::info("Loading screenshot textures...");

		constexpr auto get_textures = [](std::string_view a_folder, std::vector<std::string>& a_textures) {
			const std::filesystem::directory_entry folder{ a_folder };
			if (!folder.exists()) {
				std::filesystem::create_directory(a_folder);
				return;
			}

			const auto iterator = std::filesystem::directory_iterator(a_folder);

			for (const auto& entry : iterator) {
				if (entry.exists()) {
					if (const auto& path = entry.path(); !path.empty() && path.extension() == ".dds") {
						auto pathStr = entry.path().string();
						a_textures.push_back(Texture::Sanitize(pathStr));
					}
				}
			}
		};

		get_textures(screenshotFolder, screenshots);
		get_textures(paintingFolder, paintings);

		index = screenshots.size();  // current index + 1

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
	}

	bool Manager::AllowMultiScreenshots() const
	{
		return allowMultiScreenshots;
	}

	bool Manager::CanAutoHideMenus() const
	{
		return autoHideMenus;
	}

	bool Manager::CanDisplayScreenshot() const
	{
		return takeScreenshotAsDDS && (!screenshots.empty() || !paintings.empty());
	}

	void Manager::TakeScreenshotAsTexture()
	{
		if (!takeScreenshotAsDDS) {
			return;
		}

		const auto renderer = RE::BSGraphics::Renderer::GetSingleton();
		if (!renderer) {
			return;
		}

		Paths ssPaths(GetIndex());

		// capture screenshot to be used for regular dds/painted dds
		DirectX::ScratchImage inputImage;
		Texture::CaptureTexture(renderer, inputImage);

		// regular
		if (compressTextures) {
			DirectX::ScratchImage outputImage;
			Texture::CompressTexture(renderer, inputImage, outputImage);
			Texture::SaveToDDS(outputImage, ssPaths.screenshot);
			outputImage.Release();
		} else {
			Texture::SaveToDDS(inputImage, ssPaths.screenshot);
		}

		// painting
		if (applyPaintFilter) {
			DirectX::ScratchImage outputImage;
			Texture::OilPaintingFilter(inputImage.GetImages(), paintFilter.radius, paintFilter.intensity, outputImage);
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

		inputImage.Release();

		IncrementIndex();
		AddScreenshotPaths(ssPaths);
	}

	std::string Manager::GetRandomScreenshot()
	{
		if (screenshots.empty()) {
			return {};
		}

		return screenshots[RNG().Generate<std::size_t>(0, screenshots.size() - 1)];
	}

	std::string Manager::GetRandomPainting()
	{
		// fallback to screenshots
		if (paintings.empty()) {
			return GetRandomScreenshot();
		}

		return paintings[RNG().Generate<std::size_t>(0, screenshots.size() - 1)];
	}
}
