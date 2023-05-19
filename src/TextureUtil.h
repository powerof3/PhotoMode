#pragma once

namespace Screenshot
{
	struct Paths;
}

namespace Texture
{
	namespace detail
	{
		std::wstring to_wstring(const std::string& a_str);
	}

	std::string Sanitize(std::string& a_path);
	void        CaptureTexture(const RE::BSGraphics::Renderer* a_this, DirectX::ScratchImage& a_inputImage);
	bool        OilPaintingFilter(const DirectX::Image* a_srcImage, std::int32_t a_radius, float a_intensity, DirectX::ScratchImage& a_outImage);
	void        SaveToDDS(const RE::BSGraphics::Renderer* a_this, const DirectX::ScratchImage& a_inputImage, const std::string& a_path);
}
