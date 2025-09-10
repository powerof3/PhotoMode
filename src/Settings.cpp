#include "Settings.h"

#include "ImGui/IconsFonts.h"
#include "Input.h"
#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/LoadScreen.h"
#include "Screenshots/Manager.h"

void Settings::LoadINI(const wchar_t* a_path, const INIFunc a_func, bool a_generate)
{
	CSimpleIniA ini;
	ini.SetUnicode();

	if (ini.LoadFile(a_path) >= SI_OK || a_generate) {
		a_func(ini);

		if (a_generate) {
			(void)ini.SaveFile(a_path);
		}
	}
}

void Settings::LoadINI(const wchar_t* a_defaultPath, const wchar_t* a_userPath, INIFunc a_func)
{
	LoadINI(a_defaultPath, a_func);
	LoadINI(a_userPath, a_func);
}

void Settings::Load(FileType type, INIFunc a_func, bool a_generate) const
{
	switch (type) {
	case FileType::kFonts:
		LoadINI(fontsPath, a_func, a_generate);
		break;
	case FileType::kStyles:
		LoadINI(stylesPath, a_func, a_generate);
		break;
	case FileType::kMCM:
		LoadINI(defaultMCMPath, userMCMPath, a_func);
		break;
	case FileType::kDisplayTweaks:
		LoadINI(defaultDisplayTweaksPath, userDisplayTweaksPath, a_func);
		break;
	default:
		break;
	}
}

void Settings::Save(FileType type, INIFunc a_func, bool a_generate) const
{
	switch (type) {
	case FileType::kFonts:
		LoadINI(fontsPath, a_func, a_generate);
		break;
	case FileType::kStyles:
		LoadINI(stylesPath, a_func, a_generate);
		break;
	case FileType::kMCM:
		LoadINI(defaultMCMPath, a_func, a_generate);
		break;
	default:
		break;
	}
}

void Settings::LoadMCMSettings() const
{
	constexpr auto load_mcm = [](auto& ini) {
		MANAGER(Hotkeys)->LoadHotKeys(ini);

		MANAGER(Screenshot)->LoadMCMSettings(ini);
		MANAGER(LoadScreen)->LoadMCMSettings(ini);

		MANAGER(IconFont)->LoadMCMSettings(ini);  // button scheme
		MANAGER(Input)->LoadMCMSettings(ini);     // key held duration

		MANAGER(PhotoMode)->LoadMCMSettings(ini);
	};

	Load(FileType::kMCM, load_mcm);
}
