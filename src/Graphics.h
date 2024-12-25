#pragma once

namespace Texture
{
	std::string Sanitize(std::string& a_path);

	void AlphaBlendImage(const DirectX::Image* a_baseImg, const DirectX::Image* a_overlayImg, DirectX::ScratchImage& a_outImage, float a_intensity);

	bool OilPaintingFilter(const DirectX::Image* a_srcImage, std::int32_t a_radius, float a_intensity, DirectX::ScratchImage& a_outImage);

	void CompressTexture(const RE::BSGraphics::Renderer* a_this, const DirectX::ScratchImage& a_inputImage, DirectX::ScratchImage& a_outputImage);

	void SaveToDDS(const DirectX::ScratchImage& a_inputImage, std::string_view a_path);
	void SaveToPNG(const DirectX::ScratchImage& a_inputImage, std::string_view a_path);
}

namespace Mesh
{
	std::string Sanitize(std::string& a_path);
}
