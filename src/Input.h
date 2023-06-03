#pragma once

namespace Input
{
	enum class TYPE
	{
		kKeyboard,
		kGamepadDirectX,  // xbox
		kGamepadOrbis     // ps4
	};

	class Manager final :
		public ISingleton<Manager>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		void LoadSettings(CSimpleIniA& a_ini);

		static void Register();

		TYPE GetInputType() const;

		bool IsScreenshotQueued() const;
		void QueueScreenshot(bool a_forceQueue);
		void OnScreenshotFinish();

	private:
		static ImGuiKey ToImGuiKey(KEY a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_DIRECTX a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_ORBIS a_key);
		void            SendKeyEvent(std::uint32_t a_key, bool a_keyPressed) const;

		void HideMenu(bool a_hide);

		EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;

		// members
		TYPE inputType{};

		struct
		{
			bool  allowMultiScreenshots{ true };
			float keyHeldDuration{ 0.5 };
			bool  queued{ false };
		} screenshots;

		bool menuHidden{ false };
	};
}
