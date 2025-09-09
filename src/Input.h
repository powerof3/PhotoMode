#pragma once

namespace Input
{
	enum class DEVICE
	{
		kKeyboard,
		kMouse,
		kGamepadDirectX,  // xbox
		kGamepadOrbis     // ps4
	};

	class Manager final :
		public REX::Singleton<Manager>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static void Register();
		void        LoadMCMSettings(const CSimpleIniA& a_ini);

		DEVICE GetInputDevice() const;

		void          LoadDefaultKeys();
		std::uint32_t GetDefaultScreenshotKey() const;

		void HideMenu(bool a_hide);
		bool IsScreenshotQueued() const;
		void QueueScreenshot(bool a_forceQueue);
		void OnScreenshotFinish();

		static void ToggleCursor(bool a_enable);

	private:
		bool            SetInputDevice(RE::INPUT_DEVICE a_device, std::uint32_t& a_hotkey);
		bool            TiltWithMouse(const RE::ButtonEvent* a_buttonEvent, std::uint32_t a_key) const;
		static ImGuiKey ToImGuiKey(KEY a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_DIRECTX a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_ORBIS a_key);
		void            SendKeyEvent(std::uint32_t a_key, float a_value, bool a_keyPressed) const;

		EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;

		// members
		bool screenshotQueued{ false };
		bool menuHidden{ false };
		bool panCamera{ false };

		float keyHeldDuration{ 0.5 };

		std::uint32_t screenshotKeyboard{ 0 };
		std::uint32_t screenshotMouse{ 0 };
		std::uint32_t screenshotGamepad{ 0 };

		DEVICE inputDevice{ DEVICE::kKeyboard };
	};
}
