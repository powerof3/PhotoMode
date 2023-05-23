#include "Screenshots.h"
#include "Input.h"
#include "LoadScreen.h"
#include "Settings.h"
#include "TextureUtil.h"
#include "ImGui/Util.h"

namespace Screenshot
{
	Paths::Paths(std::uint32_t index) :
		screenshot(fmt::format("{}/Screenshot_{}.dds", screenshotFolder, index)),
		painting(fmt::format("{}/Screenshot_{}.dds", paintingFolder, index))
	{}

	void Manager::get_textures(const std::string& a_folder, std::vector<std::string>& a_textures)
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
		ini::get_value(a_ini, index, "PhotoMode", "ScreenShotIndex", nullptr);

		get_textures(screenshotFolder, screenshots);
		get_textures(paintingFolder, paintings);
	}

    void Manager::SaveSettings(CSimpleIniA& a_ini) const
    {
		a_ini.SetBoolValue("Screenshots", "VanillaScreenshots", takeVanillaSS);
		a_ini.SetBoolValue("Screenshots", "LoadscreenScreenshots", takeScreenshotAsTexture);
		a_ini.SetBoolValue("Screenshots", "ApplyPaintingFilter", applyPaintFilter);
		a_ini.SetDoubleValue("Screenshots", "PaintIntensity", paintFilter.intensity);
		a_ini.SetLongValue("Screenshots", "PaintRadius", paintFilter.radius);
	}

    void Manager::Revert()
	{
		takeScreenshotAsTexture = true;
		takeVanillaSS = true;
		applyPaintFilter = true;

		paintFilter.radius = 3;
		paintFilter.intensity = 20;

		LoadScreen::Manager::GetSingleton()->Revert();
	}

    void Manager::Draw()
	{
		ImGui::OnOffToggle("Vanilla screenshots", &takeVanillaSS);
		ImGui::OnOffToggle("Loadscreen Screenshots", &takeScreenshotAsTexture);

		ImGui::BeginDisabled(!takeScreenshotAsTexture);
	    ImGui::Dummy({0,5});
		ImGui::OnOffToggle("Painting filter", &applyPaintFilter, "ENABLED", "DISABLED");

		ImGui::Slider("Paint intensity", &paintFilter.intensity, 1.0f, 100.0f);
		ImGui::Slider("Paint radius", &paintFilter.radius, 1, 10);

		if (ImGui::BeginTabBar("LoadScreen##Bar")) {
			if (ImGui::BeginTabItem("LoadScreen")) {
				LoadScreen::Manager::GetSingleton()->Draw();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
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
		ini.SetLongValue("PhotoMode", "ScreenShotIndex", index);

		(void)ini.SaveFile(path);
	}

	bool Manager::CanDisplayScreenshot() const
	{
		return takeScreenshotAsTexture && (!screenshots.empty() || !paintings.empty());
	}

    bool Manager::TakeScreenshotAsTexture(const RE::BSGraphics::Renderer* a_renderer)
	{
		if (!takeScreenshotAsTexture) {
			return false;
		}

	    Paths ssPaths(GetIndex());

		DirectX::ScratchImage inputImage;
		Texture::CaptureTexture(a_renderer, inputImage);

		Texture::SaveToDDS(a_renderer, inputImage, ssPaths.screenshot);

		if (applyPaintFilter) {
			DirectX::ScratchImage outputImage;

		    Texture::OilPaintingFilter(inputImage.GetImages(), paintFilter.radius, paintFilter.intensity, outputImage);
			Texture::SaveToDDS(a_renderer, outputImage, ssPaths.painting);

            outputImage.Release();
		}

		inputImage.Release();

		IncrementIndex();
		AddScreenshotPaths(ssPaths);

		// render target should have no UI, safe to enable
	    Input::Manager::GetSingleton()->OnScreenshotFinish();

		// return false so it can capture vanilla screenshot as well
		return !takeVanillaSS;
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
			const auto startTime = std::chrono::steady_clock::now();

			if (!get<Input::Manager>()->IsScreenshotQueued() || !get<Manager>()->TakeScreenshotAsTexture(RE::BSGraphics::Renderer::GetSingleton())) {
				func(a_screenshotPath, a_format);
			}

			const auto endTime = std::chrono::steady_clock::now();

			auto durUS = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			auto durMS = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

			logger::info("took {}us/{} ms", durUS, durMS);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHook()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(35556, 36555), OFFSET(0x48E, 0x454) };  // Main::Swap
		stl::write_thunk_call<TakeScreenshot>(target.address());
	}
}
