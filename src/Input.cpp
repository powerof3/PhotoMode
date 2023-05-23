#include "Input.h"
#include "PhotoMode/Manager.h"

namespace Input
{
	void Manager::LoadSettings(CSimpleIniA& a_ini)
	{
		ini::get_value(a_ini, allowMultiScreenshots, "MultiScreenshots", "Enable", ";Allow multi-screenshots by holding down the PrintScn key");
		ini::get_value(a_ini, keyHeldDuration, "MultiScreenshots", "PrintScnHoldDuration", ";How long should PrintScn be held down before(in seconds).");
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
		return screenshotQueued;
	}

	void Manager::QueueScreenshot(bool a_forceQueue)
	{
		screenshotQueued = true;
		//HideMenu(true);

		if (a_forceQueue) {
			(void)RE::MenuControls::GetSingleton()->QueueScreenshot();
		}
	}

	void Manager::OnScreenshotFinish()
	{
		screenshotQueued = false;
		//HideMenu(false);
	}

	ImGuiKey Manager::BSWinKeyToImGuiKey(KEY a_key)
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

		for (auto event = *a_evn; event; event = event->next) {
		    if (event->eventType == RE::INPUT_EVENT_TYPE::kChar) {
				if (photoMode->IsActive()) {
					io.AddInputCharacter(static_cast<RE::CharEvent*>(event)->keycode);
				}
			} else if (const auto button = event->AsButtonEvent()) {
				const auto key = button->GetIDCode();

				// toggle key
				if (key == photoMode->GetHotKey() && button->IsDown()) {
					photoMode->ToggleActive();
				}

				if (!photoMode->IsActive()) {
					if (button->QUserEvent() == RE::UserEvents::GetSingleton()->screenshot) {
						if (allowMultiScreenshots && button->HeldDuration() > keyHeldDuration) {
							RE::MenuControls::GetSingleton()->QueueScreenshot();
						}
					}
				} else {
					switch (event->GetDevice()) {
					case RE::INPUT_DEVICE::kKeyboard:
						{
							io.AddKeyEvent(BSWinKeyToImGuiKey(static_cast<KEY>(key)), button->IsPressed());
							if (button->QUserEvent() == RE::UserEvents::GetSingleton()->screenshot) {
								if (button->IsDown()) {
									QueueScreenshot(false);
								} else if (allowMultiScreenshots && button->HeldDuration() > keyHeldDuration) {
									QueueScreenshot(true);
								}
							} else if (key == KEY::kR && button->IsDown()) {
								photoMode->Revert(false);
							} else if (key == KEY::kT && button->IsDown()) {
								const auto UI = RE::UI::GetSingleton();
								UI->ShowMenus(!UI->IsShowingMenus());
							}
						}
						break;
						/*case RE::INPUT_DEVICE::kMouse:
						{
							if (key < ImGuiMouseButton_COUNT) {
								io.AddMouseButtonEvent(key, button->IsPressed());
							} else if (key == RE::BSWin32MouseDevice::Keys::kWheelUp) {
								io.AddMouseWheelEvent(0, button->Value());
							} else if (key == RE::BSWin32MouseDevice::Keys::kWheelDown) {
								io.AddMouseWheelEvent(0, -button->Value());
							}
						}
						break;*/
					case RE::INPUT_DEVICE::kGamepad:
						break;
					default:
						break;
					}
				}
			}
		}

		return EventResult::kContinue;
	}
}
