#pragma once

#include "ImGui/IconsFontAwesome6.h"

#include "Camera.h"
#include "Filters.h"
#include "Input.h"
#include "Player.h"
#include "Time.h"

namespace Input
{
	enum class TYPE : std::uint32_t;
}

namespace PhotoMode
{
	class Manager :
		public ISingleton<Manager>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static void Register();

		void LoadSettings(CSimpleIniA& a_ini);
		void SaveSettings(CSimpleIniA& a_ini) const;

		static bool GetValid();
		bool        IsActive() const;
		void        Activate();
		void        Deactivate();
		void        ToggleActive();

		void Revert(bool a_deactivate = false);

		[[nodiscard]] static float GetResetHoldDuration();
		bool                       GetResetAll() const;
		void                       DoResetAll();

		void SetInputType(Input::TYPE a_inputType);

		std::uint32_t ResetKey() const;
		std::uint32_t TakePhotoKey() const;
		std::uint32_t ToggleUIKey() const;
		std::uint32_t RightTabKey() const;
		std::uint32_t LeftTabKey() const;

		void NavigateTab(bool a_left);

		void CheckActive(RE::InputEvent* const* a_event);

		[[nodiscard]] float GetViewRoll(float a_fallback) const;

		void Draw();
		bool OnFrameUpdate();

	private:
		enum TAB_TYPE : std::int32_t
		{
			kCamera,
			kTime,
			kPlayer,
			kFilters,
			kSettings
		};

		// kMenu | kActivate | kFighting | kJumping | kConsole | kSneaking
		static constexpr auto controlFlags = static_cast<RE::ControlMap::UEFlag>(1244);

		static constexpr std::array tabs = {
		    "$PM_Camera",
		    "$PM_TimeWeather",
		    "$PM_Player",
		    "$PM_Filters",
		    "$PM_Settings"
		};
		static constexpr std::array tabIcons = {
		    ICON_FA_CAMERA,
		    ICON_FA_CLOCK,
		    ICON_FA_PERSON,
		    ICON_FA_CIRCLE_HALF_STROKE,
		    ICON_FA_GEAR
		};

	    static constexpr std::array tabResetNotifs = { "$PM_ResetNotifCamera", "$PM_ResetNotifTime", "$PM_ResetNotifPlayer", "$PM_ResetNotifFilters", "$PM_ResetNotifSettings" };

		static void ToggleActive_Input(const KeyCombination*);

		void DrawControls();
		void DrawBar() const;

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		// members
		bool activated{ false };

		struct
		{
			const std::set<std::uint32_t>& GetKeys() const
			{
				if (GetSingleton()->inputType == Input::TYPE::kKeyboard) {
					return keyboard.GetKeys();
				}
				return gamePad.GetKeys();
			}

			KeyCombination keyboard{ "N", ToggleActive_Input };
			KeyCombination gamePad{ "LShoulder+RShoulder", ToggleActive_Input };
		} IO;

		std::int32_t previousTab{ kCamera };
		std::int32_t currentTab{ kCamera };

		Camera  cameraTab;
		Time    timeTab;
		Player  playerTab;
		Filters filterTab;

		Input::TYPE inputType{};
		bool        updateKeyboardFocus{ false };

		RE::CameraState cameraState{ RE::CameraState::kThirdPerson };
		bool            menusAlreadyHidden{ false };
		bool            resetWindow{ true };

		bool resetAll{ false };
	};

	void InstallHooks();
}
