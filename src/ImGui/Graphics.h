#pragma once

namespace ImGui
{
	struct Texture
	{
		Texture() = delete;
		Texture(std::wstring_view a_folder, std::wstring_view a_textureName);
		Texture(std::wstring_view a_path);

		virtual ~Texture();

		virtual bool Load(bool a_resizeToScreenRes);

		// members
		std::wstring                           path{};
		ComPtr<ID3D11ShaderResourceView>       srView{ nullptr };
		std::shared_ptr<DirectX::ScratchImage> image{ nullptr };
		ImVec2                                 size{};
	};
}
