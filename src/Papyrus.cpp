#include "Papyrus.h"

#include "Settings.h"

namespace Papyrus
{
	void OnConfigClose(RE::TESQuest*)
	{
		Settings::GetSingleton()->LoadMCMSettings();
	}

	bool Register(RE::BSScript::IVirtualMachine* a_vm)
	{
		if (!a_vm) {
			return false;
		}

		a_vm->RegisterFunction("OnConfigClose", MCM, OnConfigClose);

		logger::info("Registered {} class", MCM);

		return true;
	}
}
