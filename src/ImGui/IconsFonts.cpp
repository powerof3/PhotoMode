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

	bool IconTexture::Load(float a_scale)
	{
		auto scale = (a_scale / 1080) * RE::BSGraphics::Renderer::GetScreenSize().height;
		return ImGui::Texture::LoadImpl(scale);
	}

	void Manager::LoadFontSettings(CSimpleIniA& a_ini)
	{
		ini::get_value(a_ini, fontName, "Fonts", "sFont", nullptr);
		fontName = R"(Data\Interface\ImGuiIcons\Fonts\)" + fontName;

		const auto resolutionScale = ImGui::Renderer::GetResolutionScale();

		fontSize = static_cast<float>(a_ini.GetDoubleValue("Fonts", "iFontSize", fontSize)) * resolutionScale;
		largeFontSize = static_cast<float>(a_ini.GetDoubleValue("Fonts", "iLargeFontSize", largeFontSize)) * resolutionScale;
		iconSize = static_cast<float>(a_ini.GetDoubleValue("Fonts", "iIconSize", iconSize)) * resolutionScale;
		largeIconSize = static_cast<float>(a_ini.GetDoubleValue("Fonts", "iLargeIconSize", largeIconSize)) * resolutionScale;
	}

	void Manager::LoadSettings()
	{
		Settings::GetSingleton()->Load(FileType::kFonts, [&](auto& ini) { LoadFontSettings(ini); });
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		buttonScheme = static_cast<BUTTON_SCHEME>(a_ini.GetLongValue("Controls", "iButtonScheme", std::to_underlying(buttonScheme)));
	}

	void Manager::LoadIcons()
	{
		float buttonScale = ImGui::GetUserStyleVar(ImGui::USER_STYLE::kButtons);
		float checkboxScale = ImGui::GetUserStyleVar(ImGui::USER_STYLE::kCheckbox);
		float stepperScale = ImGui::GetUserStyleVar(ImGui::USER_STYLE::kStepper);

		std::vector<std::pair<IconTexture*, float>> queue;
		queue.reserve(keyboard.size() + (gamePad.size() * 2) + mouse.size() + 9);

		const auto queue_into = [&](IconTexture& a_texture, float a_scale) {
			queue.emplace_back(&a_texture, a_scale);
		};

		queue_into(unknownKey, buttonScale);
		queue_into(upKey, buttonScale);
		queue_into(downKey, buttonScale);
		queue_into(leftKey, buttonScale);
		queue_into(rightKey, buttonScale);
		for (auto& [key, texture] : keyboard) {
			queue_into(texture, buttonScale);
		}
		for (auto& [key, textures] : gamePad) {
			auto& [xbox, ps4] = textures;
			queue_into(xbox, buttonScale);
			queue_into(ps4, buttonScale);
		}
		for (auto& [key, texture] : mouse) {
			queue_into(texture, buttonScale);
		}
		queue_into(stepperLeft, stepperScale);
		queue_into(stepperRight, stepperScale);
		queue_into(checkbox, checkboxScale);
		queue_into(checkboxFilled, checkboxScale);

		std::for_each(std::execution::par, queue.begin(), queue.end(), [](auto& a_entry) {
			a_entry.first->Load(a_entry.second);
		});
	}

	void Manager::LoadFonts()
	{
		const auto& io = ImGui::GetIO();

		io.Fonts->AddFontFromFileTTF(fontName.c_str(), fontSize);

		ImFontConfig icon_config;
		icon_config.MergeMode = true;
		icon_config.PixelSnapH = true;
		icon_config.OversampleH = icon_config.OversampleV = 1;

		io.Fonts->AddFontFromFileTTF(R"(Data\Interface\ImGuiIcons\Fonts\)" FONT_ICON_FILE_NAME_FAS, iconSize, &icon_config);
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

void ImGui::CalcButtonWidth(float& a_width, const IconFont::IconTexture* a_texture, const char* a_textLabel, bool a_sameLine)
{
	const auto& spacing = ImGui::GetStyle().ItemSpacing.x;

	a_width += a_texture->size.x;
	a_width += spacing;
	a_width += ImGui::CalcTextSize(a_textLabel).x;
	if (a_sameLine) {
		a_width += spacing;
	}
}

float ImGui::PrecalcButtonBarWidth(std::span<const ButtonBarItem> a_items)
{
	float width = 0.0f;
	for (const auto& item : a_items) {
		if (item.conditional) {
			CalcButtonWidth(width, item.icon, item.label);
		}
	}
	return width;
}

void ImGui::ButtonBar(std::span<const ButtonBarItem> a_items, float a_itemsWidth, float a_alignment)
{
	AlignForWidth(a_itemsWidth, a_alignment);

	for (auto i = 0; i < a_items.size(); ++i) {
		const auto& item = a_items[i];
		if (!item.conditional) {
			continue;
		}
		BeginDisabled(item.disabled);
		ButtonIconWithLabel(item.label, item.icon, true);
		EndDisabled();
		if (i + 1 < a_items.size()) {
			SameLine();
		}
	}
}

void ImGui::ButtonBar(std::span<const ButtonBarItem> a_items, float a_alignment)
{
	auto itemsWidth = PrecalcButtonBarWidth(a_items);
	ButtonBar(a_items, itemsWidth, a_alignment);
}
