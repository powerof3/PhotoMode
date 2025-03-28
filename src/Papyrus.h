#pragma once

namespace Papyrus
{
	inline constexpr auto MCM = "PhotoMode_MCM"sv;
	inline constexpr auto script = "po3_photomode";

	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;
#define STATIC_ARGS [[maybe_unused]] VM *a_vm, [[maybe_unused]] StackID a_stackID, RE::StaticFunctionTag *

	void OnConfigClose(RE::TESQuest*);

	bool Register(RE::BSScript::IVirtualMachine* a_vm);
}
