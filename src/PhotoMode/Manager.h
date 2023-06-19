#pragma once

#include "ImGui/IconsFontAwesome6.h"

#include "Tabs/Camera.h"
#include "Tabs/Filters.h"
#include "Tabs/Player.h"
#include "Tabs/Time.h"

namespace PhotoMode
{
	class Manager :
		public ISingleton<Manager>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static void Register();
		void        LoadMCMSettings(const CSimpleIniA& a_ini);

		static bool GetValid();
		bool        IsActive() const;
		void        Activate();
		void        Deactivate();
		void        ToggleActive();

		void Revert(bool a_deactivate = false);

		bool GetResetAll() const;
		void DoResetAll();

		void NavigateTab(bool a_left);

		[[nodiscard]] float GetViewRoll(float a_fallback) const;

		void Draw();
		bool OnFrameUpdate();

	private:
		enum TAB_TYPE : std::int32_t
		{
			kCamera,
			kTime,
			kPlayer,
			kFilters
		};

		// kMenu | kActivate | kFighting | kJumping | kConsole | kSneaking
		static constexpr auto controlFlags = static_cast<RE::ControlMap::UEFlag>(1244);

		static constexpr std::array tabs = {
			"$PM_Camera",
			"$PM_TimeWeather",
			"$PM_Player",
			"$PM_Filters"
		};
		static constexpr std::array tabIcons = {
			ICON_FA_CAMERA,
			ICON_FA_CLOCK,
			ICON_FA_PERSON,
			ICON_FA_CIRCLE_HALF_STROKE
		};
		static constexpr std::array tabResetNotifs = { "$PM_ResetNotifCamera", "$PM_ResetNotifTime", "$PM_ResetNotifPlayer", "$PM_ResetNotifFilters" };

		void DrawControls();
		void DrawBar() const;

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		// members
		bool activated{ false };

		std::int32_t previousTab{ kCamera };
		std::int32_t currentTab{ kCamera };

		Camera  cameraTab;
		Time    timeTab;
		Player  playerTab;
		Filters filterTab;

		bool updateKeyboardFocus{ false };

		RE::CameraState cameraState{ RE::CameraState::kThirdPerson };
		bool            menusAlreadyHidden{ false };
		bool            resetWindow{ true };
		bool            resetAll{ false };

		float freeCameraSpeed{ 4.0f };
		bool  freezeTimeOnStart{ true };
	};
}
