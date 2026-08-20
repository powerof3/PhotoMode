#pragma once

namespace Texture
{
	namespace detail
	{
		template <class F>
		void ParallelizeRows(std::size_t a_height, F&& a_func)
		{
			const auto        numThreads = std::thread::hardware_concurrency();
			const std::size_t rowsPerThread = a_height / numThreads;

			std::vector<std::jthread> threads;
			threads.reserve(numThreads);

			for (std::size_t i = 0; i < numThreads; ++i) {
				std::size_t startRow = i * rowsPerThread;
				std::size_t endRow = (i == numThreads - 1) ? a_height : (i + 1) * rowsPerThread;
				threads.emplace_back(a_func, startRow, endRow);
			}
		}
	}

	std::string Sanitize(std::string& a_path);

	void AlphaBlendImage(const DirectX::Image* a_baseImg, const DirectX::Image* a_overlayImg, DirectX::ScratchImage& a_outImage, float a_intensity);

	bool OilPaintingFilter(const DirectX::Image* a_srcImage, std::int32_t a_radius, float a_intensity, DirectX::ScratchImage& a_outImage);

	bool CompressTexture(const RE::BSGraphics::Renderer* a_this, const DirectX::ScratchImage& a_inputImage, DirectX::ScratchImage& a_outputImage);

	bool SaveToDDS(const DirectX::ScratchImage& a_inputImage, std::string_view a_path);
	bool SaveToPNG(const DirectX::ScratchImage& a_inputImage, std::string_view a_path, bool a_forceSRGB);
}

namespace Mesh
{
	std::string Sanitize(std::string& a_path);
}
