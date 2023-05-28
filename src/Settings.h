#pragma once

class Settings : public ISingleton<Settings>
{
public:
	void LoadSettings() const;
	void SaveSettings() const;

	template <class T>
	void Load(CSimpleIniA& a_ini) const;
	template <class T>
	void Save(CSimpleIniA& a_ini) const;

	const wchar_t* GetPath() const;

private:
	// members
	const wchar_t* path{ L"Data/SKSE/Plugins/po3_PhotoMode.ini" };
};

template <class T>
void Settings::Load(CSimpleIniA& a_ini) const
{
	T::GetSingleton()->LoadSettings(a_ini);
}

template <class T>
void Settings::Save(CSimpleIniA& a_ini) const
{
	T::GetSingleton()->SaveSettings(a_ini);
}
