#include "Settings.h"
#include "Input.h"
#include "Screenshots/LoadScreen.h"
#include "Screenshots/Manager.h"

void Settings::LoadSettings() const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	Load<Input::Manager>(ini);       // photomode IO
	Load<Screenshot::Manager>(ini);  // screenshot
	Load<LoadScreen::Manager>(ini);  // loadscreen

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
