#include "Console.h"

#include "Gallery/Manager.h"
#include "PhotoMode/Manager.h"

namespace Console
{
	struct StartPhotoMode
	{
		constexpr static auto OG_COMMAND = "ResizeLargeRefLODGrid"sv;

		constexpr static auto LONG_NAME = "PhotoMode"sv;
		constexpr static auto SHORT_NAME = "PhotoMode"sv;
		constexpr static auto HELP = "Open Photo Mode\n"sv;

		static bool Execute(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
		{
			SKSE::GetTaskInterface()->AddTask([]() {
				MANAGER(PhotoMode)->IsActive() ? MANAGER(PhotoMode)->Deactivate() : MANAGER(PhotoMode)->Activate();
			});
			return true;
		}
	};

	struct StartPhotoGallery
	{
		constexpr static auto OG_COMMAND = "ToggleStippleFade"sv;

		constexpr static auto LONG_NAME = "PhotoGallery"sv;
		constexpr static auto SHORT_NAME = "PhotoGallery"sv;
		constexpr static auto HELP = "Open Photo Gallery\n"sv;

		static bool Execute(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
		{
			SKSE::GetTaskInterface()->AddTask([]() {
				MANAGER(Gallery)->IsActive() ? MANAGER(Gallery)->Deactivate() : MANAGER(Gallery)->Activate();
			});
			return true;
		}
	};

	void Install()
	{
		logger::info("{:*^30}", "CONSOLE COMMANDS");

		ConsoleCommandHandler<StartPhotoMode>::Install();
		ConsoleCommandHandler<StartPhotoGallery>::Install();
	}
}
