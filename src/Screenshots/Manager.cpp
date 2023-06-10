#include "Screenshots/Manager.h"

#include "Input.h"
#include "LoadScreen.h"
#include "Settings.h"
#include "Textures.h"

namespace Screenshot
{
	Paths::Paths(std::uint32_t index) :
		screenshot(fmt::format("{}/Screenshot_{}.dds", screenshotFolder, index)),
		painting(fmt::format("{}/Screenshot_{}.dds", paintingFolder, index))
	{}

	void Manager::get_textures(std::string_view a_folder, std::vector<std::string>& a_textures)
	{
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
	}

	void Manager::LoadScreenshotIndex(CSimpleIniA& a_ini)
	{
		ini::get_value(a_ini, index, "Screenshots", "Index", nullptr);
	}

	void Manager::LoadSettings(const CSimpleIniA& a_ini)
	{
		allowMultiScreenshots = a_ini.GetBoolValue("Screenshots", "bMultiScreenshots", allowMultiScreenshots);
		takeScreenshotAsDDS = a_ini.GetBoolValue("Screenshots", "bLoadScreenPics", takeScreenshotAsDDS);

		applyPaintFilter = a_ini.GetBoolValue("Screenshots", "bPaintFilter", applyPaintFilter);
		paintFilter.intensity = a_ini.GetDoubleValue("Screenshots", "bPaintIntensity", paintFilter.intensity);
		paintFilter.radius = a_ini.GetLongValue("Screenshots", "iPaintRadius", paintFilter.radius);

		compressTextures = a_ini.GetBoolValue("Screenshots", "bCompressTextures", compressTextures);

		MANAGER(LoadScreen)->LoadSettings(a_ini);
	}

	void Manager::LoadScreenshotTextures()
	{
		logger::info("Loading screenshot textures...");

		get_textures(screenshotFolder, screenshots);
		get_textures(paintingFolder, paintings);

		logger::info("\t{} screenshots", screenshots.size());
		logger::info("\t{} paintings", paintings.size());
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
		const auto path = Settings::GetSingleton()->GetConfigPath();

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(path);

		ini.SetLongValue("Screenshots", "Index", index++);

		(void)ini.SaveFile(path);
	}

	bool Manager::AllowMultiScreenshots() const
	{
		return allowMultiScreenshots;
	}

	float Manager::GetKeyHeldDuration() const
	{
		return keyHeldDuration;
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

		// render target should have no UI, safe to enable
		Input::Manager::GetSingleton()->OnScreenshotFinish();
	}

	std::string Manager::GetRandomScreenshot()
	{
		if (screenshots.empty()) {
			return {};
		}

		return screenshots[RNG().Generate<std::size_t>(0, screenshots.size() - 1)];
	}

	std::string Manager::GetRandomPaintingShot()
	{
		// fallback to screenshots
		if (paintings.empty()) {
			return GetRandomScreenshot();
		}

		return paintings[RNG().Generate<std::size_t>(0, screenshots.size() - 1)];
	}

	struct TakeScreenshot
	{
		static void thunk(const char* a_screenshotPath, RE::BSGraphics::TextureFileFormat a_format)
		{
			if (MANAGER(Input)->IsScreenshotQueued()) {
				MANAGER(Screenshot)->TakeScreenshotAsTexture();
			}
			func(a_screenshotPath, a_format);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHook()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(35556, 36555), OFFSET(0x48E, 0x454) };  // Main::Swap
		stl::write_thunk_call<TakeScreenshot>(target.address());
	}
}
