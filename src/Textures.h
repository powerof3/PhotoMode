#pragma once

namespace Texture
{
	std::string Sanitize(std::string& a_path);

	void CaptureTexture(const RE::BSGraphics::Renderer* a_this, DirectX::ScratchImage& a_inputImage);
	bool OilPaintingFilter(const DirectX::Image* a_srcImage, std::int32_t a_radius, float a_intensity, DirectX::ScratchImage& a_outImage);
	void CompressTexture(const RE::BSGraphics::Renderer* a_this, const DirectX::ScratchImage& a_inputImage, DirectX::ScratchImage& a_outputImage);
	void SaveToDDS(const DirectX::ScratchImage& a_inputImage, const std::string& a_path);
}
