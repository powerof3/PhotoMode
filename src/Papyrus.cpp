#include "Papyrus.h"

#include "Gallery/Manager.h"
#include "PhotoMode/Manager.h"

#include "Settings.h"

namespace Papyrus
{
	void OnConfigClose(RE::TESQuest*)
	{
		Settings::GetSingleton()->LoadMCMSettings();
	}

	static std::vector<std::int32_t> GetVersion(STATIC_ARGS)
	{
		return { Version::MAJOR, Version::MINOR, Version::PATCH };
	}

	static bool TogglePhotoMode(STATIC_ARGS, bool a_show)
	{
		return ToggleMenu<PhotoMode::Manager>(STATIC_VARS, a_show);
	}

	static bool TogglePhotoGallery(STATIC_ARGS, bool a_show)
	{
		return ToggleMenu<Gallery::Manager>(STATIC_VARS, a_show);
	}

	static bool IsPhotoModeActive(STATIC_ARGS)
	{
		return IsMenuActive<PhotoMode::Manager>();
	}

	static bool IsPhotoGalleryActive(STATIC_ARGS)
	{
		return IsMenuActive<Gallery::Manager>();
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		if (!a_vm) {
			return false;
		}

		a_vm->RegisterFunction("OnConfigClose", MCM, OnConfigClose);

		a_vm->RegisterFunction("TogglePhotoMode"sv, script, TogglePhotoMode);
		a_vm->RegisterFunction("IsPhotoModeActive"sv, script, IsPhotoModeActive);
		a_vm->RegisterFunction("TogglePhotoGallery"sv, script, TogglePhotoGallery);
		a_vm->RegisterFunction("IsPhotoGalleryActive"sv, script, IsPhotoGalleryActive);
		a_vm->RegisterFunction("GetVersion"sv, script, GetVersion);

		logger::info("Registered {} class", MCM);

		return true;
	}
}
