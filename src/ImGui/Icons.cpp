#include "Icons.h"

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

				width = static_cast<uint32_t>(image.GetMetadata().width * (0.0004630f * windowHeight));
				height = static_cast<uint32_t>(image.GetMetadata().height * (0.0004630f * windowHeight));

				pTexture->Release();
			}
		}

		image.Release();

		return result;
	}

	void Manager::LoadIcons()
	{
		unknownKey.Init();

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

ImVec2 ImGui::ButtonIcon(const Icon::ImageData* a_imageData)
{
	const float height = ImGui::GetWindowSize().y;
	const auto  size = ImVec2(a_imageData->width, a_imageData->height);

	ImGui::SetCursorPosY((height - size.y) / 2);
	ImGui::Image(a_imageData->srView, ImVec2(a_imageData->width, a_imageData->height));

	return size;
}

void ImGui::ButtonIcon(const std::set<const Icon::ImageData*>& a_imageData)
{
	BeginGroup();
	for (auto& imageData : a_imageData) {
		auto       pos = ImGui::GetCursorPos();
		const auto size = ImGui::ButtonIcon(imageData);
		ImGui::SetCursorPos({ pos.x + size.x, pos.y });
	}
	EndGroup();
}

void ImGui::ButtonIconWithLabel(const char* a_text, const Icon::ImageData* a_imageData)
{
	ImGui::ButtonIcon(a_imageData);
	ImGui::SameLine();
	ImGui::CenterLabel(a_text, true);
}

void ImGui::ButtonIconWithLabel(const char* a_text, const std::set<const Icon::ImageData*>& a_imageData)
{
	ImGui::ButtonIcon(a_imageData);
	ImGui::SameLine();
	ImGui::CenterLabel(a_text, true);
}
