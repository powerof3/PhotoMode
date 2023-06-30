#pragma once

class Settings : public ISingleton<Settings>
{
public:
	void LoadSettings() const;
	void LoadMCMSettings() const;

private:
	static void SerializeINI(const wchar_t* a_path, std::function<void(CSimpleIniA&)> a_func, bool a_generate = false);

	// members
	const wchar_t* fontsPath{ L"Data/PhotoMode/Fonts/fonts.ini" };
	const wchar_t* defaultMCMPath{ L"Data/MCM/Config/PhotoMode/settings.ini" };
	const wchar_t* userMCMPath{ L"Data/MCM/Settings/PhotoMode.ini" };
};
