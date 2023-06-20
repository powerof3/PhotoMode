#include "Graphics.h"

std::string Mesh::Sanitize(std::string& a_path)
{
	a_path = clib_util::string::tolower(a_path);

	a_path = srell::regex_replace(a_path, srell::regex("/+|\\\\+"), "\\");
	a_path = srell::regex_replace(a_path, srell::regex("^\\\\+"), "");
	a_path = srell::regex_replace(a_path, srell::regex(R"(.*?[^\s]meshes\\|^meshes\\)", srell::regex::icase), "");

	return a_path;
}

namespace Texture
{
	std::string Sanitize(std::string& a_path)
	{
		a_path = clib_util::string::tolower(a_path);

		a_path = srell::regex_replace(a_path, srell::regex("/+|\\\\+"), "\\");
		a_path = srell::regex_replace(a_path, srell::regex("^\\\\+"), "");
		a_path = srell::regex_replace(a_path, srell::regex(R"(.*?[^\s]textures\\|^textures\\)", srell::regex::icase), "");

		return a_path;
	}

	void CaptureTexture(const RE::BSGraphics::Renderer* a_this, DirectX::ScratchImage& a_inputImage)
	{
		const ComPtr<ID3D11Texture2D>     texture{ a_this->data.renderTargets[RE::RENDER_TARGET::kSCREENSHOT].texture };
		const ComPtr<ID3D11Device>        device{ a_this->data.forwarder };
		const ComPtr<ID3D11DeviceContext> deviceContext{ a_this->data.context };

		// Capture texture;
		DirectX::CaptureTexture(device.Get(), deviceContext.Get(), texture.Get(), a_inputImage);
	}

	// https://www.codeproject.com/Articles/471994/OilPaintEffect
	// https://github.com/aarizhov/DFPerformanceMeter/blob/master/Examples/iOS%20Language%20Performance%20Example/CPU/PureC/OilPaintingC.m
	bool OilPaintingFilter(const DirectX::Image* a_srcImage, const std::int32_t a_radius, const float a_intensity, DirectX::ScratchImage& a_outImage)
	{
		auto hr = a_outImage.InitializeFromImage(*a_srcImage);
		if (FAILED(hr)) {
			return false;
		}

		const auto intensityFactor = 255.0f / std::clamp(a_intensity, 0.0f, 255.0f);

		const std::uint8_t* inPixels = a_srcImage->pixels;
		std::uint8_t*       outPixels = a_outImage.GetPixels();

		const auto& height = a_srcImage->height;
		const auto& width = a_srcImage->width;
		const auto& bytesInARow = a_srcImage->rowPitch;

		auto processRows = [&](const std::size_t startRow, const std::size_t endRow) {
			std::array<std::int32_t, 256> intensityCount{ 0 };
			std::array<std::int32_t, 256> avgR{ 0 };
			std::array<std::int32_t, 256> avgG{ 0 };
			std::array<std::int32_t, 256> avgB{ 0 };

			auto currRowOffset = (startRow * bytesInARow);
			for (auto currRow = startRow; currRow < endRow; currRow++) {
				for (auto currColumn = 0; currColumn < width; currColumn++) {
					// Reset calculations of last pixel.
					intensityCount.fill(0);
					avgR.fill(0);
					avgG.fill(0);
					avgB.fill(0);

					const std::int32_t minY = std::max(-a_radius, -static_cast<std::int32_t>(currRow));
					const std::int32_t maxY = std::min(a_radius, static_cast<std::int32_t>(height - currRow - 1));
					const std::int32_t minX = std::max(-a_radius, -currColumn);
					const std::int32_t maxX = std::min(a_radius, static_cast<std::int32_t>(width - currColumn - 1));

					// Grab luminance values of neighbouring pixels
					for (std::int32_t offsetY = minY; offsetY <= maxY; offsetY++) {
						for (std::int32_t offsetX = minX; offsetX <= maxX; offsetX++) {
							const std::int32_t offset = ((currColumn + offsetX) << 2) + (currRow + offsetY) * bytesInARow;

							const std::uint32_t R = inPixels[offset];
							const std::uint32_t G = inPixels[offset + 1];
							const std::uint32_t B = inPixels[offset + 2];

							// Find intensity of RGB value and apply intensity level.
							const auto currIntensity = static_cast<std::int32_t>(((R + G + B) / 3) / intensityFactor);

							intensityCount[currIntensity]++;
							avgR[currIntensity] += R;
							avgG[currIntensity] += G;
							avgB[currIntensity] += B;
						}
					}

					// Find max intensity
					const std::int32_t maxIntensityIndex = std::distance(intensityCount.begin(), std::ranges::max_element(intensityCount));
					const std::int32_t currMaxIntensityCount = intensityCount[maxIntensityIndex];

					const auto offset = (currColumn << 2) + currRowOffset;
					outPixels[offset] = avgR[maxIntensityIndex] / currMaxIntensityCount;
					outPixels[offset + 1] = avgG[maxIntensityIndex] / currMaxIntensityCount;
					outPixels[offset + 2] = avgB[maxIntensityIndex] / currMaxIntensityCount;
				}
				currRowOffset += bytesInARow;
			}
		};

		const auto numThreads = std::thread::hardware_concurrency();

		std::vector<std::jthread> threads;
		threads.reserve(numThreads);

		const std::size_t rowsPerThread = height / numThreads;

		for (std::size_t i = 0; i < numThreads; ++i) {
			std::size_t startRow = i * rowsPerThread;
			std::size_t endRow = (i + 1) * rowsPerThread;

			threads.emplace_back(std::jthread(processRows, startRow, endRow));
		}

		std::ranges::for_each(threads, [](std::jthread& t) {
			t.join();
		});

		return true;
	}

	void CompressTexture(const RE::BSGraphics::Renderer* a_this, const DirectX::ScratchImage& a_inputImage, DirectX::ScratchImage& a_outputImage)
	{
		// Compress texture
		const ComPtr<ID3D11Device> device{ a_this->data.forwarder };

		auto hr = DirectX::Compress(device.Get(), a_inputImage.GetImages(), 1, a_inputImage.GetMetadata(),
			DXGI_FORMAT_BC7_UNORM,
			DirectX::TEX_COMPRESS_BC7_QUICK,
			0.0f,
			a_outputImage);
		if (FAILED(hr)) {
			logger::info("Failed to compress dds");
		}
	}

	void SaveToDDS(const DirectX::ScratchImage& a_inputImage, const std::string& a_path)
	{
		// Save texture
		const auto wPath = stl::utf8_to_utf16(a_path);
		auto       hr = DirectX::SaveToDDSFile(a_inputImage.GetImages(), 1, a_inputImage.GetMetadata(), DirectX::DDS_FLAGS_NONE, wPath->c_str());
		if (FAILED(hr)) {
			logger::info("Failed to save dds");
		}
	}
}
