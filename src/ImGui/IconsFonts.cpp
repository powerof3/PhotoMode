#include "IconsFonts.h"

#include "IconsFontAwesome6.h"
#include "Input.h"
#include "Util.h"

namespace IconFont
{
	ImageData::ImageData(std::wstring_view a_iconName)
	{
		path.append(a_iconName).append(L".png");
	}

	ImageData::~ImageData()
	{
		if (srView) {
			srView->Release();
			srView = nullptr;
		}
	}

	bool ImageData::Init()
	{
		bool result = false;

		DirectX::ScratchImage image;
		HRESULT               hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, image);

		if (SUCCEEDED(hr)) {
			if (auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
				ID3D11Resource* pTexture{};
				hr = DirectX::CreateTexture(renderer->data.forwarder, image.GetImages(), 1, image.GetMetadata(), &pTexture);

				if (SUCCEEDED(hr)) {
					D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
					srvDesc.Format = image.GetMetadata().format;
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = 1;
					srvDesc.Texture2D.MostDetailedMip = 0;

					hr = renderer->data.forwarder->CreateShaderResourceView(pTexture, &srvDesc, &srView);
					result = SUCCEEDED(hr);
				}

				// 0.0004630f is 0.5/1080
				// at 1080 render at half size

				const auto windowHeight = renderer->data.renderWindows[0].windowHeight;

				size.x = image.GetMetadata().width * (0.0004630f * windowHeight);
				size.y = image.GetMetadata().height * (0.0004630f * windowHeight);

				pTexture->Release();
			}
		}

		image.Release();

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
		unknownKey.Init();

		upKey.Init();
		downKey.Init();
		leftKey.Init();
		rightKey.Init();

		std::for_each(keyboard.begin(), keyboard.end(), [](auto& imageData) {
			imageData.second.Init();
		});
		std::for_each(gamePad.begin(), gamePad.end(), [](auto& imageData) {
			auto& [xbox, ps4] = imageData.second;
			xbox.Init();
			ps4.Init();
		});

		stepperLeft.Init();
		stepperRight.Init();
		checkbox.Init();
		checkboxFilled.Init();
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

	const ImageData* Manager::GetStepperLeft() const
	{
		return &stepperLeft;
	}
	const ImageData* Manager::GetStepperRight() const
	{
		return &stepperRight;
	}

    const ImageData* Manager::GetCheckbox() const
	{
		return &checkbox;
	}
    const ImageData* Manager::GetCheckboxFilled() const
	{
		return &checkboxFilled;
	}

    const ImageData* Manager::GetIcon(std::uint32_t key)
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

	std::set<const ImageData*> Manager::GetIcons(const std::set<std::uint32_t>& keys)
	{
		std::set<const ImageData*> icons{};
		for (auto& key : keys) {
			icons.insert(GetIcon(key));
		}
		return icons;
	}

	const ImageData* Manager::GetGamePadIcon(const GamepadIcon& a_icons) const
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
	const auto imageData = MANAGER(IconFont)->GetIcon(a_key);
	return ButtonIcon(imageData, false);
}

ImVec2 ImGui::ButtonIcon(const IconFont::ImageData* a_imageData, bool a_centerIcon)
{
	if (a_centerIcon) {
		const float height = ImGui::GetWindowSize().y;
		ImGui::SetCursorPosY((height - a_imageData->size.y) / 2);
	}
	ImGui::Image(a_imageData->srView, a_imageData->size);

	return a_imageData->size;
}

void ImGui::ButtonIcon(const std::set<const IconFont::ImageData*>& a_imageData, bool a_centerIcon)
{
	BeginGroup();
	for (auto& imageData : a_imageData) {
		auto       pos = ImGui::GetCursorPos();
		const auto size = ImGui::ButtonIcon(imageData, a_centerIcon);
		ImGui::SetCursorPos({ pos.x + size.x, pos.y });
	}
	EndGroup();
}

void ImGui::ButtonIconWithLabel(const char* a_text, const IconFont::ImageData* a_imageData, bool a_centerIcon)
{
	ImGui::ButtonIcon(a_imageData, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}

void ImGui::ButtonIconWithLabel(const char* a_text, const std::set<const IconFont::ImageData*>& a_imageData, bool a_centerIcon)
{
	ImGui::ButtonIcon(a_imageData, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}
