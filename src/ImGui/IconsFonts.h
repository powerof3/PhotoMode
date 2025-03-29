#pragma once

#include "ImGui/Graphics.h"

namespace IconFont
{
	struct IconTexture final : ImGui::Texture
	{
		IconTexture() = delete;
		IconTexture(std::wstring_view a_iconName);

		~IconTexture() override = default;

		bool Load(bool a_resizeToScreenRes = false) override;
		void Resize(float a_scale);

		// members
		ImVec2 imageSize{};
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		struct GamepadIcon
		{
			IconTexture xbox;
			IconTexture ps4;
		};

		void LoadSettings();
		void LoadMCMSettings(const CSimpleIniA& a_ini);

		void LoadIcons();
		void ReloadFonts();
		void ResizeIcons();

		ImFont* GetLargeFont() const;

		const IconTexture* GetStepperLeft() const;
		const IconTexture* GetStepperRight() const;
		const IconTexture* GetCheckbox() const;
		const IconTexture* GetCheckboxFilled() const;

		const IconTexture*           GetIcon(std::uint32_t key);
		std::set<const IconTexture*> GetIcons(const std::set<std::uint32_t>& keys);

		const IconTexture* GetGamePadIcon(const GamepadIcon& a_icons) const;

	private:
		enum class BUTTON_SCHEME
		{
			kAutoDetect,
			kXbox,
			kPS4
		};

		void    LoadFontSettings(CSimpleIniA& a_ini);
		ImFont* LoadFontIconSet(float a_fontSize, float a_iconSize, const ImVector<ImWchar>& a_ranges) const;

		// members
		bool loadedFonts{ false };

		std::string fontName{ "Jost-Regular.ttf" };
		float       fontSize{ 26 };
		float       iconSize{ 20 };
		float       largeFontSize{ 30 };
		float       largeIconSize{ 24 };

		ImFont* largeFont{ nullptr };

		IconTexture stepperLeft{ L"StepperLeft"sv };
		IconTexture stepperRight{ L"StepperRight"sv };
		IconTexture checkbox{ L"Checkbox"sv };
		IconTexture checkboxFilled{ L"Checkbox-Filled"sv };

		IconTexture unknownKey{ L"UnknownKey"sv };

		IconTexture leftKey{ L"Left"sv };
		IconTexture rightKey{ L"Right"sv };
		IconTexture upKey{ L"Up"sv };
		IconTexture downKey{ L"Down"sv };

		Map<KEY, IconTexture> keyboard{
			{ KEY::kTab, IconTexture(L"Tab"sv) },
			{ KEY::kPageUp, IconTexture(L"PgUp"sv) },
			{ KEY::kPageDown, IconTexture(L"PgDn"sv) },
			{ KEY::kHome, IconTexture(L"Home"sv) },
			{ KEY::kEnd, IconTexture(L"End"sv) },
			{ KEY::kInsert, IconTexture(L"Insert"sv) },
			{ KEY::kDelete, IconTexture(L"Delete"sv) },
			{ KEY::kBackspace, IconTexture(L"Backspace"sv) },
			{ KEY::kSpacebar, IconTexture(L"Space"sv) },
			{ KEY::kEnter, IconTexture(L"Enter"sv) },
			{ KEY::kEscape, IconTexture(L"Esc"sv) },
			{ KEY::kLeftControl, IconTexture(L"L-Ctrl"sv) },
			{ KEY::kLeftShift, IconTexture(L"L-Shift"sv) },
			{ KEY::kLeftAlt, IconTexture(L"L-Alt"sv) },
			//{ KEY::kLeftWin, IconTexture(L"LeftWin"sv) },
			{ KEY::kRightControl, IconTexture(L"R-Ctrl"sv) },
			{ KEY::kRightShift, IconTexture(L"R-Shift"sv) },
			{ KEY::kRightAlt, IconTexture(L"R-Alt"sv) },
			//{ KEY::kRightWin, IconTexture(L"RightWin"sv) },
			{ KEY::kNum0, IconTexture(L"0"sv) },
			{ KEY::kNum1, IconTexture(L"1"sv) },
			{ KEY::kNum2, IconTexture(L"2"sv) },
			{ KEY::kNum3, IconTexture(L"3"sv) },
			{ KEY::kNum4, IconTexture(L"4"sv) },
			{ KEY::kNum5, IconTexture(L"5"sv) },
			{ KEY::kNum6, IconTexture(L"6"sv) },
			{ KEY::kNum7, IconTexture(L"7"sv) },
			{ KEY::kNum8, IconTexture(L"8"sv) },
			{ KEY::kNum9, IconTexture(L"9"sv) },
			{ KEY::kA, IconTexture(L"A"sv) },
			{ KEY::kB, IconTexture(L"B"sv) },
			{ KEY::kC, IconTexture(L"C"sv) },
			{ KEY::kD, IconTexture(L"D"sv) },
			{ KEY::kE, IconTexture(L"E"sv) },
			{ KEY::kF, IconTexture(L"F"sv) },
			{ KEY::kG, IconTexture(L"G"sv) },
			{ KEY::kH, IconTexture(L"H"sv) },
			{ KEY::kI, IconTexture(L"I"sv) },
			{ KEY::kJ, IconTexture(L"J"sv) },
			{ KEY::kK, IconTexture(L"K"sv) },
			{ KEY::kL, IconTexture(L"L"sv) },
			{ KEY::kM, IconTexture(L"M"sv) },
			{ KEY::kN, IconTexture(L"N"sv) },
			{ KEY::kO, IconTexture(L"O"sv) },
			{ KEY::kP, IconTexture(L"P"sv) },
			{ KEY::kQ, IconTexture(L"Q"sv) },
			{ KEY::kR, IconTexture(L"R"sv) },
			{ KEY::kS, IconTexture(L"S"sv) },
			{ KEY::kT, IconTexture(L"T"sv) },
			{ KEY::kU, IconTexture(L"U"sv) },
			{ KEY::kV, IconTexture(L"V"sv) },
			{ KEY::kW, IconTexture(L"W"sv) },
			{ KEY::kX, IconTexture(L"X"sv) },
			{ KEY::kY, IconTexture(L"Y"sv) },
			{ KEY::kZ, IconTexture(L"Z"sv) },
			{ KEY::kF1, IconTexture(L"F1"sv) },
			{ KEY::kF2, IconTexture(L"F2"sv) },
			{ KEY::kF3, IconTexture(L"F3"sv) },
			{ KEY::kF4, IconTexture(L"F4"sv) },
			{ KEY::kF5, IconTexture(L"F5"sv) },
			{ KEY::kF6, IconTexture(L"F6"sv) },
			{ KEY::kF7, IconTexture(L"F7"sv) },
			{ KEY::kF8, IconTexture(L"F8"sv) },
			{ KEY::kF9, IconTexture(L"F9"sv) },
			{ KEY::kF10, IconTexture(L"F10"sv) },
			{ KEY::kF11, IconTexture(L"F11"sv) },
			{ KEY::kF12, IconTexture(L"F12"sv) },
			{ KEY::kApostrophe, IconTexture(L"Quotesingle"sv) },
			{ KEY::kComma, IconTexture(L"Comma"sv) },
			{ KEY::kMinus, IconTexture(L"Hyphen"sv) },
			{ KEY::kPeriod, IconTexture(L"Period"sv) },
			{ KEY::kSlash, IconTexture(L"Slash"sv) },
			{ KEY::kSemicolon, IconTexture(L"Semicolon"sv) },
			{ KEY::kEquals, IconTexture(L"Equal"sv) },
			{ KEY::kBracketLeft, IconTexture(L"Bracketleft"sv) },
			{ KEY::kBackslash, IconTexture(L"Backslash"sv) },
			{ KEY::kBracketRight, IconTexture(L"Bracketright"sv) },
			{ KEY::kTilde, IconTexture(L"Tilde"sv) },
			{ KEY::kCapsLock, IconTexture(L"CapsLock"sv) },
			{ KEY::kScrollLock, IconTexture(L"ScrollLock"sv) },
			{ KEY::kNumLock, IconTexture(L"NumLock"sv) },
			{ KEY::kPrintScreen, IconTexture(L"PrintScreen"sv) },
			{ KEY::kPause, IconTexture(L"Pause"sv) },
			{ KEY::kKP_0, IconTexture(L"NumPad0"sv) },
			{ KEY::kKP_1, IconTexture(L"Keypad1"sv) },
			{ KEY::kKP_2, IconTexture(L"Keypad2"sv) },
			{ KEY::kKP_3, IconTexture(L"Keypad3"sv) },
			{ KEY::kKP_4, IconTexture(L"Keypad4"sv) },
			{ KEY::kKP_5, IconTexture(L"Keypad5"sv) },
			{ KEY::kKP_6, IconTexture(L"Keypad6"sv) },
			{ KEY::kKP_7, IconTexture(L"Keypad7"sv) },
			{ KEY::kKP_8, IconTexture(L"Keypad8"sv) },
			{ KEY::kKP_9, IconTexture(L"NumPad9"sv) },
			{ KEY::kKP_Decimal, IconTexture(L"NumPadDec"sv) },
			{ KEY::kKP_Divide, IconTexture(L"NumPadDivide"sv) },
			{ KEY::kKP_Multiply, IconTexture(L"NumPadMult"sv) },
			{ KEY::kKP_Subtract, IconTexture(L"NumPadMinus"sv) },
			{ KEY::kKP_Plus, IconTexture(L"NumPadPlus"sv) },
			{ KEY::kKP_Enter, IconTexture(L"KeypadEnter"sv) }
		};

		Map<std::uint32_t, GamepadIcon> gamePad{
			{ SKSE::InputMap::kGamepadButtonOffset_START, { IconTexture(L"360_Start"sv), IconTexture(L"PS3_Start"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_BACK, { IconTexture(L"360_Back"sv), IconTexture(L"PS3_Back"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LEFT_THUMB, { IconTexture(L"360_LS"sv), IconTexture(L"PS3_L3"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_RIGHT_THUMB, { IconTexture(L"360_RS"sv), IconTexture(L"PS3_R3"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LEFT_SHOULDER, { IconTexture(L"360_LB"sv), IconTexture(L"PS3_LB"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_RIGHT_SHOULDER, { IconTexture(L"360_RB"sv), IconTexture(L"PS3_RB"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_A, { IconTexture(L"360_A"sv), IconTexture(L"PS3_A"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_B, { IconTexture(L"360_B"sv), IconTexture(L"PS3_B"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_X, { IconTexture(L"360_X"sv), IconTexture(L"PS3_X"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_Y, { IconTexture(L"360_Y"sv), IconTexture(L"PS3_Y"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_LT, { IconTexture(L"360_LT"sv), IconTexture(L"PS3_LT"sv) } },
			{ SKSE::InputMap::kGamepadButtonOffset_RT, { IconTexture(L"360_RT"sv), IconTexture(L"PS3_RT"sv) } },
		};

		Map<std::uint32_t, IconTexture> mouse{
			{ 256 + MOUSE::kLeftButton, IconTexture(L"Mouse1"sv) },
			{ 256 + +MOUSE::kRightButton, IconTexture(L"Mouse2"sv) },
			{ 256 + +MOUSE::kMiddleButton, IconTexture(L"Mouse3"sv) },
			{ 256 + +MOUSE::kButton3, IconTexture(L"Mouse4"sv) },
			{ 256 + +MOUSE::kButton4, IconTexture(L"Mouse5 "sv) },
			{ 256 + +MOUSE::kButton5, IconTexture(L"Mouse6"sv) },
			{ 256 + +MOUSE::kButton6, IconTexture(L"Mouse7"sv) },
			{ 256 + +MOUSE::kButton7, IconTexture(L"Mouse8"sv) },
		};

		BUTTON_SCHEME buttonScheme{ BUTTON_SCHEME::kAutoDetect };
	};
}

namespace ImGui
{
	ImVec2 ButtonIcon(std::uint32_t a_key);

	ImVec2 ButtonIcon(const IconFont::IconTexture* a_texture, bool a_centerIcon);
	void   ButtonIcon(const std::set<const IconFont::IconTexture*>& a_texture, bool a_centerIcon);

	void ButtonIconWithLabel(const char* a_text, const IconFont::IconTexture* a_texture, bool a_centerIcon);
	void ButtonIconWithLabel(const char* a_text, const std::set<const IconFont::IconTexture*>& a_texture, bool a_centerIcon);
}
