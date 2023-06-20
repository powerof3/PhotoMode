#include "Settings.h"

#include "ImGui/IconsFonts.h"
#include "Input.h"
#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/LoadScreen.h"
#include "Screenshots/Manager.h"

void Settings::SerializeINI(const wchar_t* a_path, const std::function<void(CSimpleIniA&)> a_func, bool a_generate)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (const auto rc = ini.LoadFile(a_path); !a_generate && rc < SI_OK) {
		return;
	}

	a_func(ini);

	(void)ini.SaveFile(a_path);
}

void Settings::LoadSettings() const
{
	SerializeINI(
		fontsPath, [](auto& ini) {
			MANAGER(IconFont)->LoadSettings(ini);  // fonts, icons
		},
		true);

	LoadMCMSettings();
}

void Settings::LoadMCMSettings() const
{
	constexpr auto load_mcm_settings = [](auto& ini) {
		MANAGER(Hotkeys)->LoadHotKeys(ini);

		MANAGER(Screenshot)->LoadMCMSettings(ini);
		MANAGER(LoadScreen)->LoadMCMSettings(ini);

		MANAGER(IconFont)->LoadMCMSettings(ini);  // button scheme
		MANAGER(Input)->LoadMCMSettings(ini);     // key held duration

		MANAGER(PhotoMode)->LoadMCMSettings(ini);
	};

	SerializeINI(defaultMCMPath, load_mcm_settings);
	SerializeINI(userMCMPath, load_mcm_settings);
}
