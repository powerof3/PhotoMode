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
		Unload();
	}

	bool Texture::LoadImpl(float a_scale, const RE::BSGraphics::ScreenSize& a_size, bool a_resetImage)
	{
		bool result = false;

		image = std::make_shared<DirectX::ScratchImage>();
		HRESULT hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_FORCE_RGB | DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, *image);

		if (SUCCEEDED(hr)) {
			if (auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
				const auto  get_resize_dimensions = [&]() -> std::pair<std::size_t, std::size_t> {
					const auto& meta = image->GetMetadata();
					if (a_size.width > 0 && a_size.height > 0 && (a_size.width != meta.width || a_size.height != meta.height)) {
						return { a_size.width, a_size.height };
					}
					if (a_scale != 1.0f) {
						return { static_cast<std::size_t>(meta.width * a_scale), static_cast<std::size_t>(meta.height * a_scale) };
					}
					return { 0, 0 };
				};
				
				if (auto [newWidth, newHeight] = get_resize_dimensions(); newWidth > 0 && newHeight > 0) {
					auto resized = std::make_shared<DirectX::ScratchImage>();
					if (SUCCEEDED(DirectX::Resize(*image->GetImage(0, 0, 0), newWidth, newHeight, DirectX::TEX_FILTER_FANT, *resized))) {
						image = std::move(resized);
					}
				}

				const auto device = reinterpret_cast<ID3D11Device*>(renderer->data.forwarder);
				hr = DirectX::CreateShaderResourceView(device, image->GetImages(), 1, image->GetMetadata(), &srView);
				result = SUCCEEDED(hr);

				size.x = static_cast<float>(image->GetMetadata().width);
				size.y = static_cast<float>(image->GetMetadata().height);

				if (a_resetImage) {
					image.reset();
				}
			}
		}

		return result;
	}

	void Texture::Unload()
	{
		srView.Reset();
		image.reset();
	}

	// Source - https://stackoverflow.com/a/69105584
	// Posted by Aliaksei Luferau
	// Retrieved 2026-08-14, License - CC BY-SA 4.0
	RE::BSGraphics::ScreenSize Texture::GetPNGDimensions(const std::filesystem::path& a_path)
	{
		unsigned char buf[8];

		std::ifstream in(a_path, std::ios::binary);
		if (!in.good()) {
			return {};
		}

		unsigned char signature[8];
		in.read(reinterpret_cast<char*>(signature), 8);
		if (signature[0] != 0x89 || signature[1] != 'P' || signature[2] != 'N' || signature[3] != 'G') {
			return {};
		}

		in.seekg(16);
		in.read(reinterpret_cast<char*>(&buf), 8);

		const auto width = (std::uint32_t(buf[0]) << 24) | (std::uint32_t(buf[1]) << 16) | (std::uint32_t(buf[2]) << 8) | std::uint32_t(buf[3]);
		const auto height = (std::uint32_t(buf[4]) << 24) | (std::uint32_t(buf[5]) << 16) | (std::uint32_t(buf[6]) << 8) | std::uint32_t(buf[7]);

		return { width, height };
	}
}
