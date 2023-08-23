#pragma once

#include "ImGui/IconsFontAwesome6.h"

#include "Tabs/Camera.h"
#include "Tabs/Character.h"
#include "Tabs/Filters.h"
#include "Tabs/Overlays.h"
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

		void                                  OnDataLoad();
		std::pair<Texture::ImageData*, float> GetOverlay() const;

	private:
		enum TAB_TYPE : std::int32_t
		{
			kCamera,
			kTime,
			kCharacter,
			kFilters,
			kOverlays
		};

		// kMenu | kActivate | kJumping
		static constexpr auto controlFlags = static_cast<RE::ControlMap::UEFlag>(1036);

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

		static void        TogglePlayerControls(bool a_enable);
		void               DrawControls();
		void               DrawBar() const;
		[[nodiscard]] bool SetupJournalMenu() const;

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		// members
		bool activated{ false };
		bool hiddenUI{ false };
		bool revertENB{ false };

		std::int32_t previousTab{ kCamera };
		std::int32_t currentTab{ kCamera };

		Camera cameraTab;
		Time   timeTab;

		Map<RE::FormID, Character> characterTab;
		RE::Actor*                 cachedCharacter{ nullptr };
		RE::Actor*                 prevCachedCharacter{ nullptr };

		Filters  filterTab;
		Overlays overlaysTab;

		bool updateKeyboardFocus{ false };

		RE::CameraState originalcameraState{ RE::CameraState::kThirdPerson };

		bool resetWindow{ true };
		bool resetPlayerTabs{ true };
		bool resetAll{ false };

		bool menusAlreadyHidden{ false };
		bool allowTextInput{ false };

		bool    noItemsFocused{ false };
		ImGuiID lastFocusedID{ 0 };
		bool    restoreLastFocusID{ false };

		float freeCameraSpeed{ 4.0f };
		bool  freezeTimeOnStart{ true };
		bool  openFromPauseMenu{ true };

		RE::TESGlobal* activeGlobal{ nullptr };
	};
}
