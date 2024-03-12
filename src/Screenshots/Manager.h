#pragma once

namespace Screenshot
{
	inline std::string_view screenshotFolder{ "Data\\Textures\\PhotoMode\\Screenshots"sv };
	inline std::string_view paintingFolder{ "Data\\Textures\\PhotoMode\\Screenshots\\Paintings"sv };

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
		void LoadScreenshots();

		bool TakeScreenshot();

		std::uint32_t GetIndex() const;
		void          IncrementIndex();

		bool        CanDisplayScreenshotInLoadScreen() const;
		std::string GetRandomScreenshot();
		std::string GetRandomPainting();

		bool AllowMultiScreenshots() const;
		bool CanAutoHideMenus() const;
		bool CanApplyPaintFilter() const;

	private:
		void AddScreenshotPaths(Paths& a_paths);
		void TakeScreenshotAsTexture(const DirectX::ScratchImage& a_ssImage, const DirectX::ScratchImage& a_paintingImage);

		// members
		std::vector<std::string> screenshots{};
		std::vector<std::string> paintings{};
		std::int32_t             index{ -1 };

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

		bool                  useCustomFolderDirectory{ true };
		std::filesystem::path photoDirectory{};
	};
}
