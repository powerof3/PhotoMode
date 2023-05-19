#pragma once

#include "States.h"

namespace PhotoMode
{
	class Manager : public ISingleton<Manager>
	{
	public:
		void LoadSettings(CSimpleIniA& a_ini);

		std::uint32_t GetHotKey() const;

		bool        IsActive() const;
		static bool GetValid();

		void Activate();
		void Deactivate();
		void ToggleActive();

		void GetOriginalState();
		void Revert(bool a_deactivate);

		[[nodiscard]] float GetViewRoll(float a_fallback) const;

		void Draw();

	private:
		void DrawControls();
		void DrawGrid() const;
		void DrawBar() const;

		// kMenu | kActivate | kFighting | kJumping | kConsole | kSneaking
		static constexpr auto controlFlags = static_cast<RE::ControlMap::UEFlag>(1244);

		bool activated{ false };

		// controls
		std::uint32_t hotKey{ 49 };  // N

		// current state
		State currentState{};

		// state before photomode
		State           originalState{};
		RE::CameraState originalCameraState{ RE::CameraState::kThirdPerson };
		bool            menuAlreadyHidden{ false };
		bool            doResetWindow{};

		float timescaleMult{ 1.0f };

		bool idlePlayed{ false };
		bool weatherForced{ false };
		bool effectsPlayed{ false };

		RE::ImageSpaceBaseData imageSpaceData{};
	};

	void InstallHooks();
}
