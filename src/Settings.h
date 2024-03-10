#pragma once

class Settings : public ISingleton<Settings>
{
public:
	void LoadSettings() const;
	void LoadMCMSettings() const;

	void SerializeMCM(std::function<void(CSimpleIniA&)> a_func);
	void SerializeStyles(std::function<void(CSimpleIniA&)> a_func) const;
	void SerializeFonts(std::function<void(CSimpleIniA&)> a_func) const;

private:
	static void SerializeINI(const wchar_t* a_path, std::function<void(CSimpleIniA&)> a_func, bool a_generate = false);
	static void SerializeINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, std::function<void(CSimpleIniA&)> a_func);

	// members
	const wchar_t* fontsPath{ L"Data/Interface/PhotoMode/fonts.ini" };
	const wchar_t* stylesPath{ L"Data/Interface/PhotoMode/styles.ini" };

	const wchar_t* defaultMCMPath{ L"Data/MCM/Config/PhotoMode/settings.ini" };
	const wchar_t* userMCMPath{ L"Data/MCM/Settings/PhotoMode.ini" };

	const wchar_t* defaultDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks.ini" };
	const wchar_t* userDisplayTweaksPath{ L"Data/SKSE/Plugins/SSEDisplayTweaks_Custom.ini" };
};
