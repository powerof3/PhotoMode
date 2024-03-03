#pragma once

#include "Graphics.h"

namespace IconFont
{
	struct IconData final : Texture::ImageData
	{
		IconData() = delete;
		IconData(std::wstring_view a_iconName);

		~IconData() override = default;

		bool Load(bool a_resizeToScreenRes = false) override;
		void Resize(float a_scale);

		// members
		ImVec2 imageSize{};
	};

	class Manager final : public ISingleton<Manager>
	{
	public:
		struct GamepadIcon
		{
			IconData xbox;
			IconData ps4;
		};

		void LoadSettings(CSimpleIniA& a_ini);
		void LoadMCMSettings(const CSimpleIniA& a_ini);

		void LoadIcons();
		void ReloadFonts();
		void ResizeIcons();

		ImFont* GetLargeFont() const;

		const IconData* GetStepperLeft() const;
		const IconData* GetStepperRight() const;
		const IconData* GetCheckbox() const;
		const IconData* GetCheckboxFilled() const;

		const IconData*           GetIcon(std::uint32_t key);
		std::set<const IconData*> GetIcons(const std::set<std::uint32_t>& keys);

		const IconData* GetGamePadIcon(const GamepadIcon& a_icons) const;

	private:
		enum class BUTTON_SCHEME
		{
			kAutoDetect,
			kXbox,
			kPS4
		};

		ImFont* LoadFontIconSet(float a_fontSize, float a_iconSize, const ImVector<ImWchar>& a_ranges) const;

		// members
		bool loadedFonts{ false };

		std::string fontName{ "Jost-Regular.ttf" };
		float       fontSize{ 26 };
		float       iconSize{ 20 };
		float       largeFontSize{ 30 };
		float       largeIconSize{ 24 };

		ImFont* largeFont{ nullptr };

		IconData stepperLeft{ L"StepperLeft"sv };
		IconData stepperRight{ L"StepperRight"sv };
		IconData checkbox{ L"Checkbox"sv };
		IconData checkboxFilled{ L"Checkbox-Filled"sv };

		IconData unknownKey{ L"UnknownKey"sv };

		IconData leftKey{ L"Left"sv };
		IconData rightKey{ L"Right"sv };
		IconData upKey{ L"Up"sv };
		IconData downKey{ L"Down"sv };

		Map<KEY, IconData> keyboard{
			{ KEY::kTab, IconData(L"Tab"sv) },
			{ KEY::kPageUp, IconData(L"PgUp"sv) },
			{ KEY::kPageDown, IconData(L"PgDown"sv) },
			{ KEY::kHome, IconData(L"Home"sv) },
			{ KEY::kEnd, IconData(L"End"sv) },
			{ KEY::kInsert, IconData(L"Insert"sv) },
			{ KEY::kDelete, IconData(L"Delete"sv) },
			{ KEY::kBackspace, IconData(L"Backspace"sv) },
			{ KEY::kSpacebar, IconData(L"Space"sv) },
			{ KEY::kEnter, IconData(L"Enter"sv) },
			{ KEY::kEscape, IconData(L"Esc"sv) },
			{ KEY::kLeftControl, IconData(L"L-Ctrl"sv) },
			{ KEY::kLeftShift, IconData(L"L-Shift"sv) },
			{ KEY::kLeftAlt, IconData(L"L-Alt"sv) },
			//{ KEY::kLeftWin, IconData(L"LeftWin"sv) },
			{ KEY::kRightControl, IconData(L"R-Ctrl"sv) },
			{ KEY::kRightShift, IconData(L"R-Shift"sv) },
			{ KEY::kRightAlt, IconData(L"R-Alt"sv) },
			//{ KEY::kRightWin, IconData(L"RightWin"sv) },
			{ KEY::kNum0, IconData(L"0"sv) },
			{ KEY::kNum1, IconData(L"1"sv) },
			{ KEY::kNum2, IconData(L"2"sv) },
			{ KEY::kNum3, IconData(L"3"sv) },
			{ KEY::kNum4, IconData(L"4"sv) },
			{ KEY::kNum5, IconData(L"5"sv) },
			{ KEY::kNum6, IconData(L"6"sv) },
			{ KEY::kNum7, IconData(L"7"sv) },
			{ KEY::kNum8, IconData(L"8"sv) },
			{ KEY::kNum9, IconData(L"9"sv) },
			{ KEY::kA, IconData(L"A"sv) },
			{ KEY::kB, IconData(L"B"sv) },
			{ KEY::kC, IconData(L"C"sv) },
			{ KEY::kD, IconData(L"D"sv) },
			{ KEY::kE, IconData(L"E"sv) },
			{ KEY::kF, IconData(L"F"sv) },
			{ KEY::kG, IconData(L"G"sv) },
			{ KEY::kH, IconData(L"H"sv) },
			{ KEY::kI, IconData(L"I"sv) },
			{ KEY::kJ, IconData(L"J"sv) },
			{ KEY::kK, IconData(L"K"sv) },
			{ KEY::kL, IconData(L"L"sv) },
			{ KEY::kM, IconData(L"M"sv) },
			{ KEY::kN, IconData(L"N"sv) },
			{ KEY::kO, IconData(L"O"sv) },
			{ KEY::kP, IconData(L"P"sv) },
			{ KEY::kQ, IconData(L"Q"sv) },
			{ KEY::kR, IconData(L"R"sv) },
			{ KEY::kS, IconData(L"S"sv) },
			{ KEY::kT, IconData(L"T"sv) },
			{ KEY::kU, IconData(L"U"sv) },
			{ KEY::kV, IconData(L"V"sv) },
			{ KEY::kW, IconData(L"W"sv) },
			{ KEY::kX, IconData(L"X"sv) },
			{ KEY::kY, IconData(L"Y"sv) },
			{ KEY::kZ, IconData(L"Z"sv) },
			{ KEY::kF1, IconData(L"F1"sv) },
			{ KEY::kF2, IconData(L"F2"sv) },
			{ KEY::kF3, IconData(L"F3"sv) },
			{ KEY::kF4, IconData(L"F4"sv) },
			{ KEY::kF5, IconData(L"F5"sv) },
			{ KEY::kF6, IconData(L"F6"sv) },
			{ KEY::kF7, IconData(L"F7"sv) },
			{ KEY::kF8, IconData(L"F8"sv) },
			{ KEY::kF9, IconData(L"F9"sv) },
			{ KEY::kF10, IconData(L"F10"sv) },
			{ KEY::kF11, IconData(L"F11"sv) },
			{ KEY::kF12, IconData(L"F12"sv) },
			{ KEY::kApostrophe, IconData(L"Quotesingle"sv) },
			{ KEY::kComma, IconData(L"Comma"sv) },
			{ KEY::kMinus, IconData(L"Hyphen"sv) },
			{ KEY::kPeriod, IconData(L"Period"sv) },
			{ KEY::kSlash, IconData(L"Slash"sv) },
			{ KEY::kSemicolon, IconData(L"Semicolon"sv) },
			{ KEY::kEquals, IconData(L"Equal"sv) },
			{ KEY::kBracketLeft, IconData(L"Bracketleft"sv) },
			{ KEY::kBackslash, IconData(L"Backslash"sv) },
			{ KEY::kBracketRight, IconData(L"Bracketright"sv) },
			{ KEY::kTilde, IconData(L"Tilde"sv) },
			{ KEY::kCapsLock, IconData(L"CapsLock"sv) },
			{ KEY::kScrollLock, IconData(L"ScrollLock"sv) },
			//{ KEY::kNumLock, IconData(L"NumLock"sv) },
			{ KEY::kPrintScreen, IconData(L"PrintScreen"sv) },
			{ KEY::kPause, IconData(L"Pause"sv) },
			{ KEY::kKP_0, IconData(L"NumPad0"sv) },
			//{ KEY::kKP_1, IconData(L"Keypad1"sv) },
			//{ KEY::kKP_2, IconData(L"Keypad2"sv) },
			//{ KEY::kKP_3, IconData(L"Keypad3"sv) },
			//{ KEY::kKP_4, IconData(L"Keypad4"sv) },
			//{ KEY::kKP_5, IconData(L"Keypad5"sv) },
			//{ KEY::kKP_6, IconData(L"Keypad6"sv) },
			//{ KEY::kKP_7, IconData(L"Keypad7"sv) },
			//{ KEY::kKP_8, IconData(L"Keypad8"sv) },
			{ KEY::kKP_9, IconData(L"NumPad9"sv) },
			{ KEY::kKP_Decimal, IconData(L"NumPadDec"sv) },
			{ KEY::kKP_Divide, IconData(L"NumPadDivide"sv) },
			{ KEY::kKP_Multiply, IconData(L"NumPadMult"sv) },
			{ KEY::kKP_Subtract, IconData(L"NumPadMinus"sv) },
			{ KEY::kKP_Plus, IconData(L"NumPadPlus"sv) },
			//{ KEY::kKP_Enter, IconData(L"KeypadEnter"sv) }
		};

		Map<std::uint32_t, GamepadIcon> gamePad{
			{ SKSE::InputMap::kGamepadButtonOffset_START, { IconData(L"360_Start"sv), IconData(L"PS3_Start"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_BACK, { IconData(L"360_Back"sv), IconData(L"PS3_Back"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LEFT_THUMB, { IconData(L"360_LS"sv), IconData(L"PS3_L3"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_RIGHT_THUMB, { IconData(L"360_RS"sv), IconData(L"PS3_R3"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LEFT_SHOULDER, { IconData(L"360_LB"sv), IconData(L"PS3_LB"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_RIGHT_SHOULDER, { IconData(L"360_RB"sv), IconData(L"PS3_RB"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_A, { IconData(L"360_A"sv), IconData(L"PS3_A"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_B, { IconData(L"PS3_B"sv), IconData(L"PS3_B"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_X, { IconData(L"360_X"sv), IconData(L"PS3_X"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_Y, { IconData(L"360_Y"sv), IconData(L"PS3_Y"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LT, { IconData(L"360_LT"sv), IconData(L"PS3_LT"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_RT, { IconData(L"360_RT"sv), IconData(L"PS3_RT"sv) } },
		};

		Map<std::uint32_t, IconData> mouse{
			{ 256 + MOUSE::kLeftButton, IconData(L"Mouse1"sv) },
			{ 256 + +MOUSE::kRightButton, IconData(L"Mouse2"sv) },
			{ 256 + +MOUSE::kMiddleButton, IconData(L"Mouse3"sv) },
			{ 256 + +MOUSE::kButton3, IconData(L"Mouse4"sv) },
			{ 256 + +MOUSE::kButton4, IconData(L"Mouse5 "sv) },
			{ 256 + +MOUSE::kButton5, IconData(L"Mouse6"sv) },
			{ 256 + +MOUSE::kButton6, IconData(L"Mouse7"sv) },
			{ 256 + +MOUSE::kButton7, IconData(L"Mouse8"sv) },
		};

		BUTTON_SCHEME buttonScheme{ BUTTON_SCHEME::kAutoDetect };
	};
}

namespace ImGui
{
	ImVec2 ButtonIcon(std::uint32_t a_key);

	ImVec2 ButtonIcon(const IconFont::IconData* a_IconData, bool a_centerIcon);
	void   ButtonIcon(const std::set<const IconFont::IconData*>& a_IconData, bool a_centerIcon);

	void ButtonIconWithLabel(const char* a_text, const IconFont::IconData* a_IconData, bool a_centerIcon);
	void ButtonIconWithLabel(const char* a_text, const std::set<const IconFont::IconData*>& a_IconData, bool a_centerIcon);
}
