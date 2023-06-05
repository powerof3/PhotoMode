#pragma once

#include "Input.h"
#include "States.h"

namespace Input
{
	enum class TYPE;
}

namespace PhotoMode
{
	class Manager : public ISingleton<Manager>
	{
	public:
		void LoadSettings(CSimpleIniA& a_ini);

		static bool GetValid();
		bool        IsActive() const;
		void        Activate();
		void        Deactivate();

		void GetOriginalState();
		void Revert(bool a_deactivate = false);
		void RevertTab(std::int32_t a_tabIndex);

		[[nodiscard]] float GetResetHoldDuration() const;
		bool                GetResetAll() const;
		void                DoResetAll(bool a_enable);

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
		void OnFrameUpdate() const;

	private:
		static void ToggleActive(const KeyCombination*);

		void DrawControls();
		void DrawBar() const;

		// kMenu | kActivate | kFighting | kJumping | kConsole | kSneaking
		static constexpr auto controlFlags = static_cast<RE::ControlMap::UEFlag>(1244);

		static constexpr std::array tabs = { "$PM_Camera", "$PM_TimeWeather", "$PM_Player", "$PM_Filters", "$PM_Screenshots" };
		static constexpr std::array tabResetNotifs = { "$PM_ResetNotifCamera", "$PM_ResetNotifTime", "$PM_ResetNotifPlayer", "$PM_ResetNotifFilters", "$PM_ResetNotifScreenshots" };

		// members
		bool activated{ false };

		std::int32_t previousTab{ 0 };
		std::int32_t currentTab{ 0 };
		bool         updateKeyboardFocus{ false };

		Input::TYPE inputType{};

		struct
		{
			const std::set<std::uint32_t>& GetKeys() const
			{
				if (GetSingleton()->inputType == Input::TYPE::kKeyboard) {
					return keyboard.GetKeys();
				}
				return gamePad.GetKeys();
			}

			KeyCombination keyboard{ "N", ToggleActive };
			KeyCombination gamePad{ "LShoulder+RShoulder", ToggleActive };
		} IO;

		State           originalState{};
		RE::CameraState originalCameraState{ RE::CameraState::kThirdPerson };
		State           currentState{};
		bool            menusAlreadyHidden{ false };
		bool            resetWindow{ true };

		float timescaleMult{ 1.0f };
		bool  idlePlayed{ false };
		bool  weatherForced{ false };
		bool  effectsPlayed{ false };
		bool  vfxPlayed{ false };
		bool  imodPlayed{ false };

		float resetAllHoldDuration{ 0.5 };
		bool  resetAll{ false };

		RE::ImageSpaceBaseData imageSpaceData{};
	};

	void InstallHook();
}
