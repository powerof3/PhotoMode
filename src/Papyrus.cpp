#include "Papyrus.h"

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

	static bool ToggleMenu(STATIC_ARGS, bool a_show)
	{
		auto* manager = PhotoMode::Manager::GetSingleton();
		if (!manager) {
			a_vm->TraceStack("[Photo Mode]: Failed to get the Photo Mode Manager.", a_stackID);
			return false;
		}

		if (manager->IsActive() && a_show) {
			if (a_vm) {
				a_vm->TraceStack("[Photo Mode]: Attempted to open menu while it was already open.", a_stackID);
			}
			return false;
		} else if (!manager->IsActive() && !a_show) {
			if (a_vm) {
				a_vm->TraceStack("[Photo Mode]: Attempted to close menu while it was not open.", a_stackID);
			}
			return false;
		}

		if (a_show) {
			manager->Activate();
		} else {
			manager->Deactivate();
		}
		return true;
	}

	static bool IsPhotoModeActive(STATIC_ARGS)
	{
		auto* manager = PhotoMode::Manager::GetSingleton();
		if (!manager) {
			return false;
		}

		return manager->IsActive();
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		if (!a_vm) {
			return false;
		}

		a_vm->RegisterFunction("OnConfigClose", MCM, OnConfigClose);

		a_vm->RegisterFunction("ToggleMenu"sv, script, ToggleMenu);
		a_vm->RegisterFunction("IsPhotoModeActive"sv, script, IsPhotoModeActive);
		a_vm->RegisterFunction("GetVersion"sv, script, GetVersion);

		logger::info("Registered {} class", MCM);

		return true;
	}
}
