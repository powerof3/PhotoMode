#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

// Simple Result type for consistent error handling
template<typename T>
struct Result {
	bool success;
	T value;
	std::string error_message;

	Result(bool s, T v, std::string msg = "") : success(s), value(v), error_message(msg) {}

	static Result<T> Ok(T v) { return Result(true, v); }
	static Result<T> Error(std::string msg) { return Result(false, T{}, msg); }

	explicit operator bool() const { return success; }
};

// Specialization for void
template<>
struct Result<void> {
	bool success;
	std::string error_message;

	Result(bool s, std::string msg = "") : success(s), error_message(msg) {}

	static Result<void> Ok() { return Result(true); }
	static Result<void> Error(std::string msg) { return Result(false, msg); }

	explicit operator bool() const { return success; }
};

namespace PhotoMode
{
	// Camera position data structure
	struct CameraPosition
	{
		CameraPosition(std::string_view a_name = "");

		RE::NiPoint3 position{};
		float fov{ 75.0f };
		float freeCameraRotationX{ 0.0f };
		float freeCameraRotationY{ 0.0f };

		std::string name{};
		std::string timestamp{};

		Result<void> SaveToFile(const std::filesystem::path& a_folder) const;
		Result<void> LoadFromFile(const std::string& a_filename, const std::filesystem::path& a_folder);
		Result<void> DeleteFile(const std::filesystem::path& a_folder) const;
		Result<void> ApplyToCamera() const;

		std::string GetFilename() const;
		static std::string GenerateTimestamp();

		void SerializeToJson(nlohmann::json& json) const;
		void DeserializeFromJson(const nlohmann::json& json);
	};
	class CameraPositions
	{
	public:
		void Draw();

		Result<std::string> SaveCameraPositionEntry(std::string_view a_name = "");
		Result<void> LoadCameraPositionEntry(const CameraPosition& a_position);
		Result<void> DeleteCameraPositionEntry(const CameraPosition& a_position);
		Result<std::vector<CameraPosition>> GetAvailableCameraPositionsList() const;
		CameraPosition GetCurrentCameraPosition() const;

	private:
		void RefreshCameraPositions();
		int FindPositionIndexByTimestamp(const std::string& a_timestamp) const;
		std::vector<std::string> BuildPositionNames() const;
		void LoadSelectedCameraPosition();
		void DeleteSelectedCameraPosition();

		void InitializeCameraPositionsDirectory();
		void EnsureDirectoryInitialized() const;
		std::filesystem::path GetCameraPositionsDirectory() const;

		std::vector<CameraPosition> positions{};
		int selectedPositionIndex{ -1 };
		mutable std::filesystem::path cameraPositionsDirectory{};
	};
}
