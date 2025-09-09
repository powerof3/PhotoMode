#include "CameraPositions.h"

#include "ImGui/Renderer.h"
#include "ImGui/Widgets.h"
#include "PhotoMode/Manager.h"
#include "Translation.h"

namespace PhotoMode
{
	CameraPosition::CameraPosition(std::string_view a_name) :
		name(a_name), timestamp(GenerateTimestamp())
	{
	}

	std::string CameraPosition::GetFilename() const
	{
		return std::format("CameraPosition_{}.json", timestamp);
	}

	Result<void> CameraPosition::SaveToFile(const std::filesystem::path& a_folder) const
	{
		std::error_code ec;
		if (!std::filesystem::exists(a_folder, ec)) {
			std::filesystem::create_directories(a_folder, ec);
			if (ec) {
				return Result<void>::Error(std::format("Failed to create camera positions directory '{}': {}", a_folder.string(), ec.message()));
			}
		}

		const std::filesystem::path filePath = a_folder / std::format("CameraPosition_{}.json", timestamp);

		std::string buffer;
		auto        glz_ec = glz::write_file_json(this, filePath.string(), buffer);
		if (glz_ec) {
			return Result<void>::Error(std::format("Failed to write camera position to file '{}': {}", filePath.string(), glz::format_error(glz_ec, buffer)));
		} else {
			logger::debug("Saved camera position to: {}", filePath.string());
			return Result<void>::Ok();
		}
	}

	Result<void> CameraPosition::LoadFromFile(const std::string& a_filename, const std::filesystem::path& a_folder)
	{
		const std::filesystem::path filePath = a_folder / a_filename;

		std::string buffer;
		auto        glz_ec = glz::read_file_json(*this, filePath.string(), buffer);
		if (glz_ec) {
			return Result<void>::Error(std::format("Failed to load camera position file '{}': {}", filePath.string(), glz::format_error(glz_ec, buffer)));
		} else {
			logger::debug("Successfully loaded camera position from: {}", filePath.string());
			return Result<void>::Ok();
		}
	}

	Result<void> CameraPosition::DeleteFile(const std::filesystem::path& a_folder) const
	{
		const std::filesystem::path filePath = a_folder / std::format("CameraPosition_{}.json", timestamp);
		std::error_code             ec;
		if (std::filesystem::remove(filePath, ec)) {
			logger::debug("Deleted camera position file: {}", filePath.string());
			return Result<void>::Ok();
		} else {
			return Result<void>::Error(std::format("Failed to delete camera position file '{}': {}", filePath.string(), ec.message()));
		}
	}

	Result<void> CameraPosition::ApplyToCamera() const
	{
		logger::info("Applying camera position: {}", name);

		const auto pcCamera = RE::PlayerCamera::GetSingleton();
		if (!pcCamera) {
			return Result<void>::Error("Failed to apply camera position: PlayerCamera singleton is null");
		}

		if (!pcCamera->IsInFreeCameraMode()) {
			pcCamera->ToggleFreeCameraMode(false);
		}

		if (pcCamera->IsInFreeCameraMode()) {
			auto freeCameraState = static_cast<RE::FreeCameraState*>(pcCamera->currentState.get());
			if (freeCameraState) {
				freeCameraState->translation = position;

				if (freeCameraRotation.x != 0.0f || freeCameraRotation.y != 0.0f) {
					freeCameraState->rotation.x = freeCameraRotation.x;
					freeCameraState->rotation.y = freeCameraRotation.y;
				}

				pcCamera->worldFOV = fov;
				MANAGER(PhotoMode)->SetViewRoll(viewRoll);

				return Result<void>::Ok();
			}
		}

		return Result<void>::Error("Failed to apply camera position");
	}

	void CameraPositions::Draw()
	{
		static bool positionsLoaded = false;
		if (!positionsLoaded) {
			RefreshCameraPositions();
			positionsLoaded = true;
		}

		ImGui::SeparatorText("$PM_CameraPositions_Header"_T);

		ImGui::PushID("CameraPositions");
		{
			if (ImGui::OutlineButton(std::format("{}##CameraPosSave", "$PM_CameraPositions_Save"_T).c_str())) {
				auto result = SaveCameraPositionEntry();
				if (result) {
					RefreshCameraPositions();
					selectedPositionIndex = FindPositionIndexByTimestamp(result.value);
					RE::PlaySound("UIMenuOK");
				} else {
					logger::error("Failed to save camera position: {}", result.error_message);
				}
			}

			if (!positions.empty()) {
				ImGui::AlignTextToFramePadding();
				ImGui::Text("$PM_CameraPositions_Select"_T);

				ImGui::SameLine();

				ImGui::PushStyleColor(ImGuiCol_NavCursor, ImGui::GetUserStyleColorVec4(ImGui::USER_STYLE::kComboBoxText));
				ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_TextDisabled));

				ImGui::PushItemWidth(-200 * ImGui::Renderer::GetResolutionScale());
				if (ImGui::ComboWithFilter("##CameraPosSelect", &selectedPositionIndex, GetPositionNames())) {
					ImGui::SetKeyboardFocusHere(-1);
				}
				ImGui::PopItemWidth();
				ImGui::PopStyleColor(2);

				ImGui::SameLine();

				ImGui::BeginDisabled(selectedPositionIndex < 0);
				{
					if (ImGui::OutlineButton(std::format("{}##CameraPosLoad", "$PM_CameraPositions_Load"_T).c_str())) {
						LoadSelectedCameraPosition();
					}
					ImGui::SameLine();
					if (ImGui::OutlineButton(std::format("{}##CameraPosDelete", "$PM_CameraPositions_Delete"_T).c_str())) {
						DeleteSelectedCameraPosition();
					}
				}
				ImGui::EndDisabled();
			} else {
				ImGui::Text("$PM_CameraPositions_None"_T);
				ImGui::Text("$PM_CameraPositions_Help"_T);
			}

			ImGui::Spacing();

			if (ImGui::OutlineButton(std::format("{}##CameraPosRefresh", "$PM_CameraPositions_Refresh"_T).c_str())) {
				RefreshCameraPositions();
				selectedPositionIndex = -1;
				RE::PlaySound("UIMenuFocus");
			}
		}
		ImGui::PopID();
	}

	void CameraPositions::RefreshCameraPositions()
	{
		auto result = GetAvailableCameraPositionsList();
		if (result) {
			positions = result.value;
			positionNames.clear();
		} else {
			logger::error("Failed to refresh camera positions: {}", result.error_message);
			positions.clear();
			positionNames.clear();
		}
	}

	std::int32_t CameraPositions::FindPositionIndexByTimestamp(const std::string& a_timestamp) const
	{
		for (std::int32_t i = 0; i < static_cast<std::int32_t>(positions.size()); ++i) {
			if (positions[i].timestamp == a_timestamp) {
				return i;
			}
		}
		return -1;
	}

	const std::vector<std::string>& CameraPositions::GetPositionNames()
	{		
		if (positionNames.empty()) {
			positionNames.reserve(positions.size());
			for (const auto& pos : positions) {
				positionNames.emplace_back(pos.GetFilename());
			}
		}		
		return positionNames;
	}

	void CameraPositions::LoadSelectedCameraPosition()
	{
		if (selectedPositionIndex >= 0 && selectedPositionIndex < static_cast<std::int32_t>(positions.size())) {
			auto result = LoadCameraPositionEntry(positions[selectedPositionIndex]);
			if (result) {
				RE::PlaySound("UIMenuOK");
			} else {
				logger::error("Failed to load camera position: {}", result.error_message);
			}
		}
	}

	void CameraPositions::DeleteSelectedCameraPosition()
	{
		if (selectedPositionIndex >= 0 && selectedPositionIndex < static_cast<std::int32_t>(positions.size())) {
			auto result = DeleteCameraPositionEntry(positions[selectedPositionIndex]);
			if (result) {
				RefreshCameraPositions();
				selectedPositionIndex = -1;
				RE::PlaySound("UIMenuCancel");
			} else {
				logger::error("Failed to delete camera position: {}", result.error_message);
			}
		}
	}

	Result<std::string> CameraPositions::SaveCameraPositionEntry(std::string_view a_name)
	{
		InitializeCameraPositionsDirectory();

		CameraPosition position = GetCurrentCameraPosition();
		position.name = a_name;

		position.timestamp = CameraPosition::GenerateTimestamp();

		auto result = position.SaveToFile(GetCameraPositionsDirectory());
		if (!result) {
			return Result<std::string>::Error(result.error_message);
		}

		logger::debug("Saved camera position {} to {}", position.timestamp, GetCameraPositionsDirectory().string());
		return Result<std::string>::Ok(position.timestamp);
	}

	Result<void> CameraPositions::LoadCameraPositionEntry(const CameraPosition& a_position)
	{
		auto result = a_position.ApplyToCamera();
		if (!result) {
			return Result<void>::Error(result.error_message);
		}
		logger::debug("Loaded camera position {} from {}", a_position.name, GetCameraPositionsDirectory().string());
		return Result<void>::Ok();
	}

	Result<void> CameraPositions::DeleteCameraPositionEntry(const CameraPosition& a_position)
	{
		auto result = a_position.DeleteFile(GetCameraPositionsDirectory());
		if (!result) {
			return Result<void>::Error(result.error_message);
		}
		logger::debug("Deleted camera position {} from {}", a_position.name, GetCameraPositionsDirectory().string());
		return Result<void>::Ok();
	}

	Result<std::vector<CameraPosition>> CameraPositions::GetAvailableCameraPositionsList() const
	{
		std::vector<CameraPosition> positionList;

		EnsureDirectoryInitialized();

		std::error_code ec;
		if (!std::filesystem::exists(GetCameraPositionsDirectory(), ec)) {
			return Result<std::vector<CameraPosition>>::Ok(positionList);
		}

		for (const auto& entry : std::filesystem::directory_iterator(GetCameraPositionsDirectory())) {
			if (entry.is_regular_file()) {
				const auto& path = entry.path();
				if (path.extension() == ".json") {
					std::string    filename = path.filename().string();
					CameraPosition position;
					auto           result = position.LoadFromFile(filename, GetCameraPositionsDirectory());
					if (result) {
						positionList.push_back(position);
					} else {
						logger::warn("Failed to load camera position file {}: {}", filename, result.error_message);
					}
				}
			}
		}

		std::sort(positionList.begin(), positionList.end(), [](const CameraPosition& a, const CameraPosition& b) {
			return a.name < b.name;
		});

		return Result<std::vector<CameraPosition>>::Ok(positionList);
	}

	CameraPosition CameraPositions::GetCurrentCameraPosition() const
	{
		const auto pcCamera = RE::PlayerCamera::GetSingleton();
		if (!pcCamera) {
			return CameraPosition();
		}

		CameraPosition position;

		const auto currentState = pcCamera->currentState;
		if (!currentState || currentState->id != RE::CameraState::kFree) {
			// Return empty position if not in free camera mode
			return position;
		}

		currentState->GetTranslation(position.position);

		auto freeCameraState = static_cast<RE::FreeCameraState*>(currentState.get());
		if (freeCameraState) {
			position.freeCameraRotation.x = freeCameraState->rotation.x;
			position.freeCameraRotation.y = freeCameraState->rotation.y;
		}

		position.fov = pcCamera->worldFOV;
		position.viewRoll = MANAGER(PhotoMode)->GetViewRoll();

		return position;
	}

	void CameraPositions::InitializeCameraPositionsDirectory()
	{
		EnsureDirectoryInitialized();

		std::error_code ec;
		if (!std::filesystem::exists(cameraPositionsDirectory, ec)) {
			logger::debug("Camera positions directory does not exist, creating it... ({})", ec.message());
			if (!std::filesystem::create_directories(cameraPositionsDirectory, ec)) {
				logger::error("Failed to create camera positions directory: {}", ec.message());
			}
		}
	}

	void CameraPositions::EnsureDirectoryInitialized() const
	{
		if (cameraPositionsDirectory.empty()) {
			if (auto directory = logger::log_directory()) {
				directory->remove_filename();
				*directory /= "Saves\\PhotoMode\\CameraPositions"sv;
				logger::info("Camera positions directory: {}", directory->string());
				cameraPositionsDirectory = *directory;
			}
		}
	}

	std::filesystem::path CameraPositions::GetCameraPositionsDirectory() const
	{
		return cameraPositionsDirectory;
	}

	std::string CameraPosition::GenerateTimestamp()
	{
		auto              now = std::chrono::system_clock::now();
		auto              time_t = std::chrono::system_clock::to_time_t(now);
		std::stringstream ss;
		std::tm           tm_buf;
		localtime_s(&tm_buf, &time_t);
		ss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
		return ss.str();
	}
}
