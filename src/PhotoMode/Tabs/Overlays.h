#pragma once

#include "ImGui/Graphics.h"

namespace PhotoMode
{
	class Overlays
	{
	public:
		void LoadOverlays();
		void RevertOverlays();

		bool HasOverlay() const;

		ImGui::Texture*                   UpdateOverlay();
		std::pair<ImGui::Texture*, float> GetCurrentOverlay() const;

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
		StringMap<StringMap<ImGui::Texture>> overlays{};
		ImGui::Texture*                      cachedOverlay{ nullptr };
		bool                                 updateOverlay{ false };
		bool                                 hasOverlays{ false };

		FileIndex                     folders{};
		Map<std::uint32_t, FileIndex> folderFiles{};

		float alpha{ 1.0f };
	};
}
