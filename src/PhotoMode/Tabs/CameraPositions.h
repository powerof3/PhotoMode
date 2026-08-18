#pragma once

template <typename T>
using Result = std::expected<T, std::string>;

namespace PhotoMode
{
	// Camera position data structure
	struct CameraPosition
	{
		CameraPosition(std::string_view a_name = "");

		Result<void> SaveToFile(const std::filesystem::path& a_folder) const;
		Result<void> LoadFromFile(const std::string& a_filename, const std::filesystem::path& a_folder);
		Result<void> DeleteFile(const std::filesystem::path& a_folder) const;
		Result<void> ApplyToCamera() const;

		std::string        GetFilename() const;
		static std::string GenerateTimestamp();

		// members
		std::string           name{};
		std::string           timestamp{};
		RE::NiPoint3          position{};
		float                 fov{ 75.0f };
		float                 viewRoll{ 0.0f };
		RE::NiPoint2          freeCameraRotation{};
		RE::NiPoint3          playerPos{};
		RE::NiPoint3          playerRot{};
		RE::BGSNumericIDIndex cell{};
	};

	class CameraPositions
	{
	public:
		void Draw();

		Result<std::string>                 SaveCameraPositionEntry(std::string_view a_name = "");
		Result<void>                        LoadCameraPositionEntry(const CameraPosition& a_position);
		Result<void>                        DeleteCameraPositionEntry(const CameraPosition& a_position);
		Result<std::vector<CameraPosition>> GetAvailableCameraPositionsList() const;
		CameraPosition                      GetCurrentCameraPosition() const;

	private:
		void                            RefreshCameraPositions();
		std::int32_t                    FindPositionIndexByTimestamp(const std::string& a_timestamp) const;
		const std::vector<std::string>& GetPositionNames();
		void                            LoadSelectedCameraPosition();
		void                            DeleteSelectedCameraPosition();

		void                  InitializeCameraPositionsDirectory();
		void                  EnsureDirectoryInitialized() const;
		std::filesystem::path GetCameraPositionsDirectory() const;

		// members
		std::vector<CameraPosition>   positions{};
		std::vector<std::string>      positionNames{};
		std::int32_t                  selectedPositionIndex{ -1 };
		mutable std::filesystem::path cameraPositionsDirectory{};
		bool                          savePlayerTransform{ true };
	};
}

// glaze
template <>
struct glz::meta<RE::NiPoint3>
{
	using T = RE::NiPoint3;
	static constexpr auto value = array(&T::x, &T::y, &T::z);
};

template <>
struct glz::meta<RE::NiPoint2>
{
	using T = RE::NiPoint2;
	static constexpr auto value = array(&T::x, &T::y);
};

template <>
struct glz::meta<RE::NiMatrix3>
{
	static constexpr auto read = [](RE::NiMatrix3& input, const std::array<float, 3>& vec) {
		input.SetEulerAnglesXYZ(RE::deg_to_rad(vec[0]), RE::deg_to_rad(vec[1]), RE::deg_to_rad(vec[2]));
	};
	static constexpr auto write = [](auto& input) -> auto {
		std::array<float, 3> vec{};
		input.ToEulerAnglesXYZ(vec[0], vec[1], vec[2]);
		vec[0] = RE::rad_to_deg(vec[0]);
		vec[1] = RE::rad_to_deg(vec[1]);
		vec[2] = RE::rad_to_deg(vec[2]);
		return vec;
	};
	static constexpr auto value = custom<read, write>;
};

template <>
struct glz::meta<RE::BGSNumericIDIndex>
{
	using T = RE::BGSNumericIDIndex;
	static constexpr auto value = array(&T::data1, &T::data2, &T::data3);
};

template <>
struct glz::meta<PhotoMode::CameraPosition>
{
	using T = PhotoMode::CameraPosition;
	static constexpr auto value = object(
		"name", &T::name,
		"timestamp", &T::timestamp,
		"position", &T::position,
		"fov", &T::fov,
		"viewRoll", &T::viewRoll,
		"rotation", &T::freeCameraRotation,
		"pcPosition", &T::playerPos,
		"pcRotation", &T::playerRot,
		"pcCell", &T::cell);
};
