#pragma once

namespace PhotoMode::IGCSBridge
{
	class Bridge : public REX::Singleton<Bridge>
	{
	public:
		void Initialize();
		void OnPhotoModeActivated();
		void OnFrameUpdate();
		void OnPhotoModeDeactivated();
		void RunExhaustiveDiagnostic();
		void AdvanceRuntimeDiagnostic();
		void StopRuntimeDiagnostic(bool a_cancelled);
		void DumpCameraAndConnectorState(std::string_view a_reason);

		void LogHookInstallation(std::uintptr_t a_vtable, std::size_t a_index);
		void DiagnosticRenderHook(RE::FreeCameraState* a_state, const RE::NiPoint3& a_originalTranslation);
		void DiagnosticAfterOverride(const RE::NiPoint3& a_finalTranslation);
		bool OverrideRenderedTranslation(RE::NiPoint3& a_translation) const;

		std::uint8_t StartScreenshotSession(std::uint8_t a_sessionType);
		void         EndScreenshotSession();
		void         MoveCameraMultishot(float a_leftRight, float a_upDown, float a_fov, bool a_fromStart);
		void         MoveCameraPanorama(float a_stepAngle);

	private:
		struct CameraSnapshot
		{
			RE::NiPoint3 position{};
			float        pitch{ 0.0f };
			float        yaw{ 0.0f };
			float        roll{ 0.0f };
			float        fov{ 70.0f };
			bool         valid{ false };
		};

		struct Basis
		{
			RE::NiPoint3 right{};
			RE::NiPoint3 up{};
			RE::NiPoint3 forward{};
		};

		bool CaptureCamera(CameraSnapshot& a_snapshot) const;
		bool ApplyCamera(const CameraSnapshot& a_snapshot) const;
		void TryConnectToIGCSConnector();
		void PublishCameraData(bool a_available);
		void ClearCameraData();
		void LoadFocusConfiguration();
		void BuildFocusAnchor();
		void ApplyFocusAnchorConvergence(CameraSnapshot& a_sample) const;

		static Basis BuildBasis(float a_pitch, float a_yaw, float a_roll);

		void                WriteStatus(std::string_view a_text) const;
		void                AppendDiagnostic(std::string_view a_text) const;
		static RE::NiPoint3 Normalize(const RE::NiPoint3& a_value);
		static bool         IsFinite(const RE::NiPoint3& a_value);

		std::filesystem::path statusFile;
		std::uint8_t*         cameraToolsBuffer{ nullptr };
		bool                  initialized{ false };
		bool                  connectorSearchLogged{ false };
		bool                  connectorConnectedLogged{ false };
		bool                  photoModeActive{ false };
		bool                  sessionActive{ false };
		CameraSnapshot        sessionBase{};
		CameraSnapshot        currentSample{};
		float                 currentLeftRight{ 0.0f };
		float                 currentUpDown{ 0.0f };

		// Legacy V19 focus-anchor fields retained for diagnostics/rollback; V20 does not apply toe-in.
		float        focusScreenX{ 0.536f };
		float        focusScreenY{ 0.693f };
		float        focusDistance{ 100.0f };
		float        focusAspectRatio{ 1600.0f / 900.0f };
		RE::NiPoint3 focusAnchorWorld{};
		bool         focusAnchorValid{ false };

		// Visible F4 runtime diagnostic. It drives the actual Photo Mode camera
		// through real orientations and aperture offsets, then restores it.
		bool           runtimeTestActive{ false };
		bool           runtimeLogPending{ false };
		std::uint32_t  runtimeFrameCounter{ 0 };
		std::uint32_t  runtimeStep{ 0 };
		std::uint32_t  runtimePass{ 0 };
		std::uint32_t  runtimeFail{ 0 };
		CameraSnapshot runtimeBase{};
		CameraSnapshot runtimePose{};
		RE::NiPoint3   runtimeExpected{};
		float          runtimeExpectedLR{ 0.0f };
		float          runtimeExpectedUD{ 0.0f };

		bool             diagF6Down{ false };
		bool             diagF7Down{ false };
		bool             diagF8Down{ false };
		bool             diagF5Down{ false };
		bool             diagF10Down{ false };
		bool             diagF9Down{ false };
		bool             diagF4Down{ false };
		std::atomic_bool diagLiveCapturePending{ false };

		// Automatic real-session diagnostics. A command serial changes once per
		// IGCS_MoveCameraMultishot call; the render hook logs each serial once.
		std::atomic<std::uint64_t> movementSerial{ 0 };
		std::uint64_t              lastLoggedMovementSerial{ 0 };
		std::uint64_t              autoPendingSerial{ 0 };
		bool                       autoFinalPending{ false };
		std::uint32_t              autoLoggedSamples{ 0 };
		RE::NiPoint3               autoExpectedFinal{};
		RE::NiPoint3               autoPreHook{};
		RE::NiPoint3               autoState{};
		float                      autoLR{ 0.0f };
		float                      autoUD{ 0.0f };
		float                      autoFov{ 0.0f };
		float                      maxRenderPositionError{ 0.0f };
		float                      maxRenderForwardLeak{ 0.0f };
		float                      maxRotationDrift{ 0.0f };
		float                      maxFovDrift{ 0.0f };
		std::uint64_t              zeroOffsetSamples{ 0 };
		std::uint64_t              nonZeroOffsetSamples{ 0 };
		float                      minLR{ 0.0f };
		float                      maxLR{ 0.0f };
		float                      minUD{ 0.0f };
		float                      maxUD{ 0.0f };
		std::uint64_t              nonZeroLRCommands{ 0 };
		std::uint64_t              nonZeroUDCommands{ 0 };
		bool                       diagBaseValid{ false };
		bool                       diagRightValid{ false };
		bool                       diagUpValid{ false };
		enum class DiagnosticInjection : std::uint8_t
		{
			None,
			Right,
			Up
		};
		DiagnosticInjection diagInjection{ DiagnosticInjection::None };
		bool                diagCaptureFinalPending{ false };
		RE::NiPoint3        diagExpectedFinal{};
		RE::NiPoint3        diagPreHook{};
		RE::NiPoint3        diagBaseRender{};
		RE::NiPoint3        diagBaseState{};
		RE::NiPoint3        diagRightRender{};
		RE::NiPoint3        diagUpRender{};
		Basis               diagBasis{};
		float               diagPitch{ 0.0f };
		float               diagYaw{ 0.0f };
		float               diagRoll{ 0.0f };
	};
}
