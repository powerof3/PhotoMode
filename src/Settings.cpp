#include "Settings.h"
#include "Screenshots.h"
#include "Input.h"
#include "LoadScreen.h"
#include "PhotoMode/Manager.h"

void Settings::LoadSettings() const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	Load<PhotoMode::Manager>(ini);
    Load<Input::Manager>(ini);
	Load<Screenshot::Manager>(ini);
	Load<LoadScreen::Manager>(ini);

	(void)ini.SaveFile(path);
}

void Settings::SaveSettings() const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	Save<Screenshot::Manager>(ini);
	Save<LoadScreen::Manager>(ini);

	(void)ini.SaveFile(path);
}

const wchar_t* Settings::GetPath() const
{
	return path;
}
