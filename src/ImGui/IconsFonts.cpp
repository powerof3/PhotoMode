#include "IconsFonts.h"

#include "IconsFontAwesome6.h"
#include "Input.h"
#include "Renderer.h"
#include "Settings.h"
#include "Styles.h"
#include "Util.h"

namespace IconFont
{
	IconTexture::IconTexture(std::wstring_view a_iconName) :
		ImGui::Texture(LR"(Data/Interface/ImGuiIcons/Icons/)", a_iconName)
	{}

	bool IconTexture::Load(bool a_resizeToScreenRes)
	{
		const bool result = ImGui::Texture::Load(a_resizeToScreenRes);

		if (result) {
			// store original size
			imageSize = size;
			// don't need this
			if (image) {
				image.reset();
			}
		}

		return result;
	}

	void IconTexture::Resize(float a_scale)
	{
		auto scale = a_scale / 1080;  // standard window height
		size = imageSize * (scale * RE::BSGraphics::Renderer::GetScreenSize().height);
	}

	void Manager::LoadFontSettings(CSimpleIniA& a_ini)
	{
		ini::get_value(a_ini, fontName, "Fonts", "sFont", nullptr);
		fontName = R"(Data\Interface\ImGuiIcons\Fonts\)" + fontName;

		const auto resolutionScale = ImGui::Renderer::GetResolutionScale();

		fontSize = static_cast<float>(a_ini.GetLongValue("Fonts", "iFontSize", fontSize)) * resolutionScale;
		largeFontSize = static_cast<float>(a_ini.GetLongValue("Fonts", "iLargeFontSize", largeFontSize)) * resolutionScale;
		iconSize = static_cast<float>(a_ini.GetLongValue("Fonts", "iIconSize", iconSize)) * resolutionScale;
		largeIconSize = static_cast<float>(a_ini.GetLongValue("Fonts", "iLargeIconSize", largeIconSize)) * resolutionScale;
	}

	void Manager::LoadSettings()
	{
		Settings::GetSingleton()->SerializeFonts([this](auto& ini) { LoadFontSettings(ini); });
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		buttonScheme = static_cast<BUTTON_SCHEME>(a_ini.GetLongValue("Controls", "iButtonScheme", std::to_underlying(buttonScheme)));
	}

	void Manager::LoadIcons()
	{
		unknownKey.Load();

		upKey.Load();
		downKey.Load();
		leftKey.Load();
		rightKey.Load();

		std::for_each(keyboard.begin(), keyboard.end(), [](auto& IconTexture) {
			IconTexture.second.Load();
		});
		std::for_each(gamePad.begin(), gamePad.end(), [](auto& IconTexture) {
			auto& [xbox, ps4] = IconTexture.second;
			xbox.Load();
			ps4.Load();
		});
		std::for_each(mouse.begin(), mouse.end(), [](auto& IconTexture) {
			IconTexture.second.Load();
		});

		stepperLeft.Load();
		stepperRight.Load();
		checkbox.Load();
		checkboxFilled.Load();
	}

	void Manager::ReloadFonts()
	{
		auto& io = ImGui::GetIO();
		io.Fonts->Clear();

		ImVector<ImWchar> ranges;

		ImFontGlyphRangesBuilder builder;
		builder.AddText(RE::BSScaleformManager::GetSingleton()->validNameChars.c_str());
		builder.AddChar(0xf030);  // CAMERA
		builder.AddChar(0xf017);  // CLOCK
		builder.AddChar(0xf183);  // PERSON
		builder.AddChar(0xf042);  // CONTRAST
		builder.AddChar(0xf03e);  // IMAGE
		builder.BuildRanges(&ranges);

		io.FontDefault = LoadFontIconSet(fontSize, iconSize, ranges);
		largeFont = LoadFontIconSet(largeFontSize, largeIconSize, ranges);

		io.Fonts->Build();

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();
	}

	void Manager::ResizeIcons()
	{
		float buttonScale = ImGui::GetUserStyleVar(ImGui::USER_STYLE::kButtons);
		float checkboxScale = ImGui::GetUserStyleVar(ImGui::USER_STYLE::kCheckbox);
		float stepperScale = ImGui::GetUserStyleVar(ImGui::USER_STYLE::kStepper);

		unknownKey.Resize(buttonScale);

		upKey.Resize(buttonScale);
		downKey.Resize(buttonScale);
		leftKey.Resize(buttonScale);
		rightKey.Resize(buttonScale);

		std::for_each(keyboard.begin(), keyboard.end(), [&](auto& IconTexture) {
			IconTexture.second.Resize(buttonScale);
		});
		std::for_each(gamePad.begin(), gamePad.end(), [&](auto& IconTexture) {
			auto& [xbox, ps4] = IconTexture.second;
			xbox.Resize(buttonScale);
			ps4.Resize(buttonScale);
		});
		std::for_each(mouse.begin(), mouse.end(), [&](auto& IconTexture) {
			IconTexture.second.Resize(buttonScale);
		});

		stepperLeft.Resize(stepperScale);
		stepperRight.Resize(stepperScale);

		checkbox.Resize(checkboxScale);
		checkboxFilled.Resize(checkboxScale);
	}

	ImFont* Manager::LoadFontIconSet(float a_fontSize, float a_iconSize, const ImVector<ImWchar>& a_ranges) const
	{
		const auto& io = ImGui::GetIO();

		const auto font = io.Fonts->AddFontFromFileTTF(fontName.c_str(), a_fontSize, nullptr, a_ranges.Data);

		ImFontConfig icon_config;
		icon_config.MergeMode = true;
		icon_config.PixelSnapH = true;
		icon_config.OversampleH = icon_config.OversampleV = 1;

		io.Fonts->AddFontFromFileTTF(R"(Data\Interface\ImGuiIcons\Fonts\)" FONT_ICON_FILE_NAME_FAS, a_iconSize, &icon_config, a_ranges.Data);

		return font;
	}

	ImFont* Manager::GetLargeFont() const
	{
		return largeFont;
	}

	const IconTexture* Manager::GetStepperLeft() const
	{
		return &stepperLeft;
	}
	const IconTexture* Manager::GetStepperRight() const
	{
		return &stepperRight;
	}

	const IconTexture* Manager::GetCheckbox() const
	{
		return &checkbox;
	}
	const IconTexture* Manager::GetCheckboxFilled() const
	{
		return &checkboxFilled;
	}

	const IconTexture* Manager::GetIcon(std::uint32_t key)
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
				if (auto inputDevice = MANAGER(Input)->GetInputDevice(); inputDevice == Input::DEVICE::kKeyboard || inputDevice == Input::DEVICE::kMouse) {
					if (key >= SKSE::InputMap::kMacro_MouseButtonOffset) {
						if (const auto it = mouse.find(key); it != mouse.end()) {
							return &it->second;
						}
					} else if (const auto it = keyboard.find(static_cast<KEY>(key)); it != keyboard.end()) {
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

	std::set<const IconTexture*> Manager::GetIcons(const std::set<std::uint32_t>& keys)
	{
		std::set<const IconTexture*> icons{};
		if (keys.empty()) {
			icons.insert(&unknownKey);
		} else {
			for (auto& key : keys) {
				icons.insert(GetIcon(key));
			}
		}
		return icons;
	}

	const IconTexture* Manager::GetGamePadIcon(const GamepadIcon& a_icons) const
	{
		switch (buttonScheme) {
		case BUTTON_SCHEME::kAutoDetect:
			return MANAGER(Input)->GetInputDevice() == Input::DEVICE::kGamepadOrbis ? &a_icons.ps4 : &a_icons.xbox;
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
	const auto IconTexture = MANAGER(IconFont)->GetIcon(a_key);
	return ButtonIcon(IconTexture, false);
}

ImVec2 ImGui::ButtonIcon(const IconFont::IconTexture* a_texture, bool a_centerIcon)
{
	if (a_centerIcon) {
		const float height = ImGui::GetWindowSize().y;
		ImGui::SetCursorPosY((height - a_texture->size.y) / 2);
	}
	ImGui::Image((ImTextureID)a_texture->srView.Get(), a_texture->size);

	return a_texture->size;
}

void ImGui::ButtonIcon(const std::set<const IconFont::IconTexture*>& a_texture, bool a_centerIcon)
{
	BeginGroup();
	for (auto& IconTexture : a_texture) {
		auto       pos = ImGui::GetCursorPos();
		const auto size = ImGui::ButtonIcon(IconTexture, a_centerIcon);
		ImGui::SetCursorPos({ pos.x + size.x, pos.y });
	}
	EndGroup();
}

void ImGui::ButtonIconWithLabel(const char* a_text, const IconFont::IconTexture* a_texture, bool a_centerIcon)
{
	ImGui::ButtonIcon(a_texture, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}

void ImGui::ButtonIconWithLabel(const char* a_text, const std::set<const IconFont::IconTexture*>& a_texture, bool a_centerIcon)
{
	ImGui::ButtonIcon(a_texture, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}
