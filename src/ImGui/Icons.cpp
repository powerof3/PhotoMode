#include "Icons.h"

#include "IconsFontAwesome6.h"
#include "Input.h"
#include "Util.h"

namespace Icon
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
		constexpr float          baseFontSize = 24.0f;
		constexpr float          bigFontSize = 28.0f;
		constexpr float          baseIconSize = 20.0f;
		constexpr float          bigIconSize = 24.0f;
		static constexpr ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

		const auto& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF(R"(Data\Interface\Fonts\Jost-Medium.ttf)", baseFontSize);

		ImFontConfig config;
		config.MergeMode = true;
		config.PixelSnapH = true;
		config.OversampleH = config.OversampleV = 1;

		io.Fonts->AddFontFromFileTTF("Data\\Interface\\Fonts\\" FONT_ICON_FILE_NAME_FAS, baseIconSize, &config, icon_ranges);

		config.MergeMode = false;

		bigIconFont = io.Fonts->AddFontFromFileTTF("Data\\Interface\\Fonts\\" FONT_ICON_FILE_NAME_FAS, bigIconSize, &config, icon_ranges);
		bigFont = io.Fonts->AddFontFromFileTTF(R"(Data\Interface\Fonts\Jost-Medium.ttf)", bigFontSize);

		io.Fonts->Build();
	}

	ImFont* Manager::GetBigFont() const
	{
		return bigFont;
	}

	ImFont* Manager::GetBigIconFont() const
	{
		return bigIconFont;
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
	const auto imageData = Icon::Manager::GetSingleton()->GetIcon(a_type, a_key);
	return ButtonIcon(imageData, false);
}

void ImGui::ButtonIcon(Input::TYPE a_type, const std::set<std::uint32_t>& a_keys)
{
	const auto imageData = Icon::Manager::GetSingleton()->GetIcons(a_type, a_keys);
	return ButtonIcon(imageData, false);
}

ImVec2 ImGui::ButtonIcon(const Icon::ImageData* a_imageData, bool a_centerIcon)
{
	if (a_centerIcon) {
		const float height = ImGui::GetWindowSize().y;
		ImGui::SetCursorPosY((height - a_imageData->size.y) / 2);
	}
	ImGui::Image(a_imageData->srView, a_imageData->size);

	return a_imageData->size;
}

void ImGui::ButtonIcon(const std::set<const Icon::ImageData*>& a_imageData, bool a_centerIcon)
{
	BeginGroup();
	for (auto& imageData : a_imageData) {
		auto       pos = ImGui::GetCursorPos();
		const auto size = ImGui::ButtonIcon(imageData, a_centerIcon);
		ImGui::SetCursorPos({ pos.x + size.x, pos.y });
	}
	EndGroup();
}

void ImGui::ButtonIconWithLabel(const char* a_text, const Icon::ImageData* a_imageData, bool a_centerIcon)
{
	ImGui::ButtonIcon(a_imageData, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}

void ImGui::ButtonIconWithLabel(const char* a_text, const std::set<const Icon::ImageData*>& a_imageData, bool a_centerIcon)
{
	ImGui::ButtonIcon(a_imageData, a_centerIcon);
	ImGui::SameLine();
	ImGui::CenteredText(a_text, true);
}
