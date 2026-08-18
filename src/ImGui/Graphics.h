#pragma once

namespace ImGui
{
	struct Texture
	{
		Texture() = delete;
		Texture(std::wstring_view a_folder, std::wstring_view a_textureName);
		Texture(std::wstring_view a_path);

		virtual ~Texture();

		bool LoadImpl(float a_scale, const RE::BSGraphics::ScreenSize& a_size = {}, bool a_resetImage = true);
		void Unload();
		bool IsLoaded() const { return srView != nullptr; }

		static RE::BSGraphics::ScreenSize GetPNGDimensions(const std::filesystem::path& a_path);

		// members
		std::wstring                           path{};
		ComPtr<ID3D11ShaderResourceView>       srView{ nullptr };
		std::shared_ptr<DirectX::ScratchImage> image{ nullptr };
		ImVec2                                 size{};
	};
}
