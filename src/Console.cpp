#include "Console.h"

#include "PhotoMode/Manager.h"

namespace Console
{
	struct StartPhotoMode
	{
		constexpr static auto OG_COMMAND = "ResetGrassFade"sv;

		constexpr static auto LONG_NAME = "StartPhotoMode"sv;
		constexpr static auto SHORT_NAME = "StartPhotoMode"sv;
		constexpr static auto HELP = "Start Photo Mode\n"sv;

		static bool Execute(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
		{
			if (!MANAGER(PhotoMode)->IsActive()) {
				MANAGER(PhotoMode)->Activate();
			} else {
				RE::ConsoleLog::GetSingleton()->Print("Photo Mode is already open");
			}

			return true;
		}
	};

	struct StopPhotoMode
	{
		constexpr static auto OG_COMMAND = "ToggleLandFade"sv;

		constexpr static auto LONG_NAME = "StopPhotoMode"sv;
		constexpr static auto SHORT_NAME = "StopPhotoMode"sv;
		constexpr static auto HELP = "Stop Photo Mode\n"sv;

		static bool Execute(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
		{
			if (MANAGER(PhotoMode)->IsActive()) {
				MANAGER(PhotoMode)->Deactivate();
			} else {
				RE::ConsoleLog::GetSingleton()->Print("Photo Mode is not open");
			}
			return true;
		}
	};

	void Install()
	{
		logger::info("{:*^30}", "CONSOLE COMMANDS");

		ConsoleCommandHandler<StartPhotoMode>::Install();
		ConsoleCommandHandler<StopPhotoMode>::Install();
	}
}
