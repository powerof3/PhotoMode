#include "IconsFonts.h"

#include "IconsFontAwesome6.h"
#include "Input.h"
#include "Util.h"

namespace IconFont
{
	IconData::IconData(std::wstring_view a_iconName) :
		ImageData(LR"(Data\Interface\PhotoMode\Icons\)", a_iconName)
	{}

	bool IconData::Create(bool a_resizeToScreenRes)
	{
		const bool result = ImageData::Create(a_resizeToScreenRes);

		if (result) {
			// 0.0004630f is 0.5/1080
			// at 1080 render at half size
			const auto windowHeight = RE::BSGraphics::Renderer::GetSingleton()->data.renderWindows[0].windowHeight;

			size.x = size.x * (0.0004630f * windowHeight);
			size.y = size.y * (0.0004630f * windowHeight);

			// don't need this
			if (image) {
				image.reset();
			}
		}

		return result;
	}

	void Manager::LoadSettings(CSimpleIniA& a_ini)
	{
		ini::get_value(a_ini, fontName, "Fonts", "Font", nullptr);
		fontName = R"(Data\Interface\PhotoMode\Fonts\)" + fontName;

		ini::get_value(a_ini, fontSize, "Fonts", "FontSize", nullptr);
		ini::get_value(a_ini, largeFontSize, "Fonts", "LargeFontSize", nullptr);

		ini::get_value(a_ini, iconSize, "Fonts", "IconSize", nullptr);
		ini::get_value(a_ini, largeIconSize, "Fonts", "LargeIconSize", nullptr);
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		buttonScheme = static_cast<BUTTON_SCHEME>(a_ini.GetLongValue("Controls", "iButtonScheme", std::to_underlying(buttonScheme)));
	}

	void Manager::LoadIcons()
	{
		unknownKey.Create();

		upKey.Create();
		downKey.Create();
		leftKey.Create();
		rightKey.Create();

		std::for_each(keyboard.begin(), keyboard.end(), [](auto& IconData) {
			IconData.second.Create();
		});
		std::for_each(gamePad.begin(), gamePad.end(), [](auto& IconData) {
			auto& [xbox, ps4] = IconData.second;
			xbox.Create();
			ps4.Create();
		});

		stepperLeft.Create();
		stepperRight.Create();
		checkbox.Create();
		checkboxFilled.Create();
	}

	void Manager::LoadFonts()
	{
		if (loadedFonts) {
			return;
		}

		loadedFonts = true;

		ImVector<ImWchar>        ranges;
		ImFontGlyphRangesBuilder builder;
		builder.AddText(RE::BSScaleformManager::GetSingleton()->validNameChars.c_str());
		builder.AddChar(0xf030);  // CAMERA
		builder.AddChar(0xf017);  // CLOCK
		builder.AddChar(0xf183);  // PERSON
		builder.AddChar(0xf042);  // CONTRAST
		builder.AddChar(0xf03e);  // IMAGE
		builder.BuildRanges(&ranges);

		auto& io = ImGui::GetIO();
		io.FontDefault = LoadFontIconPair(static_cast<float>(fontSize), static_cast<float>(iconSize), ranges);
		largeFont = LoadFontIconPair(static_cast<float>(largeFontSize), static_cast<float>(largeIconSize), ranges);

		io.Fonts->Build();
	}

	ImFont* Manager::LoadFontIconPair(float a_fontSize, float a_iconSize, const ImVector<ImWchar>& a_ranges) const
	{
		const auto& io = ImGui::GetIO();

		const auto font = io.Fonts->AddFontFromFileTTF(fontName.c_str(), a_fontSize, nullptr, a_ranges.Data);

		ImFontConfig icon_config;
		icon_config.MergeMode = true;
		icon_config.PixelSnapH = true;
		icon_config.OversampleH = icon_config.OversampleV = 1;

		io.Fonts->AddFontFromFileTTF(R"(Data\Interface\PhotoMode\Fonts\)" FONT_ICON_FILE_NAME_FAS, a_iconSize, &icon_config, a_ranges.Data);

		return font;
	}

	ImFont* Manager::GetLargeFont() const
	{
		return largeFont;
	}

	const IconData* Manager::GetStepperLeft() const
	{
		return &stepperLeft;
	}
	const IconData* Manager::GetStepperRight() const
	{
		return &stepperRight;
	}

	const IconData* Manager::GetCheckbox() const
	{
		return &checkbox;
	}
	const IconData* Manager::GetCheckboxFilled() const
	{
		return &checkboxFilled;
	}

	const IconData* Manager::GetIcon(std::uint32_t key)
	{
		switch (key) {
		case KEY::kUp:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_UP:
			return &upKey;
		case KEY::kDown:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_DOWN:
			return &downKey;
		case KEY::kLeft:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_LEFT:
			return &leftKey;
		case KEY::kRight:
		case SKSE::InputMap::kGamepadButtonOffset_DPAD_RIGHT:
			return &rightKey;
		default:
			{
				if (Input::GetInputType() == Input::TYPE::kKeyboard) {
					if (const auto it = keyboard.find(static_cast<KEY>(key)); it != keyboard.end()) {
						return &it->second;
					}
				} else {
					if (const auto it = gamePad.find(key); it != gamePad.end()) {
						return GetGamePadIcon(it->second);
					}
				}
				return &unknownKey;
			}
		}
	}

	std::set<const IconData*> Manager::GetIcons(const std::set<std::uint32_t>& keys)
	{
		std::set<const IconData*> icons{};
		if (keys.empty()) {
			icons.insert(&unknownKey);
		} else {
			for (auto& key : keys) {
				icons.insert(GetIcon(key));
			}
		}
		return icons;
	}

	const IconData* Manager::GetGamePadIcon(const GamepadIcon& a_icons) const
	{
		switch (buttonScheme) {
		case BUTTON_SCHEME::kAutoDetect:
			return Input::GetInputType() == Input::TYPE::kGamepadOrbis ? &a_icons.ps4 : &a_icons.xbox;
		case BUTTON_SCHEME::kXbox:
			return &a_icons.xbox;
		case BUTTON_SCHEME::kPS4:
			return &a_icons.ps4;
		default:
			return &a_icons.xbox;
		}
	}
}

ImVec2 ImGui::ButtonIcon(std::uint32_t a_key)
{
	const auto IconData = MANAGER(IconFont)->GetIcon(a_key);
	return ButtonIcon(IconData, false);
}

ImVec2 ImGui::ButtonIcon(const IconFont::IconData* a_IconData, bool a_centerIcon)
{
	if (a_centerIcon) {
		const float height = ImGui::GetWindowSize().y;
		ImGui::SetCursorPosY((height - a_IconData->size.y) / 2);
	}
	ImGui::Image(a_IconData->srView.Get(), a_IconData->size);

	return a_IconData->size;
}

void ImGui::ButtonIcon(const std::set<const IconFont::IconData*>& a_IconData, bool a_centerIcon)
{
	BeginGroup();
	for (auto& IconData : a_IconData) {
		auto       pos = ImGui::GetCursorPos();
		const auto size = ImGui::ButtonIcon(IconData, a_centerIcon);
		ImGui::SetCursorPos({ pos.x + size.x, pos.y });
	}
	EndGroup();
}

void ImGui::ButtonIconWithLabel(const char* a_text, const IconFont::IconData* a_IconData, bool a_centerIcon)
{
	ImGui::ButtonIcon(a_IconData, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}

void ImGui::ButtonIconWithLabel(const char* a_text, const std::set<const IconFont::IconData*>& a_IconData, bool a_centerIcon)
{
	ImGui::ButtonIcon(a_IconData, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}
