#pragma once

namespace Input
{
	enum class DEVICE
	{
		kNone,
		kKeyboard,
		kMouse,
		kGamepadDirectX,  // xbox
		kGamepadOrbis     // ps4
	};

	class Manager : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static Manager* GetSingleton()
		{
			return &instance;
		}

		static void Register();
		void        LoadMCMSettings(const CSimpleIniA& a_ini);

		DEVICE GetInputDevice() const;
		bool   IsInputKBM() const;
		bool   IsInputGamepad() const;
		bool   CanNavigateWithMouse() const;
		bool   DoNavigateWithMouse() const { return navigateWithMouse; }

		void          LoadDefaultKeys();
		std::uint32_t GetDefaultScreenshotKey() const;

		void HideMenu(bool a_hide);
		bool IsScreenshotQueued() const;
		void QueueScreenshot(bool a_forceQueue);
		void OnScreenshotFinish();

		static void ToggleCursor(bool a_enable);

	private:
		bool                             SetInputDevice(RE::INPUT_DEVICE a_device);
		bool                             GetHotKey(RE::INPUT_DEVICE a_device, std::uint32_t& a_hotkey) const;
		bool                             TiltCamera(const RE::ButtonEvent* a_buttonEvent, std::uint32_t a_key) const;
		static ImGuiKey                  ToImGuiKey(KEY a_key);
		static std::pair<ImGuiKey, bool> ToImGuiKey(GAMEPAD_DIRECTX a_key);
		static std::pair<ImGuiKey, bool> ToImGuiKey(GAMEPAD_ORBIS a_key);
		void                             SendKeyEvent(std::uint32_t a_key, float a_value, bool a_keyPressed) const;

		EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;

		// members
		DEVICE        inputDevice{ DEVICE::kNone };
		DEVICE        lastInputDevice{ DEVICE::kNone };	
		bool          screenshotQueued{ false };
		bool          menuHidden{ false };
		bool          panCamera{ false };
		std::uint32_t screenshotKeyboard{ 0 };
		std::uint32_t screenshotMouse{ 0 };
		std::uint32_t screenshotGamepad{ 0 };
		float         keyHeldDuration{ 0.5 };
		bool          navigateWithMouse{ true };
		bool          cursorInit{ false };

		static Manager instance;
	};

	inline constinit Manager Manager::instance;
}
