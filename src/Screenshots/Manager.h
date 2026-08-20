#pragma once

namespace Screenshot
{
	inline constexpr std::string_view screenshotFolder{ R"(data\textures\photomode\screenshots)" };
	inline constexpr std::string_view paintingFolder{ R"(data\textures\photomode\screenshots\paintings)" };

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
		bool         excludeFromLoadscreen{ false };
	};

	// Collection of photo textures to be displayed on loading screens.
	// Avoids consecutive screenshots in a row
	struct Collection
	{
		bool        empty() const { return images.empty(); }
		bool        has_valid_images() const { return !validImages.empty(); }
		std::size_t size() const { return images.size(); }

		void               LoadImages(std::string_view a_folder);
		void               AddImage(Image a_image);
		const std::string& GetRandomPath();
		std::int32_t       GetHighestIndex() const;

		void         DeleteImagesWithIndex(std::int32_t a_index, bool a_recycle);
		bool         ContainsIndex(std::int32_t a_index) const;
		Image*       GetImageWithIndex(std::int32_t a_index);
		const Image* GetImageWithIndex(std::int32_t a_index) const;

		void ApplyExclusions(const FlatSet<std::int32_t>& a_excluded);
		void ToggleLoadScreenForIndex(std::int32_t a_index);

		// members
		std::vector<Image>         images{};
		std::vector<Image>         validImages{};
		std::array<std::size_t, 2> previousIndex{ std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max() };

	private:
		void        ProcessImages(std::string_view a_folder);
		void        RebuildValidImages();
		std::size_t GetRandomIndex();
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void LoadMCMSettings(const CSimpleIniA& a_ini);
		void LoadScreenshots();

		bool TakeScreenshot();

		const std::filesystem::path& GetPhotoDirectory() const;
		const std::filesystem::path& GetThumbnailDirectory() const;

		void DeleteImagesWithIndex(std::int32_t a_index, bool a_recycle);

		// opt out of loading screens
		void ToggleLoadScreenForIndex(std::int32_t a_index);
		bool IsImageExcludedFromLoadScreen(std::int32_t a_index) const;

		std::uint32_t GetIndex() const;
		void          AssignHighestPossibleIndex();
		void          IncrementIndex();

		bool        CanDisplayScreenshotInLoadScreen() const;
		std::string GetRandomScreenshot();
		std::string GetRandomPainting();

		bool AllowMultiScreenshots() const;
		bool CanAutoHideMenus() const;
		bool CanApplyPaintFilter() const;
		bool GetForceSRGB() const;

		const Image* GetScreenshotWithIndex(std::int32_t a_index) const;
		const Image* GetPaintingWithIndex(std::int32_t a_index) const;

		void ApplyExclusions();
		void ApplyExclusion(std::int32_t a_index);

	private:
		void TakeScreenshotAsTexture(RE::BSGraphics::Renderer* a_renderer, const DirectX::ScratchImage& a_ssImage, const DirectX::ScratchImage& a_paintingImage);
		void SaveThumbnail(const DirectX::ScratchImage& a_ssImage, const std::string& a_pngPath);

		// members
		Collection   screenshots{};
		Collection   paintings{};
		std::int32_t index{ -1 };

		bool takeScreenshotAsDDS{ true };
		bool compressTextures{ true };
		bool forceSRGB{ true };

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
		std::filesystem::path thumbnailDirectory{};

		FlatSet<std::int32_t> excludedImages{};
	};
}
