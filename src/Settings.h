#pragma once

class Settings : public ISingleton<Settings>
{
public:
	void LoadSettings() const;
	void LoadMCMSettings() const;

	const wchar_t* GetConfigPath() const;

private:
	static void SerializeINI(const wchar_t* a_path, const std::function<void(CSimpleIniA&)>& a_func);

	// members
	const wchar_t* configPath{ L"Data/SKSE/Plugins/po3_PhotoMode.ini" };
	const wchar_t* defaultMCMPath{ L"Data/MCM/Config/PhotoMode/settings.ini" };
	const wchar_t* userMCMPath{ L"Data/MCM/Settings/PhotoMode.ini" };
};
