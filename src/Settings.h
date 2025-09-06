#pragma once

enum class FileType
{
	kFonts,
	kStyles,
	kMCM,
	kDisplayTweaks,
};

class Settings
{
public:
	using INIFunc = std::function<void(CSimpleIniA&)>;

	static Settings* GetSingleton()
	{
		return &instance;
	}

	void Load(FileType type, INIFunc a_func, bool a_generate = false) const;
	void Save(FileType type, INIFunc a_func, bool a_generate = false) const;

	void LoadMCMSettings() const;

private:
	static void LoadINI(const wchar_t* a_path, INIFunc a_func, bool a_generate = false);
	static void LoadINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, INIFunc a_func);

	// members
	const wchar_t* fontsPath{ L"Data/Interface/PhotoMode/fonts.ini" };
	const wchar_t* stylesPath{ L"Data/Interface/PhotoMode/styles.ini" };

	const wchar_t* defaultMCMPath{ L"Data/MCM/Config/PhotoMode/settings.ini" };
	const wchar_t* userMCMPath{ L"Data/MCM/Settings/PhotoMode.ini" };

	const wchar_t* defaultDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks.ini" };
	const wchar_t* userDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks_Custom.ini" };

	static Settings instance;
};

inline constinit Settings Settings::instance;
