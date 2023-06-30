#pragma once

namespace Texture
{
	struct ImageData
	{
		ImageData() = delete;
		ImageData(std::wstring_view a_folder, std::wstring_view a_textureName);
		ImageData(std::wstring_view a_path);

		virtual ~ImageData();

		virtual bool Create(bool a_resizeToScreenRes);

		// members
		std::wstring                           path{};
		ComPtr<ID3D11ShaderResourceView>       srView{ nullptr };
		std::shared_ptr<DirectX::ScratchImage> image{ nullptr };
		ImVec2                                 size{};
	};

	std::string Sanitize(std::string& a_path);

	void AlphaBlendImage(const DirectX::Image* baseImg, const DirectX::Image* overlayImg, DirectX::ScratchImage& resultImg, float intensity);

	bool OilPaintingFilter(const DirectX::Image* a_srcImage, std::int32_t a_radius, float a_intensity, DirectX::ScratchImage& a_outImage);

	void CompressTexture(const RE::BSGraphics::Renderer* a_this, const DirectX::ScratchImage& a_inputImage, DirectX::ScratchImage& a_outputImage);

	void SaveToDDS(const DirectX::ScratchImage& a_inputImage, std::string_view a_path);
	void SaveToPNG(const DirectX::ScratchImage& a_inputImage, std::string_view a_path);
}

namespace Mesh
{
	std::string Sanitize(std::string& a_path);
}
