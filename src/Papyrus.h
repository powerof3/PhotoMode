#pragma once

namespace Papyrus
{
	inline constexpr auto MCM = "PhotoMode_MCM"sv;
	inline constexpr auto script = "po3_photomode";

	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;

#define STATIC_ARGS [[maybe_unused]] VM *a_vm, [[maybe_unused]] StackID a_stackID, RE::StaticFunctionTag *
#define STATIC_VARS a_vm, a_stackID

	template <class T>
	bool ToggleMenu([[maybe_unused]] VM* a_vm, [[maybe_unused]] StackID a_stackID, bool a_show)
	{
		auto* manager = T::GetSingleton();
		if (!manager) {
			return false;
		}

		if (manager->IsActive() && a_show) {
			a_vm->TraceStack("[Photo Mode]: Attempted to open menu while it was already open.", a_stackID);
			return false;
		} else if (!manager->IsActive() && !a_show) {
			a_vm->TraceStack("[Photo Mode]: Attempted to close menu while it was not open.", a_stackID);
			return false;
		}

		if (a_show) {
			manager->Activate();
		} else {
			manager->Deactivate();
		}

		return true;
	}

	template<class T>
	bool IsMenuActive()
	{
		auto* manager = T::GetSingleton();
		if (!manager) {
			return false;
		}

		return manager->IsActive();
	}

	void OnConfigClose(RE::TESQuest*);
	bool Register(RE::BSScript::IVirtualMachine* a_vm);
}
