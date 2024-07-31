#pragma once

namespace Screenshot
{
	inline std::string_view screenshotFolder{ "Data\\Textures\\PhotoMode\\Screenshots"sv };
	inline std::string_view paintingFolder{ "Data\\Textures\\PhotoMode\\Screenshots\\Paintings"sv };

	struct Image
	{
		Image() = default;
		Image(std::string& a_path, std::uint32_t a_index);
		Image(std::string& a_path);

		bool operator<(const Image& a_rhs) const
		{
			return index < a_rhs.index;
		}

		// members
		std::string  path{};
		std::int32_t index{ -1 };
	};

	struct LoadScreenImage
	{
		LoadScreenImage(std::uint32_t index);

		// members
		std::uint32_t index;
		std::string   screenshot;
		std::string   painting;
	};

	class Manager final : public ISingleton<Manager>
	{
	public:
		void LoadMCMSettings(const CSimpleIniA& a_ini);
		void LoadScreenshots();

		bool TakeScreenshot();

		std::uint32_t GetIndex() const;
		void          AssignHighestPossibleIndex();
		void          IncrementIndex();

		bool        CanDisplayScreenshotInLoadScreen() const;	
		std::string GetRandomScreenshot();
		std::string GetRandomPainting();

		bool AllowMultiScreenshots() const;
		bool CanAutoHideMenus() const;
		bool CanApplyPaintFilter() const;

	private:
		void AddLoadScreenImages(LoadScreenImage& a_images);
		void TakeScreenshotAsTexture(const DirectX::ScratchImage& a_ssImage, const DirectX::ScratchImage& a_paintingImage);

		// members
		std::vector<Image> screenshots{};
		std::vector<Image> paintings{};
		std::int32_t       index{ -1 };

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
