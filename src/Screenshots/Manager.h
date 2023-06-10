#pragma once

namespace Screenshot
{
	inline std::string_view screenshotFolder{ "Data/Textures/Screenshots"sv };
	inline std::string_view paintingFolder{ "Data/Textures/Screenshots/Paintings"sv };

	enum class Type
	{
		kNone,
		kScreenshot,
		kPainting
	};

	struct Paths
	{
		Paths(std::uint32_t index);

		std::string screenshot;
		std::string painting;
	};

	class Manager final : public ISingleton<Manager>
	{
	public:
		void LoadScreenshotIndex(CSimpleIniA& a_ini);
		void LoadSettings(const CSimpleIniA& a_ini);
		void LoadScreenshotTextures();

		void TakeScreenshotAsTexture();

		std::uint32_t GetIndex() const;
		void          IncrementIndex();

		bool        CanDisplayScreenshot() const;
		std::string GetRandomScreenshot();
		std::string GetRandomPaintingShot();

		bool  AllowMultiScreenshots() const;
		float GetKeyHeldDuration() const;

	private:
		static void get_textures(std::string_view a_folder, std::vector<std::string>& a_textures);
		void        AddScreenshotPaths(Paths& a_paths);

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

		bool  allowMultiScreenshots{ true };
		float keyHeldDuration{ 0.5 };
	};

	void InstallHook();
}
