#include "Settings.h"

#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"

void Settings::SerializeINI(const wchar_t* a_path, const std::function<void(CSimpleIniA&)>& a_func)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(a_path); rc < SI_OK) {
		return;
	}

	a_func(ini);

	(void)ini.SaveFile(a_path);
}

void Settings::LoadSettings() const
{
    SerializeINI(configPath, [](auto& ini) {
		MANAGER(PhotoMode)->LoadSettings(ini);          // hotkeys
		MANAGER(Screenshot)->LoadScreenshotIndex(ini);  // screenshot index
	});

	LoadMCMSettings();
}

void Settings::LoadMCMSettings() const
{
	SerializeINI(defaultMCMPath, [](auto& ini) {
		MANAGER(Screenshot)->LoadSettings(ini);
	});
	SerializeINI(userMCMPath, [](auto& ini) {
		MANAGER(Screenshot)->LoadSettings(ini);
	});
}

void Settings::SaveSettings() const
{
	SerializeINI(configPath, [](auto& ini) {
		MANAGER(PhotoMode)->SaveSettings(ini);	// hotkeys
	});
}

const wchar_t* Settings::GetConfigPath() const
{
	return configPath;
}
