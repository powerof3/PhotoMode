#pragma once

#include "Graphics.h"

namespace PhotoMode
{
	class Overlays
	{
	public:
		void LoadOverlays();
		void RevertOverlays();

		Texture::ImageData*                   UpdateOverlay();
		std::pair<Texture::ImageData*, float> GetCurrentOverlay() const;

		void Draw();
		void DrawOverlays();

	private:
		// members
		struct FileIndex
		{
			const std::string& get_file()
			{
				return names[index];
			}

			// members
			std::vector<std::string> names;
			std::uint32_t            index;
		};

		FileIndex& GetFiles()
		{
			return folderFiles[folders.index];
		}

		// folder, file
		StringMap<StringMap<Texture::ImageData>> overlays{};
		Texture::ImageData*                      cachedOverlay{ nullptr };
		bool                                     updateOverlay{ false };
		bool                                     hasOverlays{ false };

		FileIndex                     folders{};
		Map<std::uint32_t, FileIndex> folderFiles{};

		float alpha{ 1.0f };
	};
}
