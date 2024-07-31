#pragma once

namespace Screenshot
{
	inline std::string_view screenshotFolder{ "data\\textures\\photomode\\screenshots"sv };
	inline std::string_view paintingFolder{ "data\\textures\\photomode\\screenshots\\paintings"sv };

	// .../Screenshot48.dds, 48
	struct Image
	{
		Image() = default;
		Image(std::string_view a_path, std::uint32_t a_index);
		Image(std::string& a_path);

		bool operator<(const Image& a_rhs) const
		{
			return index < a_rhs.index;
		}

		// members
		std::string  path{};
		std::int32_t index{ -1 };
	};

	// Collection of photo textures to be displayed on loading screens.
	// Avoids consecutive screenshots in a row
	struct Collection
	{
		bool        empty() const { return images.empty(); }
		std::size_t size() const { return images.size(); }

		void               LoadImages(std::string_view a_folder);
		void               AddImage(Image& a_image);
		const std::string& GetRandomPath();
		std::int32_t       GetHighestIndex() const;

		// members
		std::vector<Image>         images{};
		std::array<std::size_t, 2> previousIndex{ std::numeric_limits<std::size_t>::max() };

	private:
		std::size_t GetRandomIndex();
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
		void TakeScreenshotAsTexture(const DirectX::ScratchImage& a_ssImage, const DirectX::ScratchImage& a_paintingImage);

		// members
		Collection   screenshots{};
		Collection   paintings{};
		std::int32_t index{ -1 };

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
