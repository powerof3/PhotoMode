#include "Screenshots/Manager.h"

#include "ImGui/Util.h"
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

	void Manager::LoadSettings(CSimpleIniA& a_ini)
	{
		ini::get_value(a_ini, index, "Screenshots", "ScreenShotIndex", nullptr);
		ini::get_value(a_ini, takeScreenshotAsPNG, "Screenshots", "SaveAsPNG", ";Take regular screenshots in PhotoMode");
		ini::get_value(a_ini, takeScreenshotAsDDS, "Screenshots", "SaveAsDDS", ";Save screenshots as DDS (show in loading screens)");
		ini::get_value(a_ini, applyPaintFilter, "Screenshots", "ApplyPaintingFilter", ";Apply an oil painting filter (to loadscreen screenshots)");
		ini::get_value(a_ini, paintFilter.intensity, "Screenshots", "PaintIntensity", nullptr);
		ini::get_value(a_ini, paintFilter.radius, "Screenshots", "PaintRadius", nullptr);
		ini::get_value(a_ini, compressTextures, "Screenshots", "CompressTextures", ";Save screenshot textures with BC7 compression");

		get_textures(screenshotFolder, screenshots);
		get_textures(paintingFolder, paintings);
	}

	void Manager::SaveSettings(CSimpleIniA& a_ini) const
	{
		a_ini.SetBoolValue("Screenshots", "SaveAsPNG", takeScreenshotAsPNG);
		a_ini.SetBoolValue("Screenshots", "SaveAsDDS", takeScreenshotAsDDS);
		a_ini.SetBoolValue("Screenshots", "ApplyPaintingFilter", applyPaintFilter);
		a_ini.SetDoubleValue("Screenshots", "PaintIntensity", paintFilter.intensity);
		a_ini.SetLongValue("Screenshots", "PaintRadius", paintFilter.radius);
	}

	void Manager::Revert()
	{
		takeScreenshotAsPNG = true;
		takeScreenshotAsDDS = true;
		applyPaintFilter = true;

		paintFilter.radius = 3;
		paintFilter.intensity = 20;

		LoadScreen::Manager::GetSingleton()->Revert();
	}

	void Manager::Draw()
	{
		ImGui::OnOffToggle("$PM_RegularScreenshots"_T, &takeScreenshotAsPNG, "$PM_YES"_T, "$PM_NO"_T);
		ImGui::OnOffToggle("$PM_LoadScreenScreenshots"_T, &takeScreenshotAsDDS, "$PM_YES"_T, "$PM_NO"_T);

		ImGui::BeginDisabled(!takeScreenshotAsDDS);
		{
			ImGui::Dummy({ 0, 5 });
			ImGui::OnOffToggle("$PM_PaintingFilter"_T, &applyPaintFilter, "$PM_ENABLED"_T, "$PM_DISABLED"_T);

			ImGui::Slider("$PM_PaintIntensity"_T, &paintFilter.intensity, 1.0f, 100.0f);
			ImGui::Slider("$PM_PaintRadius"_T, &paintFilter.radius, 1, 10);

			if (ImGui::BeginTabBar("LoadScreen##Bar")) {
				if (ImGui::BeginTabItem("$PM_LoadScreen"_T)) {
					LoadScreen::Manager::GetSingleton()->Draw();
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::EndDisabled();
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

		const auto path = Settings::GetSingleton()->GetPath();

		CSimpleIniA ini;
		ini.SetUnicode();
		ini.LoadFile(path);
		ini.SetLongValue("Screenshots", "ScreenShotIndex", index);

		(void)ini.SaveFile(path);
	}

	bool Manager::CanDisplayScreenshot() const
	{
		return takeScreenshotAsDDS && (!screenshots.empty() || !paintings.empty());
	}

	bool Manager::TakeScreenshotAsTexture()
	{
		if (!takeScreenshotAsDDS) {
			return false;
		}

		const auto renderer = RE::BSGraphics::Renderer::GetSingleton();
		if (!renderer) {
			return false;
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

		// return false so it can capture vanilla screenshot as well
		return !takeScreenshotAsPNG;
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
			if (!Input::Manager::GetSingleton()->IsScreenshotQueued() || !Screenshot::Manager::GetSingleton()->TakeScreenshotAsTexture()) {
				func(a_screenshotPath, a_format);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHook()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(35556, 36555), OFFSET(0x48E, 0x454) };  // Main::Swap
		stl::write_thunk_call<TakeScreenshot>(target.address());
	}
}
