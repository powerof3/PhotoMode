#include "Input.h"
#include "PhotoMode/Manager.h"

namespace Input
{
	void Manager::LoadSettings(CSimpleIniA& a_ini)
	{
		std::string kPattern{ photomodeIO.keyboard.GetPattern() };
		ini::get_value(a_ini, kPattern, "PhotoMode", "HotKey", ";Toggle photomode\n\n;Default is N\n;DXScanCodes : https://www.creationkit.com/index.php?title=Input_Script");
		photomodeIO.keyboard.SetPattern(kPattern);

		std::string gPattern{ photomodeIO.gamePad.GetPattern() };
		ini::get_value(a_ini, gPattern, "PhotoMode", "GamepadKey", ";Default is LShoulder+RShoulder");
		photomodeIO.gamePad.SetPattern(gPattern);

		ini::get_value(a_ini, screenshots.allowMultiScreenshots, "MultiScreenshots", "Enable", ";Allow multi-screenshots by holding down the PrintScrn key");
		ini::get_value(a_ini, screenshots.keyHeldDuration, "MultiScreenshots", "PrintScrnHoldDuration", ";How long should PrintScrn be held down (in seconds).");
	}

	void Manager::Register()
	{
		logger::info("{:*^30}", "EVENTS");

		if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
			inputMgr->AddEventSink<RE::InputEvent*>(GetSingleton());
			logger::info("Registered for hotkey event");
		}
	}

	bool Manager::IsScreenshotQueued() const
	{
		return screenshots.queued;
	}

	void Manager::QueueScreenshot(bool a_forceQueue)
	{
		screenshots.queued = true;

		if (a_forceQueue) {
			(void)RE::MenuControls::GetSingleton()->QueueScreenshot();
		}
	}

	void Manager::OnScreenshotFinish()
	{
		screenshots.queued = false;
	}

	void Manager::ToggleActivate(const KeyCombination*)
	{
		PhotoMode::Manager::GetSingleton()->ToggleActive();
	}

	void Manager::SendKeyEvent(RE::INPUT_DEVICE a_device, std::uint32_t a_key, bool a_keyPressed)
	{
		ImGuiKey key;

		switch (a_device) {
		case RE::INPUT_DEVICE::kKeyboard:
			key = ToImGuiKey(static_cast<RE::BSWin32KeyboardDevice::Key>(a_key));
			break;
		case RE::INPUT_DEVICE::kGamepad:
			key = ToImGuiKey(static_cast<RE::BSWin32GamepadDevice::Key>(a_key));
			break;
		default:
			return;
		}

		ImGui::GetIO().AddKeyEvent(key, a_keyPressed);
	}

	ImGuiKey Manager::ToImGuiKey(RE::BSWin32KeyboardDevice::Key a_key)
	{
		switch (a_key) {
		case KEY::kTab:
			return ImGuiKey_Tab;
		case KEY::kLeft:
			return ImGuiKey_LeftArrow;
		case KEY::kRight:
			return ImGuiKey_RightArrow;
		case KEY::kUp:
			return ImGuiKey_UpArrow;
		case KEY::kDown:
			return ImGuiKey_DownArrow;
		case KEY::kPageUp:
			return ImGuiKey_PageUp;
		case KEY::kPageDown:
			return ImGuiKey_PageDown;
		case KEY::kHome:
			return ImGuiKey_Home;
		case KEY::kEnd:
			return ImGuiKey_End;
		case KEY::kInsert:
			return ImGuiKey_Insert;
		case KEY::kDelete:
			return ImGuiKey_Delete;
		case KEY::kBackspace:
			return ImGuiKey_Backspace;
		case KEY::kSpacebar:
			return ImGuiKey_Space;
		case KEY::kEnter:
			return ImGuiKey_Enter;
		case KEY::kEscape:
			return ImGuiKey_Escape;
		case KEY::kApostrophe:
			return ImGuiKey_Apostrophe;
		case KEY::kComma:
			return ImGuiKey_Comma;
		case KEY::kMinus:
			return ImGuiKey_Minus;
		case KEY::kPeriod:
			return ImGuiKey_Period;
		case KEY::kSlash:
			return ImGuiKey_Slash;
		case KEY::kSemicolon:
			return ImGuiKey_Semicolon;
		case KEY::kEquals:
			return ImGuiKey_Equal;
		case KEY::kBracketLeft:
			return ImGuiKey_LeftBracket;
		case KEY::kBackslash:
			return ImGuiKey_Backslash;
		case KEY::kBracketRight:
			return ImGuiKey_RightBracket;
		case KEY::kTilde:
			return ImGuiKey_GraveAccent;
		case KEY::kCapsLock:
			return ImGuiKey_CapsLock;
		case KEY::kScrollLock:
			return ImGuiKey_ScrollLock;
		case KEY::kNumLock:
			return ImGuiKey_NumLock;
		case KEY::kPrintScreen:
			return ImGuiKey_PrintScreen;
		case KEY::kPause:
			return ImGuiKey_Pause;
		case KEY::kKP_0:
			return ImGuiKey_Keypad0;
		case KEY::kKP_1:
			return ImGuiKey_Keypad1;
		case KEY::kKP_2:
			return ImGuiKey_Keypad2;
		case KEY::kKP_3:
			return ImGuiKey_Keypad3;
		case KEY::kKP_4:
			return ImGuiKey_Keypad4;
		case KEY::kKP_5:
			return ImGuiKey_Keypad5;
		case KEY::kKP_6:
			return ImGuiKey_Keypad6;
		case KEY::kKP_7:
			return ImGuiKey_Keypad7;
		case KEY::kKP_8:
			return ImGuiKey_Keypad8;
		case KEY::kKP_9:
			return ImGuiKey_Keypad9;
		case KEY::kKP_Decimal:
			return ImGuiKey_KeypadDecimal;
		case KEY::kKP_Divide:
			return ImGuiKey_KeypadDivide;
		case KEY::kKP_Multiply:
			return ImGuiKey_KeypadMultiply;
		case KEY::kKP_Subtract:
			return ImGuiKey_KeypadSubtract;
		case KEY::kKP_Plus:
			return ImGuiKey_KeypadAdd;
		case KEY::kKP_Enter:
			return ImGuiKey_KeypadEnter;
		case KEY::kLeftShift:
			return ImGuiKey_LeftShift;
		case KEY::kLeftControl:
			return ImGuiKey_LeftCtrl;
		case KEY::kLeftAlt:
			return ImGuiKey_LeftAlt;
		case KEY::kLeftWin:
			return ImGuiKey_LeftSuper;
		case KEY::kRightShift:
			return ImGuiKey_RightShift;
		case KEY::kRightControl:
			return ImGuiKey_RightCtrl;
		case KEY::kRightAlt:
			return ImGuiKey_RightAlt;
		case KEY::kRightWin:
			return ImGuiKey_RightSuper;
		/*case KEY::kAPPS:
			return ImGuiKey_Menu; - doesn't fire*/
		case KEY::kNum0:
			return ImGuiKey_0;
		case KEY::kNum1:
			return ImGuiKey_1;
		case KEY::kNum2:
			return ImGuiKey_2;
		case KEY::kNum3:
			return ImGuiKey_3;
		case KEY::kNum4:
			return ImGuiKey_4;
		case KEY::kNum5:
			return ImGuiKey_5;
		case KEY::kNum6:
			return ImGuiKey_6;
		case KEY::kNum7:
			return ImGuiKey_7;
		case KEY::kNum8:
			return ImGuiKey_8;
		case KEY::kNum9:
			return ImGuiKey_9;
		case KEY::kA:
			return ImGuiKey_A;
		case KEY::kB:
			return ImGuiKey_B;
		case KEY::kC:
			return ImGuiKey_C;
		case KEY::kD:
			return ImGuiKey_D;
		case KEY::kE:
			return ImGuiKey_E;
		case KEY::kF:
			return ImGuiKey_F;
		case KEY::kG:
			return ImGuiKey_G;
		case KEY::kH:
			return ImGuiKey_H;
		case KEY::kI:
			return ImGuiKey_I;
		case KEY::kJ:
			return ImGuiKey_J;
		case KEY::kK:
			return ImGuiKey_K;
		case KEY::kL:
			return ImGuiKey_L;
		case KEY::kM:
			return ImGuiKey_M;
		case KEY::kN:
			return ImGuiKey_N;
		case KEY::kO:
			return ImGuiKey_O;
		case KEY::kP:
			return ImGuiKey_P;
		case KEY::kQ:
			return ImGuiKey_Q;
		case KEY::kR:
			return ImGuiKey_R;
		case KEY::kS:
			return ImGuiKey_S;
		case KEY::kT:
			return ImGuiKey_T;
		case KEY::kU:
			return ImGuiKey_U;
		case KEY::kV:
			return ImGuiKey_V;
		case KEY::kW:
			return ImGuiKey_W;
		case KEY::kX:
			return ImGuiKey_X;
		case KEY::kY:
			return ImGuiKey_Y;
		case KEY::kZ:
			return ImGuiKey_Z;
		case KEY::kF1:
			return ImGuiKey_F1;
		case KEY::kF2:
			return ImGuiKey_F2;
		case KEY::kF3:
			return ImGuiKey_F3;
		case KEY::kF4:
			return ImGuiKey_F4;
		case KEY::kF5:
			return ImGuiKey_F5;
		case KEY::kF6:
			return ImGuiKey_F6;
		case KEY::kF7:
			return ImGuiKey_F7;
		case KEY::kF8:
			return ImGuiKey_F8;
		case KEY::kF9:
			return ImGuiKey_F9;
		case KEY::kF10:
			return ImGuiKey_F10;
		case KEY::kF11:
			return ImGuiKey_F11;
		case KEY::kF12:
			return ImGuiKey_F12;
		default:
			return ImGuiKey_None;
		}
	}

	ImGuiKey Manager::ToImGuiKey(RE::BSWin32GamepadDevice::Key a_key)
	{
		switch (a_key) {
		case GAMEPAD_KEY::kUp:
			return ImGuiKey_GamepadDpadUp;
		case GAMEPAD_KEY::kDown:
			return ImGuiKey_GamepadDpadDown;
		case GAMEPAD_KEY::kLeft:
			return ImGuiKey_GamepadDpadLeft;
		case GAMEPAD_KEY::kRight:
			return ImGuiKey_GamepadDpadRight;
		case GAMEPAD_KEY::kStart:
			return ImGuiKey_GamepadStart;
		case GAMEPAD_KEY::kBack:
			return ImGuiKey_GamepadBack;
		case GAMEPAD_KEY::kLeftThumb:
			return ImGuiKey_GamepadL3;
		case GAMEPAD_KEY::kRightThumb:
			return ImGuiKey_GamepadR3;
		case GAMEPAD_KEY::kLeftShoulder:
			return ImGuiKey_GamepadL1;
		case GAMEPAD_KEY::kRightShoulder:
			return ImGuiKey_GamepadR1;
		case GAMEPAD_KEY::kA:
			return ImGuiKey_GamepadFaceDown;
		case GAMEPAD_KEY::kB:
			return ImGuiKey_GamepadFaceRight;
		case GAMEPAD_KEY::kX:
			return ImGuiKey_GamepadFaceLeft;
		case GAMEPAD_KEY::kY:
			return ImGuiKey_GamepadFaceUp;
		default:
			return ImGuiKey_None;
		}
	}

	std::uint32_t Manager::ResetKey(RE::INPUT_DEVICE a_device)
	{
		switch (a_device) {
		case RE::INPUT_DEVICE::kKeyboard:
			return RE::BSWin32KeyboardDevice::Key::kR;
		case RE::INPUT_DEVICE::kGamepad:
			return RE::BSWin32GamepadDevice::Key::kY;
		default:
			return 0;
		}
	}

	std::uint32_t Manager::ToggleMenuKey(RE::INPUT_DEVICE a_device)
	{
		switch (a_device) {
		case RE::INPUT_DEVICE::kKeyboard:
			return RE::BSWin32KeyboardDevice::Key::kT;
		case RE::INPUT_DEVICE::kGamepad:
			return RE::BSWin32GamepadDevice::Key::kX;
		default:
			return 0;
		}
	}

	void Manager::HideMenu(bool a_hide)
	{
		if (a_hide && RE::UI::GetSingleton()->IsShowingMenus()) {
			RE::UI::GetSingleton()->ShowMenus(false);
			menuHidden = true;
		}
		if (!a_hide && menuHidden) {
			RE::UI::GetSingleton()->ShowMenus(true);
			menuHidden = false;
		}
	}

	EventResult Manager::ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (!a_evn || RE::UI::GetSingleton()->IsMenuOpen(RE::Console::MENU_NAME)) {
			return EventResult::kContinue;
		}

		auto&      io = ImGui::GetIO();
		const auto photoMode = get<PhotoMode::Manager>();
		const auto deviceMgr = get<RE::BSInputDeviceManager>();

		photomodeIO.keyboard.Process(a_evn);
		if (deviceMgr->IsGamepadEnabled()) {
			photomodeIO.gamePad.Process(a_evn);
		}

		for (auto event = *a_evn; event; event = event->next) {
			if (const auto charEvent = event->AsCharEvent()) {
				if (photoMode->IsActive()) {
					io.AddInputCharacter(charEvent->keycode);
				}
			} else if (const auto buttonEvent = event->AsButtonEvent()) {
				const auto key = buttonEvent->GetIDCode();
				const auto device = event->GetDevice();

				if (!photoMode->IsActive()) {
					if (buttonEvent->QUserEvent() == RE::UserEvents::GetSingleton()->screenshot) {
						if (screenshots.allowMultiScreenshots && buttonEvent->HeldDuration() > screenshots.keyHeldDuration) {
							RE::MenuControls::GetSingleton()->QueueScreenshot();
						}
					}
				} else {
					if (buttonEvent->QUserEvent() == RE::UserEvents::GetSingleton()->screenshot) {
						if (buttonEvent->IsDown()) {
							QueueScreenshot(false);
						} else if (screenshots.allowMultiScreenshots && buttonEvent->HeldDuration() > screenshots.keyHeldDuration) {
							QueueScreenshot(true);
						}
					} else if (!io.WantTextInput) {
						if (key == ResetKey(device)) {
							if (buttonEvent->IsUp()) {
								photoMode->Revert(false);
							} else if (buttonEvent->HeldDuration() > photoMode->GetResetHoldDuration()) {
								photoMode->DoResetAll(true);
							}
						} else if (key == ToggleMenuKey(device)) {
							if (buttonEvent->IsDown()) {
								const auto UI = RE::UI::GetSingleton();
								UI->ShowMenus(!UI->IsShowingMenus());
							}
						}
					}
					SendKeyEvent(device, key, buttonEvent->IsPressed());
				}
			}
		}

		return EventResult::kContinue;
	}
}
