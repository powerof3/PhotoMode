#include "Input.h"

#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"

namespace Input
{
	DEVICE Manager::GetInputDevice() const
	{
		return inputDevice;
	}

	bool Manager::IsInputKBM() const
	{
		return inputDevice == DEVICE::kKeyboard || inputDevice == DEVICE::kMouse;
	}

	bool Manager::IsInputGamepad() const
	{
		return inputDevice == DEVICE::kGamepadDirectX || inputDevice == DEVICE::kGamepadOrbis;
	}

	bool Manager::CanNavigateWithMouse() const
	{
		return IsInputKBM() && DoNavigateWithMouse();
	}

	void Manager::Register()
	{
		if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
			inputMgr->AddEventSink<RE::InputEvent*>(GetSingleton());
			logger::info("Registered for hotkey event");
		}
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		keyHeldDuration = static_cast<float>(a_ini.GetDoubleValue("Controls", "iKeyHeldDuration", keyHeldDuration));
		navigateWithMouse = a_ini.GetBoolValue("Controls", "bNavigateWithMouse", navigateWithMouse);
	}

	void Manager::LoadDefaultKeys()
	{
		const auto  controlMap = RE::ControlMap::GetSingleton();
		const auto& screenshot = RE::UserEvents::GetSingleton()->screenshot;

		screenshotKeyboard = controlMap->GetMappedKey(screenshot, RE::INPUT_DEVICE::kKeyboard);
		screenshotMouse = controlMap->GetMappedKey(screenshot, RE::INPUT_DEVICE::kMouse);
		screenshotGamepad = controlMap->GetMappedKey(screenshot, RE::INPUT_DEVICE::kGamepad);
	}

	std::uint32_t Manager::GetDefaultScreenshotKey() const
	{
		std::uint32_t key{ 0 };

		switch (inputDevice) {
		case DEVICE::kKeyboard:
			key = screenshotKeyboard;
			break;
		case DEVICE::kMouse:
			{
				key = screenshotMouse;
				key += SKSE::InputMap::kMacro_MouseButtonOffset;
			}
			break;
		case DEVICE::kGamepadDirectX:
		case DEVICE::kGamepadOrbis:
			key = SKSE::InputMap::GamepadMaskToKeycode(screenshotGamepad);
			break;
		default:
			break;
		}

		return key;
	}

	bool Manager::IsScreenshotQueued() const
	{
		return screenshotQueued;
	}

	void Manager::QueueScreenshot(bool a_forceQueue)
	{
		screenshotQueued = true;

		if (MANAGER(Screenshot)->CanAutoHideMenus()) {
			HideMenu(true);
		}

		if (inputDevice != DEVICE::kKeyboard || a_forceQueue) {
			RE::MenuControls::GetSingleton()->QueueScreenshot();
		}
	}

	void Manager::OnScreenshotFinish()
	{
		if (screenshotQueued) {
			screenshotQueued = false;
			HideMenu(false);
		}
	}

	bool Manager::SetInputDevice(RE::INPUT_DEVICE a_device)
	{
		lastInputDevice = inputDevice;

		// get input type
		switch (a_device) {
		case RE::INPUT_DEVICE::kKeyboard:
			inputDevice = DEVICE::kKeyboard;
			break;
		case RE::INPUT_DEVICE::kMouse:
			inputDevice = DEVICE::kMouse;
			break;
		case RE::INPUT_DEVICE::kGamepad:
			{
				if (RE::ControlMap::GetSingleton()->GetGamePadType() == RE::PC_GAMEPAD_TYPE::kOrbis) {
					inputDevice = DEVICE::kGamepadOrbis;
				} else {
					inputDevice = DEVICE::kGamepadDirectX;
				}
			}
			break;
		default:
			return false;
		}

		return true;
	}

	bool Manager::GetHotKey(RE::INPUT_DEVICE a_device, std::uint32_t& a_hotkey) const
	{
		// get input type
		switch (a_device) {
		case RE::INPUT_DEVICE::kKeyboard:
			break;
		case RE::INPUT_DEVICE::kMouse:
			a_hotkey += SKSE::InputMap::kMacro_MouseButtonOffset;
			break;
		case RE::INPUT_DEVICE::kGamepad:
			a_hotkey = SKSE::InputMap::GamepadMaskToKeycode(a_hotkey);
			break;
		default:
			return false;
		}

		return true;
	}

	bool Manager::TiltCamera(const RE::ButtonEvent* a_buttonEvent, std::uint32_t a_key) const
	{
		if (!RE::PlayerCamera::GetSingleton()->IsInFreeCameraMode()) {
			return false;
		}

		if (auto device = a_buttonEvent->GetDevice(); device != RE::INPUT_DEVICE::kMouse && device != RE::INPUT_DEVICE::kGamepad) {
			return false;
		}

		if (const auto freeCameraState = static_cast<RE::FreeCameraState*>(RE::PlayerCamera::GetSingleton()->currentState.get())) {
			const auto getKey = [this](std::string_view action, RE::INPUT_DEVICE device) {
				auto key = RE::ControlMap::GetSingleton()->GetMappedKey(action, device, RE::UserEvents::INPUT_CONTEXT_ID::kTFCMode);
				GetHotKey(device, key);
				return key;
			};

			static auto mouseUp = getKey("WorldZUp", RE::INPUT_DEVICE::kMouse);
			static auto mouseDown = getKey("WorldZDown", RE::INPUT_DEVICE::kMouse);
			static auto gamepadUp = getKey("WorldZUp", RE::INPUT_DEVICE::kGamepad);
			static auto gamepadDown = getKey("WorldZDown", RE::INPUT_DEVICE::kGamepad);

			if (a_key == mouseUp || a_key == gamepadUp) {
				bool released = !(a_buttonEvent->value != 0.0 || a_buttonEvent->heldDownSecs < 0.0);
				freeCameraState->verticalDirection = static_cast<std::uint16_t>(!released);
				return true;
			}
			if (a_key == mouseDown || a_key == gamepadDown) {
				if (a_buttonEvent->value == 0.0 && a_buttonEvent->heldDownSecs >= 0.0) {
					freeCameraState->verticalDirection = 0;
				} else {
					freeCameraState->verticalDirection = -1;
				}
				return true;
			}
		}

		return false;
	}

	ImGuiKey Manager::ToImGuiKey(KEY a_key)
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

	std::pair<ImGuiKey, bool> Manager::ToImGuiKey(GAMEPAD_DIRECTX a_key)
	{
		switch (a_key) {
		case GAMEPAD_DIRECTX::kUp:
			return { ImGuiKey_GamepadDpadUp, false };
		case GAMEPAD_DIRECTX::kDown:
			return { ImGuiKey_GamepadDpadDown, false };
		case GAMEPAD_DIRECTX::kLeft:
			return { ImGuiKey_GamepadDpadLeft, false };
		case GAMEPAD_DIRECTX::kRight:
			return { ImGuiKey_GamepadDpadRight, false };
		case GAMEPAD_DIRECTX::kStart:
			return { ImGuiKey_GamepadStart, false };
		case GAMEPAD_DIRECTX::kBack:
			return { ImGuiKey_GamepadBack, false };
		case GAMEPAD_DIRECTX::kLeftThumb:
			return { ImGuiKey_GamepadL3, false };
		case GAMEPAD_DIRECTX::kRightThumb:
			return { ImGuiKey_GamepadR3, false };
		case GAMEPAD_DIRECTX::kLeftShoulder:
			return { ImGuiKey_GamepadL1, false };
		case GAMEPAD_DIRECTX::kRightShoulder:
			return { ImGuiKey_GamepadR1, false };
		case GAMEPAD_DIRECTX::kA:
			return { ImGuiKey_GamepadFaceDown, false };
		case GAMEPAD_DIRECTX::kB:
			return { ImGuiKey_GamepadFaceRight, false };
		case GAMEPAD_DIRECTX::kX:
			return { ImGuiKey_GamepadFaceLeft, false };
		case GAMEPAD_DIRECTX::kY:
			return { ImGuiKey_GamepadFaceUp, false };
		case GAMEPAD_DIRECTX::kLeftTrigger:
			return { ImGuiKey_GamepadL2, true };  // analog
		case GAMEPAD_DIRECTX::kRightTrigger:
			return { ImGuiKey_GamepadR2, true };  // analog
		default:
			return { ImGuiKey_None, false };
		}
	}

	// faking this with keyboard inputs, since ImGUI doesn't support DirectInput
	std::pair<ImGuiKey, bool> Manager::ToImGuiKey(GAMEPAD_ORBIS a_key)
	{
		switch (a_key) {
		// Move / Tweak / Resize Window (in Windowing mode)
		case GAMEPAD_ORBIS::kUp:
			return { ImGuiKey_UpArrow, false };
		// Move / Tweak / Resize Window (in Windowing mode)
		case GAMEPAD_ORBIS::kDown:
			return { ImGuiKey_DownArrow, false };
		// Move / Tweak / Resize Window (in Windowing mode)
		case GAMEPAD_ORBIS::kLeft:
			return { ImGuiKey_LeftArrow, false };
		// Move / Tweak / Resize Window (in Windowing mode)
		case GAMEPAD_ORBIS::kRight:
			return { ImGuiKey_RightArrow, false };

		case GAMEPAD_ORBIS::kPS3_Start:
			return { ImGuiKey_GamepadStart, false };
		case GAMEPAD_ORBIS::kPS3_Back:
			return { ImGuiKey_GamepadBack, false };
		case GAMEPAD_ORBIS::kPS3_L3:
			return { ImGuiKey_GamepadL3, false };
		case GAMEPAD_ORBIS::kPS3_R3:
			return { ImGuiKey_GamepadR3, false };

		// Tweak Slower / Focus Previous (in Windowing mode)
		case GAMEPAD_ORBIS::kPS3_LB:
			return { ImGuiKey_NavKeyboardTweakSlow, false };
		// Tweak Faster / Focus Next (in Windowing mode)
		case GAMEPAD_ORBIS::kPS3_RB:
			return { ImGuiKey_NavKeyboardTweakFast, false };
		// Activate / Open / Toggle / Tweak
		case GAMEPAD_ORBIS::kPS3_A:
			return { ImGuiKey_Enter, false };
		// Cancel / Close / Exit
		case GAMEPAD_ORBIS::kPS3_B:
			return { ImGuiKey_Escape, false };

		case GAMEPAD_ORBIS::kPS3_X:
			return { ImGuiKey_GamepadFaceLeft, false };
		case GAMEPAD_ORBIS::kPS3_Y:
			return { ImGuiKey_GamepadFaceUp, false };

		case GAMEPAD_ORBIS::kPS3_LT:
			return { ImGuiKey_GamepadL2, true };
		case GAMEPAD_ORBIS::kPS3_RT:
			return { ImGuiKey_GamepadR2, true };
		default:
			return { ImGuiKey_None, false };
		}
	}

	void Manager::SendKeyEvent(std::uint32_t a_key, float a_value, bool a_keyPressed) const
	{
		auto& io = ImGui::GetIO();

		if (inputDevice == DEVICE::kMouse) {
			switch (auto mouseKey = static_cast<MOUSE>(a_key)) {
			case MOUSE::kWheelUp:
				{
					io.AddMouseWheelEvent(0, a_value);
					io.AddKeyEvent(ImGuiKey_UpArrow, a_keyPressed);
				}
				break;
			case MOUSE::kWheelDown:
				{
					io.AddMouseWheelEvent(0, a_value * -1);
					io.AddKeyEvent(ImGuiKey_DownArrow, a_keyPressed);
				}
				break;
			default:
				io.AddMouseButtonEvent(mouseKey, a_keyPressed);
				break;
			}
		} else {
			ImGuiKey key{ ImGuiKey_None };
			bool     analog{ false };

			switch (inputDevice) {
			case DEVICE::kKeyboard:
				key = ToImGuiKey(static_cast<KEY>(a_key));
				break;
			case DEVICE::kGamepadDirectX:
				std::tie(key, analog) = ToImGuiKey(static_cast<GAMEPAD_DIRECTX>(a_key));
				break;
			case DEVICE::kGamepadOrbis:
				std::tie(key, analog) = ToImGuiKey(static_cast<GAMEPAD_ORBIS>(a_key));
				break;
			default:
				break;
			}
			if (analog) {
				io.AddKeyAnalogEvent(key, a_value, a_keyPressed);
			} else {
				io.AddKeyEvent(key, a_keyPressed);
			}
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

	void Manager::ToggleCursor(bool a_enable)
	{
		SKSE::GetTaskInterface()->AddUITask([a_enable]() { RE::UIMessageQueue::GetSingleton()->AddMessage(RE::CursorMenu::MENU_NAME,
															   a_enable ?
																   RE::UI_MESSAGE_TYPE::kShow :
																   RE::UI_MESSAGE_TYPE::kHide,
															   nullptr); });
	}

	EventResult Manager::ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (!a_evn || !RE::Main::GetSingleton()->gameActive) {
			return EventResult::kContinue;
		}

		const auto photoMode = MANAGER(PhotoMode);
		const auto hotKeys = MANAGER(PhotoMode::Hotkeys);

		hotKeys->TogglePhotoMode(a_evn);

		if (photoMode->IsActive()) {
			if (photoMode->ShouldBlockInput()) {
				return EventResult::kContinue;
			}

			bool cursorMenuOpen = RE::UI::GetSingleton()->IsMenuOpen(RE::CursorMenu::MENU_NAME);
			bool cursorOverWindow = cursorInit && MANAGER(PhotoMode)->IsCursorHoveringOverWindow();

			for (auto event = *a_evn; event; event = event->next) {
				if (!SetInputDevice(event->GetDevice())) {
					continue;
				}

				auto& io = ImGui::GetIO();

				if (lastInputDevice != inputDevice) {
					io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
					io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

					if (IsInputGamepad()) {
						io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
					} else {
						if (IsInputKBM() && !DoNavigateWithMouse()) {
							io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
						}
					}
				}

				bool canNavigateWithMouse = CanNavigateWithMouse();

				if (canNavigateWithMouse && (!cursorInit || (!cursorMenuOpen && !panCamera))) {
					ToggleCursor(true);
					cursorInit = true;
					MANAGER(PhotoMode)->UpdateKeyboardFocus();
				} else if (IsInputGamepad() && (cursorInit || cursorMenuOpen)) {
					ToggleCursor(false);
					cursorInit = false;
					MANAGER(PhotoMode)->UpdateKeyboardFocus();
				}

				// process inputs
				if (const auto charEvent = event->AsCharEvent()) {
					io.AddInputCharacter(charEvent->keyCode);
				} else if (const auto buttonEvent = event->AsButtonEvent()) {
					const auto key = buttonEvent->GetIDCode();
					auto       hotKey = key;

					if (!GetHotKey(event->GetDevice(), hotKey) || (IsInputGamepad() || !cursorOverWindow) && TiltCamera(buttonEvent, hotKey)) {
						continue;
					}

					if (canNavigateWithMouse && hotKey == hotKeys->PanCameraKey()) {
						if (buttonEvent->IsHeld()) {
							if (!cursorOverWindow) {
								if (!panCamera) {
									ToggleCursor(false);
									panCamera = true;
								}
							}
						} else if (panCamera) {
							ToggleCursor(true);
							panCamera = false;
						}
					}

					if (!io.WantTextInput) {
						if (hotKey == hotKeys->TakePhotoKey()) {
							if (buttonEvent->IsDown()) {
								QueueScreenshot(hotKey != GetDefaultScreenshotKey());
							} else if (MANAGER(Screenshot)->AllowMultiScreenshots() && buttonEvent->HeldDuration() > keyHeldDuration) {
								QueueScreenshot(true);
							}
						} else if (hotKey == hotKeys->ToggleMenusKey()) {
							if (buttonEvent->IsDown()) {
								photoMode->ToggleUI();
							}
						} else if (!photoMode->IsHidden()) {
							if (hotKey == hotKeys->NextTabKey() && buttonEvent->IsDown()) {
								photoMode->NavigateTab(false);
							} else if (hotKey == hotKeys->PreviousTabKey() && buttonEvent->IsDown()) {
								photoMode->NavigateTab(true);
							} else if (hotKey == hotKeys->FreezeTimeKey() && buttonEvent->IsDown()) {
								RE::Main::GetSingleton()->freezeTime = !RE::Main::GetSingleton()->freezeTime;
							} else if (hotKey == hotKeys->ResetKey()) {
								if (buttonEvent->IsUp()) {
									photoMode->Revert(false);
								} else if (buttonEvent->HeldDuration() > keyHeldDuration) {
									photoMode->DoResetAll();
								}
							}
						}
					}

					if (!photoMode->IsHidden() || hotKey == hotKeys->EscapeKey()) {
						if (inputDevice == DEVICE::kKeyboard && hotKey == KEY::kTab) {
							io.AddKeyEvent(ImGuiKey_Tab, buttonEvent->IsDown());
						} else {
							SendKeyEvent(key, buttonEvent->Value(), buttonEvent->IsPressed());
						}
					}
				}
			}
		}

		return EventResult::kContinue;
	}
}
