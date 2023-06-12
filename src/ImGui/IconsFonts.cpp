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
		fontName = R"(Data\Interface\Fonts\)" + fontName;

		ini::get_value(a_ini, fontSize, "Fonts", "FontSize", nullptr);
		ini::get_value(a_ini, largeFontSize, "Fonts", "LargeFontSize", nullptr);

		ini::get_value(a_ini, iconSize, "Fonts", "IconSize", nullptr);
		ini::get_value(a_ini, largeIconSize, "Fonts", "LargeIconSize", nullptr);
	}

	void Manager::LoadIcons()
	{
		unknownKey.Init();
		stepperLeft.Init();
		stepperRight.Init();

		std::for_each(keyboard.begin(), keyboard.end(), [](auto& imageData) {
			imageData.second.Init();
		});
		std::for_each(xbox.begin(), xbox.end(), [](auto& imageData) {
			imageData.second.Init();
		});
		std::for_each(ps4.begin(), ps4.end(), [](auto& imageData) {
			imageData.second.Init();
		});
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
		builder.AddChar(0xf013);  // GEAR
		builder.BuildRanges(&ranges);

		auto& io = ImGui::GetIO();
		io.FontDefault = LoadFontIconPair(fontSize, iconSize, ranges);
		largeFont = LoadFontIconPair(largeFontSize, largeIconSize, ranges);

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

		io.Fonts->AddFontFromFileTTF(R"(Data\Interface\Fonts\)" FONT_ICON_FILE_NAME_FAS, a_iconSize, &icon_config, a_ranges.Data);

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

	const ImageData* Manager::GetIcon(Input::TYPE a_type, std::uint32_t key)
	{
		switch (a_type) {
		case Input::TYPE::kKeyboard:
			{
				if (const auto it = keyboard.find(static_cast<KEY>(key)); it != keyboard.end()) {
					return &it->second;
				}
			}
			break;
		case Input::TYPE::kGamepadDirectX:
			{
				if (const auto it = xbox.find(static_cast<GAMEPAD_DIRECTX>(key)); it != xbox.end()) {
					return &it->second;
				}
			}
			break;
		case Input::TYPE::kGamepadOrbis:
			{
				if (const auto it = ps4.find(static_cast<GAMEPAD_ORBIS>(key)); it != ps4.end()) {
					return &it->second;
				}
			}
			break;
		default:
			break;
		}

		return &unknownKey;
	}

	std::set<const ImageData*> Manager::GetIcons(Input::TYPE a_type, const std::set<std::uint32_t>& keys)
	{
		std::uint32_t              processedKey = 0;
		std::set<const ImageData*> icons{};

		for (auto& key : keys) {
			switch (a_type) {
			case Input::TYPE::kKeyboard:
				processedKey = key;
				break;
			case Input::TYPE::kGamepadDirectX:
			case Input::TYPE::kGamepadOrbis:
				processedKey = SKSE::InputMap::GamepadKeycodeToMask(key);
				break;
			default:
				break;
			}

			icons.insert(GetIcon(a_type, processedKey));
		}

		return icons;
	}
}

ImVec2 ImGui::ButtonIcon(Input::TYPE a_type, std::uint32_t a_key)
{
	const auto imageData = MANAGER(IconFont)->GetIcon(a_type, a_key);
	return ButtonIcon(imageData, false);
}

void ImGui::ButtonIcon(Input::TYPE a_type, const std::set<std::uint32_t>& a_keys)
{
	const auto imageData = MANAGER(IconFont)->GetIcons(a_type, a_keys);
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
