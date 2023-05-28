#pragma once

namespace Input
{
	using EventResult = RE::BSEventNotifyControl;

	class Manager final :
		public ISingleton<Manager>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		void LoadSettings(CSimpleIniA& a_ini);

		static void Register();

		bool IsScreenshotQueued() const;
		void QueueScreenshot(bool a_forceQueue);
		void OnScreenshotFinish();

	private:
		using KEY = RE::BSWin32KeyboardDevice::Key;
		using GAMEPAD_KEY = RE::BSWin32GamepadDevice::Key;

		static void ToggleActivate(const KeyCombination*);

		static void     SendKeyEvent(RE::INPUT_DEVICE a_device, std::uint32_t a_key, bool a_keyPressed);
		static ImGuiKey ToImGuiKey(KEY a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_KEY a_key);

		static std::uint32_t ResetKey(RE::INPUT_DEVICE a_device);
		static std::uint32_t ToggleMenuKey(RE::INPUT_DEVICE a_device);

		void HideMenu(bool a_hide);

		EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;

		// members
		struct
		{
			KeyCombination keyboard{ "N", ToggleActivate };
			KeyCombination gamePad{ "LShoulder+RShoulder", ToggleActivate };
		} photomodeIO;

		struct
		{
			bool  allowMultiScreenshots{ true };
			float keyHeldDuration{ 0.5 };
			bool  queued{ false };
		} screenshots;

		bool menuHidden{ false };
	};
}
