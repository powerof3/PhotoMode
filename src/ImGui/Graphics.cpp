#include "Graphics.h"

namespace ImGui
{
	Texture::Texture(std::wstring_view a_path) :
		path(a_path)
	{}

	Texture::Texture(std::wstring_view a_folder, std::wstring_view a_textureName)
	{
		path.append(a_folder).append(a_textureName).append(L".png");
	}

	Texture::~Texture()
	{
		if (srView) {
			srView.Reset();
		}
		if (image) {
			image.reset();
		}
	}

	bool Texture::Load(bool a_resizeToScreenRes)
	{
		bool result = false;

		image = std::make_shared<DirectX::ScratchImage>();
		HRESULT hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, *image);

		if (SUCCEEDED(hr)) {
			if (auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
				if (a_resizeToScreenRes) {
					static auto screenSize = RE::BSGraphics::Renderer::GetScreenSize();
					if (screenSize.height != image->GetMetadata().height && screenSize.width != image->GetMetadata().width) {
						DirectX::ScratchImage tmpImage;
						DirectX::Resize(*image->GetImage(0, 0, 0), screenSize.width, screenSize.height, DirectX::TEX_FILTER_CUBIC, tmpImage);

						image.reset();  // is this needed
						image = std::make_shared<DirectX::ScratchImage>(std::move(tmpImage));
					}
				}

				ComPtr<ID3D11Resource> pTexture{};
				hr = DirectX::CreateTexture((ID3D11Device*)renderer->data.forwarder, image->GetImages(), 1, image->GetMetadata(), &pTexture);

				if (SUCCEEDED(hr)) {
					D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
					srvDesc.Format = image->GetMetadata().format;
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = 1;
					srvDesc.Texture2D.MostDetailedMip = 0;

					hr = ((ID3D11Device*)renderer->data.forwarder)->CreateShaderResourceView(pTexture.Get(), &srvDesc, &srView);
					result = SUCCEEDED(hr);
				}

				size.x = static_cast<float>(image->GetMetadata().width);
				size.y = static_cast<float>(image->GetMetadata().height);

				pTexture.Reset();
			}
		}

		return result;
	}
}
