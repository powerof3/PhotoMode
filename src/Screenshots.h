#pragma once

namespace Screenshot
{
	inline std::string screenshotFolder{ "Data/Textures/Screenshots" };
	inline std::string paintingFolder{ "Data/Textures/Screenshots/Paintings" };

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
		void LoadSettings(CSimpleIniA& a_ini);
		void SaveSettings(CSimpleIniA& a_ini) const;
		void Revert();

		void Draw();

		bool TakeScreenshotAsTexture(const RE::BSGraphics::Renderer* a_renderer);

		std::uint32_t GetIndex() const;
		void          IncrementIndex();

		bool        CanDisplayScreenshot() const;
		std::string GetRandomScreenshot();
		std::string GetRandomPaintingShot();

	private:
		static void get_textures(const std::string& a_folder, std::vector<std::string>& a_textures);
		void        AddScreenshotPaths(Paths& a_paths);

		// members
		std::vector<std::string> screenshots{};
		std::vector<std::string> paintings{};
		std::uint32_t            index{ 0 };

		bool takeScreenshotAsTexture{ true };
		bool takeVanillaSS{ true };

		bool applyPaintFilter{ true };
		struct
		{
			std::int32_t radius{ 3 };
			float        intensity{ 20.0f };
		} paintFilter;
	};

	void InstallHook();
}
