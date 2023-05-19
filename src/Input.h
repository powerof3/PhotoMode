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
		using KEY = RE::BSKeyboardDevice::Key;

		static ImGuiKey BSWinKeyToImGuiKey(KEY a_key);
		void            HideMenu(bool a_hide);

		EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;

		// members
		bool  allowMultiScreenshots{ true };
		float keyHeldDuration{ 0.5 };

		bool screenshotQueued{ false };
		bool menuHidden{ false };
	};
}
