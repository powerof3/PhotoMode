#pragma once

namespace Input
{
	enum class TYPE : std::uint32_t
	{
		kKeyboard,
		kGamepadDirectX,  // xbox
		kGamepadOrbis     // ps4
	};

	inline TYPE inputType;
	TYPE        GetInputType();

	class Manager final :
		public ISingleton<Manager>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static void Register();
		void        LoadMCMSettings(const CSimpleIniA& a_ini);

		void         LoadDefaultKeys();
		std::uint32_t GetDefaultScreenshotKey(RE::INPUT_DEVICE a_device) const;

		void HideMenu(bool a_hide);
		bool IsScreenshotQueued() const;
		void QueueScreenshot(bool a_forceQueue);
		void OnScreenshotFinish();

	private:
		static ImGuiKey ToImGuiKey(KEY a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_DIRECTX a_key);
		static ImGuiKey ToImGuiKey(GAMEPAD_ORBIS a_key);
		void            SendKeyEvent(std::uint32_t a_key, bool a_keyPressed) const;

		EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;

		// members
		bool screenshotQueued{ false };
		bool menuHidden{ false };

		float keyHeldDuration{ 0.5 };

		std::uint32_t screenshotKeyboard{ 0 };
		std::uint32_t screenshotMouse{ 0 };
		std::uint32_t screenshotGamepad{ 0 };
	};
}
