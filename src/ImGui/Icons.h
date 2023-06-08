#pragma once

namespace Input
{
	enum class TYPE : std::uint32_t;
}

namespace Icon
{
	struct ImageData
	{
		ImageData() = default;
		ImageData(std::wstring_view a_iconName);
		~ImageData();

		bool Init();

		// members
		std::wstring              path{ LR"(Data\Interface\Icons\)" };
		ID3D11ShaderResourceView* srView{ nullptr };
		ImVec2                    size{};
	};

	class Manager final : public ISingleton<Manager>
	{
	public:
		void LoadIcons();
		void LoadFonts();

		ImFont* GetBigFont() const;
		ImFont* GetBigIconFont() const;

		const ImageData* GetStepperLeft() const;
		const ImageData* GetStepperRight() const;

		const ImageData*           GetIcon(Input::TYPE a_type, std::uint32_t key);
		std::set<const ImageData*> GetIcons(Input::TYPE a_type, const std::set<std::uint32_t>& keys);

	private:
		// members
		ImFont* bigFont{ nullptr };
		ImFont* bigIconFont{ nullptr };

		ImageData stepperLeft{ ImageData(L"StepperLeft"sv) };
		ImageData stepperRight{ ImageData(L"StepperRight"sv) };

		ImageData unknownKey{ ImageData(L"UnknownKey"sv) };

		Map<KEY, ImageData> keyboard{
			{ KEY::kTab, ImageData(L"Tab"sv) },
			{ KEY::kLeft, ImageData(L"Left"sv) },
			{ KEY::kRight, ImageData(L"Right"sv) },
			{ KEY::kUp, ImageData(L"Up"sv) },
			{ KEY::kDown, ImageData(L"Down"sv) },
			{ KEY::kPageUp, ImageData(L"PgUp"sv) },
			{ KEY::kPageDown, ImageData(L"PgDown"sv) },
			{ KEY::kHome, ImageData(L"Home"sv) },
			{ KEY::kEnd, ImageData(L"End"sv) },
			{ KEY::kInsert, ImageData(L"Insert"sv) },
			{ KEY::kDelete, ImageData(L"Delete"sv) },
			{ KEY::kBackspace, ImageData(L"Backspace"sv) },
			{ KEY::kSpacebar, ImageData(L"Space"sv) },
			{ KEY::kEnter, ImageData(L"Enter"sv) },
			{ KEY::kEscape, ImageData(L"Esc"sv) },
			{ KEY::kLeftControl, ImageData(L"L-Ctrl"sv) },
			{ KEY::kLeftShift, ImageData(L"L-Shift"sv) },
			{ KEY::kLeftAlt, ImageData(L"L-Alt"sv) },
			//{ KEY::kLeftWin, ImageData(L"LeftWin"sv) },
			{ KEY::kRightControl, ImageData(L"R-Ctrl"sv) },
			{ KEY::kRightShift, ImageData(L"R-Shift"sv) },
			{ KEY::kRightAlt, ImageData(L"R-Alt"sv) },
			//{ KEY::kRightWin, ImageData(L"RightWin"sv) },
			{ KEY::kNum0, ImageData(L"0"sv) },
			{ KEY::kNum1, ImageData(L"1"sv) },
			{ KEY::kNum2, ImageData(L"2"sv) },
			{ KEY::kNum3, ImageData(L"3"sv) },
			{ KEY::kNum4, ImageData(L"4"sv) },
			{ KEY::kNum5, ImageData(L"5"sv) },
			{ KEY::kNum6, ImageData(L"6"sv) },
			{ KEY::kNum7, ImageData(L"7"sv) },
			{ KEY::kNum8, ImageData(L"8"sv) },
			{ KEY::kNum9, ImageData(L"9"sv) },
			{ KEY::kA, ImageData(L"A"sv) },
			{ KEY::kB, ImageData(L"B"sv) },
			{ KEY::kC, ImageData(L"C"sv) },
			{ KEY::kD, ImageData(L"D"sv) },
			{ KEY::kE, ImageData(L"E"sv) },
			{ KEY::kF, ImageData(L"F"sv) },
			{ KEY::kG, ImageData(L"G"sv) },
			{ KEY::kH, ImageData(L"H"sv) },
			{ KEY::kI, ImageData(L"I"sv) },
			{ KEY::kJ, ImageData(L"J"sv) },
			{ KEY::kK, ImageData(L"K"sv) },
			{ KEY::kL, ImageData(L"L"sv) },
			{ KEY::kM, ImageData(L"M"sv) },
			{ KEY::kN, ImageData(L"N"sv) },
			{ KEY::kO, ImageData(L"O"sv) },
			{ KEY::kP, ImageData(L"P"sv) },
			{ KEY::kQ, ImageData(L"Q"sv) },
			{ KEY::kR, ImageData(L"R"sv) },
			{ KEY::kS, ImageData(L"S"sv) },
			{ KEY::kT, ImageData(L"T"sv) },
			{ KEY::kU, ImageData(L"U"sv) },
			{ KEY::kV, ImageData(L"V"sv) },
			{ KEY::kW, ImageData(L"W"sv) },
			{ KEY::kX, ImageData(L"X"sv) },
			{ KEY::kY, ImageData(L"Y"sv) },
			{ KEY::kZ, ImageData(L"Z"sv) },
			{ KEY::kF1, ImageData(L"F1"sv) },
			{ KEY::kF2, ImageData(L"F2"sv) },
			{ KEY::kF3, ImageData(L"F3"sv) },
			{ KEY::kF4, ImageData(L"F4"sv) },
			{ KEY::kF5, ImageData(L"F5"sv) },
			{ KEY::kF6, ImageData(L"F6"sv) },
			{ KEY::kF7, ImageData(L"F7"sv) },
			{ KEY::kF8, ImageData(L"F8"sv) },
			{ KEY::kF9, ImageData(L"F9"sv) },
			{ KEY::kF10, ImageData(L"F10"sv) },
			{ KEY::kF11, ImageData(L"F11"sv) },
			{ KEY::kF12, ImageData(L"F12"sv) },
			{ KEY::kApostrophe, ImageData(L"Quotesingle"sv) },
			{ KEY::kComma, ImageData(L"Comma"sv) },
			{ KEY::kMinus, ImageData(L"Hyphen"sv) },
			{ KEY::kPeriod, ImageData(L"Period"sv) },
			{ KEY::kSlash, ImageData(L"Slash"sv) },
			{ KEY::kSemicolon, ImageData(L"Semicolon"sv) },
			{ KEY::kEquals, ImageData(L"Equal"sv) },
			{ KEY::kBracketLeft, ImageData(L"Bracketleft"sv) },
			{ KEY::kBackslash, ImageData(L"Backslash"sv) },
			{ KEY::kBracketRight, ImageData(L"Bracketright"sv) },
			{ KEY::kTilde, ImageData(L"Tilde"sv) },
			{ KEY::kCapsLock, ImageData(L"CapsLock"sv) },
			{ KEY::kScrollLock, ImageData(L"ScrollLock"sv) },
			//{ KEY::kNumLock, ImageData(L"NumLock"sv) },
			{ KEY::kPrintScreen, ImageData(L"PrintScreen"sv) },
			{ KEY::kPause, ImageData(L"Pause"sv) },
			{ KEY::kKP_0, ImageData(L"NumPad0"sv) },
			//{ KEY::kKP_1, ImageData(L"Keypad1"sv) },
			//{ KEY::kKP_2, ImageData(L"Keypad2"sv) },
			//{ KEY::kKP_3, ImageData(L"Keypad3"sv) },
			//{ KEY::kKP_4, ImageData(L"Keypad4"sv) },
			//{ KEY::kKP_5, ImageData(L"Keypad5"sv) },
			//{ KEY::kKP_6, ImageData(L"Keypad6"sv) },
			//{ KEY::kKP_7, ImageData(L"Keypad7"sv) },
			//{ KEY::kKP_8, ImageData(L"Keypad8"sv) },
			{ KEY::kKP_9, ImageData(L"NumPad9"sv) },
			{ KEY::kKP_Decimal, ImageData(L"NumPadDec"sv) },
			{ KEY::kKP_Divide, ImageData(L"NumPadDivide"sv) },
			{ KEY::kKP_Multiply, ImageData(L"NumPadMult"sv) },
			{ KEY::kKP_Subtract, ImageData(L"NumPadMinus"sv) },
			{ KEY::kKP_Plus, ImageData(L"NumPadPlus"sv) },
			//{ KEY::kKP_Enter, ImageData(L"KeypadEnter"sv) }
		};
		Map<GAMEPAD_DIRECTX, ImageData> xbox{
			//{ GAMEPAD_DIRECTX::kUp, ImageData(L"Up"sv) },
			//{ GAMEPAD_DIRECTX::kDown, ImageData(L"Down"sv) },
			//{ GAMEPAD_DIRECTX::kLeft, ImageData(L"Left"sv) },
			//{ GAMEPAD_DIRECTX::kRight, ImageData(L"Right"sv) },
			{ GAMEPAD_DIRECTX::kStart, ImageData(L"360_Start"sv) },
			{ GAMEPAD_DIRECTX::kBack, ImageData(L"360_Back"sv) },
			{ GAMEPAD_DIRECTX::kLeftThumb, ImageData(L"360_LS"sv) },
			{ GAMEPAD_DIRECTX::kRightThumb, ImageData(L"360_RS"sv) },
			{ GAMEPAD_DIRECTX::kLeftShoulder, ImageData(L"360_LB"sv) },
			{ GAMEPAD_DIRECTX::kRightShoulder, ImageData(L"360_RB"sv) },
			{ GAMEPAD_DIRECTX::kA, ImageData(L"360_A"sv) },
			{ GAMEPAD_DIRECTX::kB, ImageData(L"360_B"sv) },
			{ GAMEPAD_DIRECTX::kX, ImageData(L"360_X"sv) },
			{ GAMEPAD_DIRECTX::kY, ImageData(L"360_Y"sv) },
			{ GAMEPAD_DIRECTX::kLeftTrigger, ImageData(L"360_LT"sv) },
			{ GAMEPAD_DIRECTX::kRightTrigger, ImageData(L"360_RT"sv) }
		};
		Map<GAMEPAD_ORBIS, ImageData> ps4{
			//{ GAMEPAD_ORBIS::kUp, ImageData(L"Up"sv) },
			//{ GAMEPAD_ORBIS::kDown, ImageData(L"Down"sv) },
			//{ GAMEPAD_ORBIS::kLeft, ImageData(L"Left"sv) },
			//{ GAMEPAD_ORBIS::kRight, ImageData(L"Right"sv) },
			{ GAMEPAD_ORBIS::kPS3_Start, ImageData(L"PS3_Start"sv) },
			{ GAMEPAD_ORBIS::kPS3_Back, ImageData(L"PS3_Back"sv) },
			{ GAMEPAD_ORBIS::kPS3_L3, ImageData(L"PS3_L3"sv) },
			{ GAMEPAD_ORBIS::kPS3_R3, ImageData(L"PS3_R3"sv) },
			{ GAMEPAD_ORBIS::kPS3_LB, ImageData(L"PS3_LB"sv) },
			{ GAMEPAD_ORBIS::kPS3_RB, ImageData(L"PS3_RB"sv) },
			{ GAMEPAD_ORBIS::kPS3_A, ImageData(L"PS3_A"sv) },
			{ GAMEPAD_ORBIS::kPS3_B, ImageData(L"PS3_B"sv) },
			{ GAMEPAD_ORBIS::kPS3_X, ImageData(L"PS3_X"sv) },
			{ GAMEPAD_ORBIS::kPS3_Y, ImageData(L"PS3_Y"sv) },
			{ GAMEPAD_ORBIS::kPS3_LT, ImageData(L"PS3_LT"sv) },
			{ GAMEPAD_ORBIS::kPS3_RT, ImageData(L"PS3_RT"sv) }
		};
	};
}

namespace ImGui
{
	ImVec2 ButtonIcon(Input::TYPE a_type, std::uint32_t a_key);
	void   ButtonIcon(Input::TYPE a_type, const std::set<std::uint32_t>& a_keys);

	ImVec2 ButtonIcon(const Icon::ImageData* a_imageData, bool a_centerIcon);
	void   ButtonIcon(const std::set<const Icon::ImageData*>& a_imageData, bool a_centerIcon);

	void ButtonIconWithLabel(const char* a_text, const Icon::ImageData* a_imageData, bool a_centerIcon);
	void ButtonIconWithLabel(const char* a_text, const std::set<const Icon::ImageData*>& a_imageData, bool a_centerIcon);
}
