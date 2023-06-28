#pragma once

namespace Screenshot
{
	inline std::string_view screenshotFolder{ "Data/Textures/Screenshots"sv };
	inline std::string_view paintingFolder{ "Data/Textures/Screenshots/Paintings"sv };

	struct Paths
	{
		Paths(std::uint32_t index);

		std::string screenshot;
		std::string painting;
	};

	class Manager final : public ISingleton<Manager>
	{
	public:
		void LoadMCMSettings(const CSimpleIniA& a_ini);
		void LoadScreenshotTextures();

		void TakeScreenshotAsTexture();

		std::uint32_t GetIndex() const;
		void          IncrementIndex();

		bool        CanDisplayScreenshot() const;
		std::string GetRandomScreenshot();
		std::string GetRandomPainting();

		bool AllowMultiScreenshots() const;
		bool CanAutoHideMenus() const;

	private:
		void AddScreenshotPaths(Paths& a_paths);

		// members
		std::vector<std::string> screenshots{};
		std::vector<std::string> paintings{};
		std::uint32_t            index{ 0 };

		bool takeScreenshotAsDDS{ true };
		bool compressTextures{ true };

		bool applyPaintFilter{ true };
		struct
		{
			std::int32_t radius{ 4 };
			float        intensity{ 30.0f };
		} paintFilter;

		bool allowMultiScreenshots{ true };
		bool autoHideMenus{ true };
	};
}
