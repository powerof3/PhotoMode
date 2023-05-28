#pragma once

#include "States.h"

namespace PhotoMode
{
	class Manager : public ISingleton<Manager>
	{
	public:
		static bool GetValid();
		bool        IsActive() const;
		void        Activate();
		void        Deactivate();
		void        ToggleActive();

		void GetOriginalState();
		void Revert(bool a_deactivate = false);
		void RevertTab(std::int32_t a_tabIndex);

		[[nodiscard]] float GetResetHoldDuration() const;
		bool                GetResetAll() const;
		void                DoResetAll(bool a_enable);

		[[nodiscard]] float GetViewRoll(float a_fallback) const;

		void Draw();
		void OnFrameUpdate() const;

	private:
		void DrawControls();
		void DrawBar() const;

		// kMenu | kActivate | kFighting | kJumping | kConsole | kSneaking
		static constexpr auto controlFlags = static_cast<RE::ControlMap::UEFlag>(1244);

		static constexpr std::array tabEnum = { "CAMERA"sv, "TIME"sv, "PLAYER"sv, "FILTERS"sv, "SCREENSHOTS"sv };
		static constexpr std::array tabEnumLC = { "camera"sv, "time"sv, "player"sv, "filters"sv, "screenshots"sv };

		bool         activated{ false };
		std::int32_t tabIndex{ 0 };

		float resetAllHoldDuration{ 0.5 };
		bool  resetAll{ false };

		State           originalState{};
		RE::CameraState originalCameraState{ RE::CameraState::kThirdPerson };
		State           currentState{};
		bool            menusAlreadyHidden{ false };
		bool            doResetWindow{ false };

		float timescaleMult{ 1.0f };
		bool  idlePlayed{ false };
		bool  weatherForced{ false };
		bool  effectsPlayed{ false };
		bool  vfxPlayed{ false };
		bool  imodPlayed{ false };

		RE::ImageSpaceBaseData imageSpaceData{};
	};

	void InstallHooks();
}
