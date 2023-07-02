#pragma once

#include "ImGui/IconsFontAwesome6.h"

#include "Tabs/Camera.h"
#include "Tabs/Filters.h"
#include "Tabs/Overlays.h"
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

		static bool        IsValid();
		static bool        ShouldBlockInput();
		[[nodiscard]] bool IsActive() const;
		void               Activate();
		void               Deactivate();
		void               ToggleActive();

		void Revert(bool a_deactivate = false);

		bool GetResetAll() const;
		void DoResetAll();

		[[nodiscard]] bool IsHidden() const;
		void               ToggleUI();

		void NavigateTab(bool a_left);

		[[nodiscard]] float GetViewRoll(float a_fallback) const;

		void Draw();
		bool OnFrameUpdate();

		void UpdateENBParams();
		void RevertENBParams();

		void                                  LoadOverlays();
		std::pair<Texture::ImageData*, float> GetOverlay() const;

	private:
		enum TAB_TYPE : std::int32_t
		{
			kCamera,
			kTime,
			kPlayer,
			kFilters,
			kOverlays
		};

		// kMenu | kActivate | kFighting | kJumping | kConsole | kSneaking
		static constexpr auto controlFlags = static_cast<RE::ControlMap::UEFlag>(1244);

		static constexpr std::array tabs = {
			"$PM_Camera",
			"$PM_TimeWeather",
			"$PM_Player",
			"$PM_Filters",
			"$PM_Overlays"
		};
		static constexpr std::array tabIcons = {
			ICON_FA_CAMERA,
			ICON_FA_CLOCK,
			ICON_FA_PERSON,
			ICON_FA_CIRCLE_HALF_STROKE,
			ICON_FA_IMAGE
		};
		static constexpr std::array tabResetNotifs = { "$PM_ResetNotifCamera", "$PM_ResetNotifTime", "$PM_ResetNotifPlayer", "$PM_ResetNotifFilters", "$PM_ResetNotifOverlays" };

		void        DrawControls();
		void        DrawBar() const;
		static bool SetupJournalMenu();

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		// members
		bool activated{ false };
		bool hiddenUI{ false };
		bool revertENB{ false };

		std::int32_t previousTab{ kCamera };
		std::int32_t currentTab{ kCamera };

		Camera   cameraTab;
		Time     timeTab;
		Player   playerTab;
		Filters  filterTab;
		Overlays overlaysTab;

		bool updateKeyboardFocus{ false };

		RE::CameraState cameraState{ RE::CameraState::kThirdPerson };

		bool resetWindow{ true };
		bool resetPlayerTabs{ true };
		bool resetAll{ false };

		bool menusAlreadyHidden{ false };
		bool openWithJournalMenu{ false };
		bool allowTextInput{ false };

		bool    noItemsFocused{ false };
		ImGuiID lastFocusedID{ 0 };
		bool    restoreLastFocusID{ false };

		float freeCameraSpeed{ 4.0f };
		bool  freezeTimeOnStart{ true };
	};
}
