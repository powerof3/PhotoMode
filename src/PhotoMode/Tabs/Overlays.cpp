#include "Overlays.h"

#include "ImGui/Widgets.h"

namespace PhotoMode
{
	void Overlays::LoadOverlays()
	{
		const std::filesystem::path overlaysPath(R"(Data\Interface\PhotoMode\Overlays)");

		if (!std::filesystem::exists(overlaysPath)) {
			return;
		}

		std::map<std::string, std::vector<std::pair<std::wstring, std::string>>> imagePaths;

		std::string currentSubFolder;
		const auto  iterator = std::filesystem::recursive_directory_iterator(overlaysPath);
		for (const auto& entry : iterator) {
			if (entry.exists()) {
				if (entry.is_directory()) {
					currentSubFolder = entry.path().filename().string();
				} else if (entry.is_regular_file()) {
					if (const auto& path = entry.path(); !path.empty() && path.extension() == ".png") {
						auto fileName = path.filename().string();
						imagePaths[currentSubFolder].push_back({ path.wstring(), fileName.erase(fileName.size() - 4) });
					}
				}
			}
		}

		for (auto& [folder, files] : imagePaths) {
			for (auto& [path, fileName] : files) {
				overlays[folder].emplace(fileName, ImGui::Texture(path));
			}
		}

		hasOverlays = !overlays.empty();

		if (hasOverlays) {
			for (auto& [folder, files] : overlays) {
				for (auto& [path, overlay] : files) {
					overlay.Load(true);
				}
			}

			std::uint32_t index = 0;

			for (auto& [folder, files] : imagePaths) {
				folders.names.push_back(folder);

				folderFiles[index].names.push_back("$PM_NONE"_T);
				for (auto& fileName : files | std::views::values) {
					folderFiles[index].names.push_back(fileName);
				}

				index++;
			}
		}
	}

	void Overlays::RevertOverlays()
	{
		cachedOverlay = nullptr;
		updateOverlay = false;

		folders.index = 0;
		for (auto& [index, files] : folderFiles) {
			files.index = 0;
		}

		alpha = 1.0f;
	}

	ImGui::Texture* Overlays::UpdateOverlay()
	{
		if (const auto it = overlays.find(folders.get_file()); it != overlays.end()) {
			const auto file = GetFiles().get_file();
			if (const auto fileIt = it->second.find(file); fileIt != it->second.end()) {
				return &fileIt->second;
			}
		}

		return nullptr;
	}

	std::pair<ImGui::Texture*, float> Overlays::GetCurrentOverlay() const
	{
		return { cachedOverlay, alpha };
	}

	void Overlays::Draw()
	{
		if (!hasOverlays) {
			ImGui::TextUnformatted("$PM_NoOverlaysInstalled"_T);
		} else {
			if (ImGui::EnumSlider("$PM_Category"_T, &folders.index, folders.names, false)) {
				// reset others to NONE
				for (auto& [index, files] : folderFiles) {
					if (index != folders.index) {
						files.index = 0;
					}
				}
				cachedOverlay = nullptr;
				updateOverlay = false;
				alpha = 1.0f;
			}
			ImGui::Indent();
			{
				if (ImGui::EnumSlider("$PM_Overlay"_T, &GetFiles().index, GetFiles().names, false)) {
					updateOverlay = true;
					alpha = 1.0f;
				}
			}
			ImGui::Unindent();
			ImGui::Slider("$PM_Intensity"_T, &alpha, 0.0f, 1.0f);
		}
	}

	void Overlays::DrawOverlays()
	{
		const auto drawList = ImGui::GetBackgroundDrawList();
		const auto size = ImGui::GetWindowSize();

		constexpr auto topLeft = ImVec2(0.0f, 0.0f);
		const auto static bottomRight = ImVec2(size.x, size.y);

		if (updateOverlay) {
			updateOverlay = false;
			cachedOverlay = UpdateOverlay();
		}

		if (cachedOverlay) {
			drawList->AddImage((ImTextureID)cachedOverlay->srView.Get(), topLeft, bottomRight, ImVec2(0, 0), ImVec2(1, 1), static_cast<ImU32>(ImColor(1.0f, 1.0f, 1.0f, alpha)));
		}
	}
}
