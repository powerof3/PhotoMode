#include "Bridge.h"
#include "PhotoMode/Manager.h"
#include <Psapi.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <numbers>
#include <limits>
#include <algorithm>
#pragma comment(lib, "Psapi.lib")

namespace
{
	using ConnectFromCameraTools = bool(__cdecl*)();
	using GetDataFromCameraToolsBuffer = std::uint8_t*(__cdecl*)();
	constexpr float kLeftRightScale = 1.0f;
	constexpr float kUpDownScale = 1.0f;
	constexpr float kLeftRightSign = 1.0f;
	constexpr float kUpDownSign = 1.0f;
	constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

	// Set to true only when investigating a regression. Normal builds keep a short status log.
	constexpr bool kVerboseDiagnostics = false;

	std::filesystem::path GetTempFile(std::wstring_view a_name)
	{
		wchar_t buffer[MAX_PATH]{};
		const DWORD length = GetTempPathW(MAX_PATH, buffer);
		if (length == 0 || length >= MAX_PATH) return std::filesystem::path(a_name);
		return std::filesystem::path(buffer) / a_name;
	}

	void WriteFloat(std::uint8_t* b, std::size_t o, float v) { std::memcpy(b + o, &v, sizeof(v)); }
	float ReadFloat(const std::uint8_t* b, std::size_t o)
	{
		float v{};
		std::memcpy(&v, b + o, sizeof(v));
		return v;
	}
	void WriteVec3(std::uint8_t* b, std::size_t o, const RE::NiPoint3& v)
	{
		WriteFloat(b, o + 0, v.x); WriteFloat(b, o + 4, v.y); WriteFloat(b, o + 8, v.z);
	}
	void WriteIdentityMatrix(std::uint8_t* b, std::size_t o)
	{
		float m[16]{}; m[0] = m[5] = m[10] = m[15] = 1.0f; std::memcpy(b + o, m, sizeof(m));
	}

	struct Quaternion
	{
		float x{};
		float y{};
		float z{};
		float w{ 1.0f };
	};

	void WriteQuaternion(std::uint8_t* b, std::size_t o, const Quaternion& q)
	{
		WriteFloat(b, o + 0, q.x);
		WriteFloat(b, o + 4, q.y);
		WriteFloat(b, o + 8, q.z);
		WriteFloat(b, o + 12, q.w);
	}

	Quaternion QuaternionFromBasis(const RE::NiPoint3& right, const RE::NiPoint3& up, const RE::NiPoint3& forward)
	{
		// Skyrim's validated camera basis is left-handed: cross(Right, Up) = -Forward.
		// A rotation quaternion can encode only a proper right-handed rotation (determinant +1).
		// Therefore the quaternion matrix uses camera Backward (-Forward) as its third column,
		// which matches the conventional camera local +Z axis. The explicit Forward vector in the
		// IGCS buffer remains unchanged and continues to be the actual viewing direction.
		const RE::NiPoint3 backward{ -forward.x, -forward.y, -forward.z };
		const float m00 = right.x,   m01 = up.x,   m02 = backward.x;
		const float m10 = right.y,   m11 = up.y,   m12 = backward.y;
		const float m20 = right.z,   m21 = up.z,   m22 = backward.z;

		Quaternion q{};
		const float trace = m00 + m11 + m22;
		if (trace > 0.0f) {
			const float s = std::sqrt(trace + 1.0f) * 2.0f;
			q.w = 0.25f * s;
			q.x = (m21 - m12) / s;
			q.y = (m02 - m20) / s;
			q.z = (m10 - m01) / s;
		} else if (m00 > m11 && m00 > m22) {
			const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
			q.w = (m21 - m12) / s;
			q.x = 0.25f * s;
			q.y = (m01 + m10) / s;
			q.z = (m02 + m20) / s;
		} else if (m11 > m22) {
			const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
			q.w = (m02 - m20) / s;
			q.x = (m01 + m10) / s;
			q.y = 0.25f * s;
			q.z = (m12 + m21) / s;
		} else {
			const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
			q.w = (m10 - m01) / s;
			q.x = (m02 + m20) / s;
			q.y = (m12 + m21) / s;
			q.z = 0.25f * s;
		}
		// Guard against accumulated float error and guarantee a valid unit quaternion.
		const float lenSq = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
		if (lenSq > 1.0e-12f) {
			const float invLen = 1.0f / std::sqrt(lenSq);
			q.x *= invLen; q.y *= invLen; q.z *= invLen; q.w *= invLen;
		} else {
			q = {};
		}
		return q;
	}
	void BasisFromQuaternion(const Quaternion& q, RE::NiPoint3& right, RE::NiPoint3& up, RE::NiPoint3& forward)
	{
		const float xx=q.x*q.x, yy=q.y*q.y, zz=q.z*q.z;
		const float xy=q.x*q.y, xz=q.x*q.z, yz=q.y*q.z;
		const float wx=q.w*q.x, wy=q.w*q.y, wz=q.w*q.z;
		// Standard column-vector rotation matrix. Columns are Right, Up, Forward.
		right   = { 1.0f-2.0f*(yy+zz), 2.0f*(xy+wz),       2.0f*(xz-wy) };
		up      = { 2.0f*(xy-wz),       1.0f-2.0f*(xx+zz), 2.0f*(yz+wx) };
		forward = { 2.0f*(xz+wy),       2.0f*(yz-wx),       1.0f-2.0f*(xx+yy) };
	}

	float Dot3(const RE::NiPoint3& a, const RE::NiPoint3& b)
	{
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}

	void WriteViewMatrix(std::uint8_t* b, std::size_t o, const RE::NiPoint3& r, const RE::NiPoint3& u, const RE::NiPoint3& f, const RE::NiPoint3& p)
	{
		// V15: publish an actual world-to-camera view matrix, not a camera world transform.
		// Skyrim's view basis is left-handed (cross(Right,Up)=-Forward), while the
		// quaternion is encoded with Backward as local +Z. Use the same proper
		// right-handed basis here: Right / Up / Backward.
		const RE::NiPoint3 backward{ -f.x, -f.y, -f.z };
		float m[16]{};
		// Row-major world-to-view matrix. Translation is -dot(axis, cameraPosition).
		m[0]=r.x;        m[1]=u.x;        m[2]=backward.x; m[3]=0.0f;
		m[4]=r.y;        m[5]=u.y;        m[6]=backward.y; m[7]=0.0f;
		m[8]=r.z;        m[9]=u.z;        m[10]=backward.z;m[11]=0.0f;
		m[12]=-Dot3(r,p);m[13]=-Dot3(u,p);m[14]=-Dot3(backward,p);m[15]=1.0f;
		std::memcpy(b + o, m, sizeof(m));
	}
}

namespace PhotoMode::IGCSBridge
{
	void Bridge::Initialize()
	{
		if (initialized) return;
		statusFile = GetTempFile(L"Skyrim_IGCSDOF_status.txt");
		initialized = true;

		// Create/truncate the small status file immediately, before any Photo Mode event.
		{
			std::ofstream output(statusFile, std::ios::out | std::ios::trunc);
			if (output) {
				output << "Skyrim IGCSDOF bridge initialized.\n";
				output << "Path: " << statusFile.string() << '\n';
			}
		}

		logger::info("[Skyrim IGCSDOF] Status file: {}", statusFile.string());
		AppendDiagnostic("Direct IGCS integration initialized; V20 native Photo Mode ZXY matrix basis. Uses the exact FromEulerAnglesZXY matrix convention discovered in Skyrim Photo Mode. Sample rotation remains fixed; V19 toe-in convergence is disabled.");
	}

	void Bridge::OnPhotoModeActivated()
	{
		Initialize();
		photoModeActive = true; sessionActive = false; sessionBase = {}; currentSample = {};
		currentLeftRight = 0.0f; currentUpDown = 0.0f;
		TryConnectToIGCSConnector(); PublishCameraData(true);
		WriteStatus("Photo Mode active; direct camera feed enabled.");
		DumpCameraAndConnectorState("PHOTO MODE ACTIVATED");
	}

	void Bridge::OnFrameUpdate()
	{
		Initialize();
		if (!cameraToolsBuffer) TryConnectToIGCSConnector();

		if constexpr (kVerboseDiagnostics) {
			// Optional developer-only diagnostics. Disabled in normal GitHub builds.
			if ((GetAsyncKeyState(VK_F4) & 0x0001) != 0) {
				if (runtimeTestActive) StopRuntimeDiagnostic(true);
				else RunExhaustiveDiagnostic();
			}

			if (runtimeTestActive) AdvanceRuntimeDiagnostic();

			if ((GetAsyncKeyState(VK_F11) & 0x0001) != 0) {
				DumpCameraAndConnectorState("F11 MANUAL SNAPSHOT");
			}

			if ((GetAsyncKeyState(VK_F10) & 0x0001) != 0) {
				diagLiveCapturePending.store(true, std::memory_order_release);
				AppendDiagnostic("F10 armed: next render-hook pass will be captured.");
			}
		}

		PublishCameraData(photoModeActive);
	}

	void Bridge::OnPhotoModeDeactivated()
	{
		if (runtimeTestActive) StopRuntimeDiagnostic(true);
		if (sessionActive) EndScreenshotSession();
		photoModeActive = false; ClearCameraData();
		WriteStatus("Photo Mode inactive; camera feed unavailable.");
	}

	bool Bridge::CaptureCamera(CameraSnapshot& s) const
	{
		const auto camera = RE::PlayerCamera::GetSingleton();
		if (!camera || !camera->IsInFreeCameraMode() || !camera->currentState) return false;
		const auto state = static_cast<RE::FreeCameraState*>(camera->currentState.get());
		if (!state) return false;
		s.position = state->translation; s.pitch = state->rotation.x; s.yaw = state->rotation.y;
		s.roll = MANAGER(PhotoMode)->GetViewRoll(); s.fov = camera->worldFOV; s.valid = true;
		return true;
	}

	bool Bridge::ApplyCamera(const CameraSnapshot& s) const
	{
		if (!s.valid) return false;
		const auto camera = RE::PlayerCamera::GetSingleton();
		if (!camera || !camera->IsInFreeCameraMode() || !camera->currentState) return false;
		const auto state = static_cast<RE::FreeCameraState*>(camera->currentState.get());
		if (!state) return false;
		state->translation = s.position; state->rotation.x = s.pitch; state->rotation.y = s.yaw; camera->worldFOV = s.fov;
		MANAGER(PhotoMode)->SetViewRoll(s.roll); return true;
	}

	void Bridge::DumpCameraAndConnectorState(std::string_view reason)
	{
		if constexpr (!kVerboseDiagnostics) return;
		Initialize();
		AppendDiagnostic(std::format("================ {} ================", reason));
		const auto camera = RE::PlayerCamera::GetSingleton();
		const auto state = (camera && camera->currentState) ? static_cast<RE::FreeCameraState*>(camera->currentState.get()) : nullptr;
		const std::uintptr_t stateVtable = state ? *reinterpret_cast<const std::uintptr_t*>(state) : 0;
		AppendDiagnostic(std::format(
			"ADDRESSES PlayerCamera=0x{:016X} currentState=0x{:016X} FreeCameraState=0x{:016X} stateVtable=0x{:016X}",
			reinterpret_cast<std::uintptr_t>(camera),
			camera ? reinterpret_cast<std::uintptr_t>(camera->currentState.get()) : 0,
			reinterpret_cast<std::uintptr_t>(state), stateVtable));
		if (camera && state) {
			AppendDiagnostic(std::format(
				"FIELD ADDRESSES translation=0x{:016X} rotation=0x{:016X} worldFOV=0x{:016X}",
				reinterpret_cast<std::uintptr_t>(&state->translation),
				reinterpret_cast<std::uintptr_t>(&state->rotation),
				reinterpret_cast<std::uintptr_t>(&camera->worldFOV)));
			CameraSnapshot snap{};
			if (CaptureCamera(snap)) {
				const Basis b = BuildBasis(snap.pitch, snap.yaw, snap.roll);
				const Quaternion q = QuaternionFromBasis(b.right,b.up,b.forward);
				RE::NiPoint3 qr{},qu{},qb{}; BasisFromQuaternion(q,qr,qu,qb); const RE::NiPoint3 qf{-qb.x,-qb.y,-qb.z};
				AppendDiagnostic(std::format(
					"CAMERA VALUES POS=({:.9f},{:.9f},{:.9f}) PITCH={:.9f} YAW={:.9f} ROLL={:.9f} FOV={:.9f}",
					snap.position.x,snap.position.y,snap.position.z,snap.pitch,snap.yaw,snap.roll,snap.fov));
				AppendDiagnostic(std::format(
					"CALC BASIS R=({:.9f},{:.9f},{:.9f}) U=({:.9f},{:.9f},{:.9f}) F=({:.9f},{:.9f},{:.9f})",
					b.right.x,b.right.y,b.right.z,b.up.x,b.up.y,b.up.z,b.forward.x,b.forward.y,b.forward.z));
				AppendDiagnostic(std::format(
					"QUAT ROUNDTRIP Q=({:.9f},{:.9f},{:.9f},{:.9f}) DOT_R={:.9f} DOT_U={:.9f} DOT_F={:.9f} HAND={:.9f}",
					q.x,q.y,q.z,q.w,Dot(Normalize(qr),Normalize(b.right)),Dot(Normalize(qu),Normalize(b.up)),Dot(Normalize(qf),Normalize(b.forward)),Dot(Cross(qr,qu),qf)));
			}
		}
		AppendDiagnostic(std::format("CONNECTOR BUFFER=0x{:016X}", reinterpret_cast<std::uintptr_t>(cameraToolsBuffer)));
		if (cameraToolsBuffer) {
			const Quaternion q{ReadFloat(cameraToolsBuffer,20),ReadFloat(cameraToolsBuffer,24),ReadFloat(cameraToolsBuffer,28),ReadFloat(cameraToolsBuffer,32)};
			RE::NiPoint3 qr{},qu{},qb{}; BasisFromQuaternion(q,qr,qu,qb); const RE::NiPoint3 qf{-qb.x,-qb.y,-qb.z};
			const RE::NiPoint3 bu{ReadFloat(cameraToolsBuffer,164),ReadFloat(cameraToolsBuffer,168),ReadFloat(cameraToolsBuffer,172)};
			const RE::NiPoint3 br{ReadFloat(cameraToolsBuffer,176),ReadFloat(cameraToolsBuffer,180),ReadFloat(cameraToolsBuffer,184)};
			const RE::NiPoint3 bf{ReadFloat(cameraToolsBuffer,188),ReadFloat(cameraToolsBuffer,192),ReadFloat(cameraToolsBuffer,196)};
			AppendDiagnostic(std::format(
				"BUFFER FLAGS available={} enabled={} movementLocked={} FOV={:.9f} POS=({:.9f},{:.9f},{:.9f})",
				cameraToolsBuffer[0],cameraToolsBuffer[1],cameraToolsBuffer[2],ReadFloat(cameraToolsBuffer,4),ReadFloat(cameraToolsBuffer,8),ReadFloat(cameraToolsBuffer,12),ReadFloat(cameraToolsBuffer,16)));
			AppendDiagnostic(std::format(
				"BUFFER BASIS R=({:.9f},{:.9f},{:.9f}) U=({:.9f},{:.9f},{:.9f}) F=({:.9f},{:.9f},{:.9f}) EULER=({:.9f},{:.9f},{:.9f})",
				br.x,br.y,br.z,bu.x,bu.y,bu.z,bf.x,bf.y,bf.z,ReadFloat(cameraToolsBuffer,200),ReadFloat(cameraToolsBuffer,204),ReadFloat(cameraToolsBuffer,208)));
			AppendDiagnostic(std::format(
				"BUFFER QUAT Q=({:.9f},{:.9f},{:.9f},{:.9f}) QUAT_BASIS_DOTS R={:.9f} U={:.9f} F={:.9f} HAND={:.9f}",
				q.x,q.y,q.z,q.w,Dot(Normalize(qr),Normalize(br)),Dot(Normalize(qu),Normalize(bu)),Dot(Normalize(qf),Normalize(bf)),Dot(Cross(qr,qu),qf)));
			std::string nonZero="BUFFER NONZERO FLOATS:";
			for (std::size_t off=4; off<256; off+=4) {
				const float v=ReadFloat(cameraToolsBuffer,off);
				if (std::isfinite(v) && std::abs(v)>0.0000001f) nonZero += std::format(" [{}]={:.6g}",off,v);
			}
			AppendDiagnostic(nonZero);
			AppendDiagnostic(std::format(
				"BUFFER VIEW36 ROWS: [{:.6f} {:.6f} {:.6f} {:.6f}] [{:.6f} {:.6f} {:.6f} {:.6f}] [{:.6f} {:.6f} {:.6f} {:.6f}] [{:.6f} {:.6f} {:.6f} {:.6f}]",
				ReadFloat(cameraToolsBuffer,36),ReadFloat(cameraToolsBuffer,40),ReadFloat(cameraToolsBuffer,44),ReadFloat(cameraToolsBuffer,48),
				ReadFloat(cameraToolsBuffer,52),ReadFloat(cameraToolsBuffer,56),ReadFloat(cameraToolsBuffer,60),ReadFloat(cameraToolsBuffer,64),
				ReadFloat(cameraToolsBuffer,68),ReadFloat(cameraToolsBuffer,72),ReadFloat(cameraToolsBuffer,76),ReadFloat(cameraToolsBuffer,80),
				ReadFloat(cameraToolsBuffer,84),ReadFloat(cameraToolsBuffer,88),ReadFloat(cameraToolsBuffer,92),ReadFloat(cameraToolsBuffer,96)));
		}
		AppendDiagnostic("============================================================");
	}

	RE::NiPoint3 Bridge::Add(const RE::NiPoint3& a, const RE::NiPoint3& b)
	{ return { a.x+b.x, a.y+b.y, a.z+b.z }; }
	RE::NiPoint3 Bridge::Scale(const RE::NiPoint3& v, float s)
	{ return { v.x*s, v.y*s, v.z*s }; }


	float Bridge::Length(const RE::NiPoint3& v)
	{ return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

	float Bridge::Dot(const RE::NiPoint3& a, const RE::NiPoint3& b)
	{ return a.x*b.x + a.y*b.y + a.z*b.z; }

	RE::NiPoint3 Bridge::Normalize(const RE::NiPoint3& v)
	{
		const float length = Length(v);
		if (length <= 0.000001f) return {};
		return Scale(v, 1.0f / length);
	}

	RE::NiPoint3 Bridge::Cross(const RE::NiPoint3& a, const RE::NiPoint3& b)
	{
		return { a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x };
	}

	bool Bridge::IsFinite(const RE::NiPoint3& v)
	{
		return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
	}

	void Bridge::RunExhaustiveDiagnostic()
	{
		Initialize();
		if (!photoModeActive) {
			AppendDiagnostic("F4 RUNTIME TEST refused: Photo Mode is not active.");
			return;
		}
		if (sessionActive) {
			AppendDiagnostic("F4 RUNTIME TEST refused: end the IGCSDOF session first.");
			return;
		}
		if (!CaptureCamera(runtimeBase)) {
			AppendDiagnostic("F4 RUNTIME TEST refused: Skyrim free camera is unavailable.");
			return;
		}

		runtimeTestActive = true;
		runtimeLogPending = false;
		runtimeFrameCounter = 0;
		runtimeStep = 0;
		runtimePass = 0;
		runtimeFail = 0;
		runtimePose = runtimeBase;
		runtimeExpected = runtimeBase.position;
		AppendDiagnostic("================ F4 VISIBLE RUNTIME CAMERA TEST ================");
		AppendDiagnostic("Real Photo Mode camera movement enabled. F4 again cancels and restores immediately.");
		AppendDiagnostic("Coverage: 9 pitches x 9 aperture positions = 81 visible runtime steps.");
		AppendDiagnostic("Positions: CENTER, +RIGHT, -RIGHT, +UP, -UP, and four diagonals.");
		AppendDiagnostic("The test uses the same BuildBasis and rendered-translation override as real IGCSDOF sessions.");
		AppendDiagnostic("V13 is comparison-only: it does not correct, compensate, refocus or alter the validated camera math.");
	}

	void Bridge::AdvanceRuntimeDiagnostic()
	{
		if (!runtimeTestActive) return;
		constexpr std::uint32_t kHoldFrames = 18;
		if (++runtimeFrameCounter < kHoldFrames) return;
		runtimeFrameCounter = 0;

		constexpr float pitchesDeg[9] = { -89.0f, -75.0f, -60.0f, -30.0f, 0.0f, 30.0f, 60.0f, 75.0f, 89.0f };
		constexpr float offsets[9][2] = {
			{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f },
			{ 0.70710678f, 0.70710678f }, { 0.70710678f, -0.70710678f },
			{ -0.70710678f, 0.70710678f }, { -0.70710678f, -0.70710678f }
		};
		constexpr float kVisibleDistance = 4.0f;
		constexpr std::uint32_t kStepsPerPitch = 9;
		constexpr std::uint32_t kTotalSteps = 81;

		if (runtimeStep >= kTotalSteps) {
			StopRuntimeDiagnostic(false);
			return;
		}

		const std::uint32_t pitchIndex = runtimeStep / kStepsPerPitch;
		const std::uint32_t directionIndex = runtimeStep % kStepsPerPitch;
		runtimePose = runtimeBase;
		runtimePose.pitch = pitchesDeg[pitchIndex] * kDegToRad;
		runtimeExpectedLR = offsets[directionIndex][0] * kVisibleDistance;
		runtimeExpectedUD = offsets[directionIndex][1] * kVisibleDistance;
		const Basis basis = BuildBasis(runtimePose.pitch, runtimePose.yaw, runtimePose.roll);
		runtimeExpected = Add(runtimeBase.position,
			Add(Scale(basis.right, runtimeExpectedLR), Scale(basis.up, runtimeExpectedUD)));
		runtimePose.position = runtimeExpected;
		runtimePose.valid = true;
		ApplyCamera(runtimePose);
		runtimeLogPending = true;
		AppendDiagnostic(std::format(
			"RUNTIME STEP {}/{} REQUEST: pitch={:.1f}deg dir={} LR={:.6f} UD={:.6f} EXPECTED=({:.9f},{:.9f},{:.9f})",
			runtimeStep + 1, kTotalSteps, pitchesDeg[pitchIndex], directionIndex,
			runtimeExpectedLR, runtimeExpectedUD,
			runtimeExpected.x, runtimeExpected.y, runtimeExpected.z));
		++runtimeStep;
	}

	void Bridge::StopRuntimeDiagnostic(bool cancelled)
	{
		if (!runtimeTestActive) return;
		runtimeTestActive = false;
		runtimeLogPending = false;
		ApplyCamera(runtimeBase);
		AppendDiagnostic(std::format(
			"F4 RUNTIME SUMMARY: steps={} PASS={} FAIL={} RESULT={} - camera restored.",
			runtimeStep, runtimePass, runtimeFail,
			cancelled ? "CANCELLED" : (runtimeFail == 0 ? "PASS" : "FAIL")));
		AppendDiagnostic("=================================================================");
	}

	Bridge::Basis Bridge::BuildBasis(float pitch, float yaw, float roll)
	{
		// V20: exact native Photo Mode convention found in SkyrimSE.exe+848826.
		// Photo Mode calls FromEulerAnglesZXY(yaw, pitch, 0). The resulting NiMatrix3
		// is laid out as columns Right / Forward / Up. For the values captured in CE:
		//
		// [ cy,      sy*cp,  sy*sp ]
		// [-sy,      cy*cp,  cy*sp ]
		// [  0,        -sp,     cp ]
		//
		// This reproduces the nine native floats exactly. View roll is then applied
		// around Forward while preserving cross(Right, Forward) = Up.
		const float cp = std::cos(pitch);
		const float sp = std::sin(pitch);
		const float cy = std::cos(yaw);
		const float sy = std::sin(yaw);
		const float cr = std::cos(roll);
		const float sr = std::sin(roll);

		const RE::NiPoint3 nativeRight{ cy, -sy, 0.0f };
		const RE::NiPoint3 nativeForward{ sy * cp, cy * cp, -sp };
		const RE::NiPoint3 nativeUp{ sy * sp, cy * sp, cp };

		Basis result{};
		result.forward = nativeForward;
		result.right = Add(Scale(nativeRight, cr), Scale(nativeUp, sr));
		result.up = Add(Scale(nativeUp, cr), Scale(nativeRight, -sr));
		return result;
	}

	void Bridge::TryConnectToIGCSConnector()
	{
		if (cameraToolsBuffer) return;
		HMODULE modules[1024]{}; DWORD bytesNeeded = 0;
		if (!EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &bytesNeeded)) return;
		const auto count = bytesNeeded / sizeof(HMODULE);
		for (DWORD i=0; i<count; ++i) {
			const auto connect = reinterpret_cast<ConnectFromCameraTools>(GetProcAddress(modules[i], "connectFromCameraTools"));
			const auto getBuffer = reinterpret_cast<GetDataFromCameraToolsBuffer>(GetProcAddress(modules[i], "getDataFromCameraToolsBuffer"));
			if (!connect && !getBuffer) continue;
			if (connect) connect();
			if (getBuffer && (cameraToolsBuffer = getBuffer())) {
				if (!connectorConnectedLogged) { connectorConnectedLogged = true; WriteStatus("Connected directly to IgcsConnector.addon64."); }
				return;
			}
		}
		if (!connectorSearchLogged) { connectorSearchLogged = true; WriteStatus("IgcsConnector exports not found yet; Photo Mode continues normally."); }
	}

	void Bridge::PublishCameraData(bool available)
	{
		if (!cameraToolsBuffer) return;
		CameraSnapshot s{};
		if (sessionActive && currentSample.valid) s = currentSample;
		else if (sessionActive && sessionBase.valid) s = sessionBase;
		else if (!available || !CaptureCamera(s)) { ClearCameraData(); return; }
		const Basis basis = BuildBasis(s.pitch, s.yaw, s.roll);
		std::memset(cameraToolsBuffer, 0, 512);
		cameraToolsBuffer[0]=1; cameraToolsBuffer[1]=1; cameraToolsBuffer[2]=sessionActive ? 1 : 0;
		WriteFloat(cameraToolsBuffer, 4, s.fov); WriteVec3(cameraToolsBuffer, 8, s.position);
		const auto orientation = QuaternionFromBasis(basis.right, basis.up, basis.forward);
		WriteQuaternion(cameraToolsBuffer, 20, orientation);
		WriteViewMatrix(cameraToolsBuffer,36,basis.right,basis.up,basis.forward,s.position);
		WriteIdentityMatrix(cameraToolsBuffer,100); WriteVec3(cameraToolsBuffer,164,basis.up);
		WriteVec3(cameraToolsBuffer,176,basis.right);
		const RE::NiPoint3 connectorBackward{ -basis.forward.x, -basis.forward.y, -basis.forward.z };
		WriteVec3(cameraToolsBuffer,188,connectorBackward);
		WriteFloat(cameraToolsBuffer,200,s.pitch); WriteFloat(cameraToolsBuffer,204,s.yaw); WriteFloat(cameraToolsBuffer,208,s.roll);
	}


	void Bridge::LoadFocusConfiguration()
	{
		// The Connector does not expose its magnifier/focus position through the public ABI.
		// V19 therefore reads a tiny editable file from %TEMP%. Missing values keep safe defaults.
		const auto path = GetTempFile(L"Skyrim_IGCSDOF_focus.ini");
		std::ifstream in(path);
		if (!in) {
			std::ofstream out(path, std::ios::trunc);
			out << "# V19 focus anchor configuration\n"
			    << "FocusScreenX=0.536\n"
			    << "FocusScreenY=0.693\n"
			    << "FocusDistance=100.0\n"
			    << "AspectRatio=1.777777778\n";
			return;
		}
		std::string line;
		while (std::getline(in, line)) {
			const auto eq = line.find('=');
			if (eq == std::string::npos) continue;
			const auto key = line.substr(0, eq);
			try {
				const float value = std::stof(line.substr(eq + 1));
				if (key == "FocusScreenX") focusScreenX = std::clamp(value, 0.0f, 1.0f);
				else if (key == "FocusScreenY") focusScreenY = std::clamp(value, 0.0f, 1.0f);
				else if (key == "FocusDistance") focusDistance = std::max(value, 0.01f);
				else if (key == "AspectRatio") focusAspectRatio = std::max(value, 0.1f);
			} catch (...) {}
		}
	}

	void Bridge::BuildFocusAnchor()
	{
		focusAnchorValid = false;
		if (!sessionBase.valid) return;
		LoadFocusConfiguration();
		const Basis basis = BuildBasis(sessionBase.pitch, sessionBase.yaw, sessionBase.roll);
		const float verticalHalf = std::tan(sessionBase.fov * 0.5f * kDegToRad);
		const float horizontalHalf = verticalHalf * focusAspectRatio;
		const float ndcX = focusScreenX * 2.0f - 1.0f;
		const float ndcY = 1.0f - focusScreenY * 2.0f;
		RE::NiPoint3 ray = Add(basis.forward,
			Add(Scale(basis.right, ndcX * horizontalHalf), Scale(basis.up, ndcY * verticalHalf)));
		ray = Normalize(ray);
		focusAnchorWorld = Add(sessionBase.position, Scale(ray, focusDistance));
		focusAnchorValid = IsFinite(focusAnchorWorld);
		AppendDiagnostic(std::format(
			"V19 FOCUS ANCHOR: screen=({:.6f},{:.6f}) distance={:.6f} aspect={:.6f} world=({:.9f},{:.9f},{:.9f}) valid={}",
			focusScreenX, focusScreenY, focusDistance, focusAspectRatio,
			focusAnchorWorld.x, focusAnchorWorld.y, focusAnchorWorld.z, focusAnchorValid));
	}

	void Bridge::ApplyFocusAnchorConvergence(CameraSnapshot& sample) const
	{
		if (!focusAnchorValid) return;
		const Basis base = BuildBasis(sessionBase.pitch, sessionBase.yaw, sessionBase.roll);
		const float verticalHalf = std::tan(sessionBase.fov * 0.5f * kDegToRad);
		const float horizontalHalf = verticalHalf * focusAspectRatio;
		const float ndcX = focusScreenX * 2.0f - 1.0f;
		const float ndcY = 1.0f - focusScreenY * 2.0f;
		const RE::NiPoint3 localRay = Normalize(Add(base.forward,
			Add(Scale(base.right, ndcX * horizontalHalf), Scale(base.up, ndcY * verticalHalf))));
		const RE::NiPoint3 targetRay = Normalize(RE::NiPoint3{
			focusAnchorWorld.x - sample.position.x,
			focusAnchorWorld.y - sample.position.y,
			focusAnchorWorld.z - sample.position.z });

		// Minimal rotation mapping the original focus ray onto the new target ray.
		RE::NiPoint3 axis = Cross(localRay, targetRay);
		const float sinAngle = Length(axis);
		const float cosAngle = std::clamp(Dot(localRay, targetRay), -1.0f, 1.0f);
		if (sinAngle < 1.0e-8f) return;
		axis = Scale(axis, 1.0f / sinAngle);
		const float angle = std::atan2(sinAngle, cosAngle);
		auto rotate = [&](const RE::NiPoint3& v) {
			const float c = std::cos(angle), ss = std::sin(angle);
			return Add(Add(Scale(v, c), Scale(Cross(axis, v), ss)), Scale(axis, Dot(axis, v) * (1.0f - c)));
		};
		const RE::NiPoint3 newRight = Normalize(rotate(base.right));
		const RE::NiPoint3 newForward = Normalize(rotate(base.forward));
		sample.pitch = std::asin(std::clamp(newForward.z, -1.0f, 1.0f));
		sample.yaw = std::atan2(newForward.x, newForward.y);
		const Basis zeroRoll = BuildBasis(sample.pitch, sample.yaw, 0.0f);
		const float cr = Dot(newRight, zeroRoll.right);
		const float sr = Dot(newRight, zeroRoll.up);
		sample.roll = std::atan2(sr, cr);
	}

	void Bridge::ClearCameraData() { if (cameraToolsBuffer) std::memset(cameraToolsBuffer, 0, 512); }

	std::uint8_t Bridge::StartScreenshotSession(std::uint8_t type)
	{
		Initialize(); CameraSnapshot s{};
		if (!photoModeActive || !CaptureCamera(s)) { WriteStatus("IGCS session start failed: Skyrim free camera is unavailable."); return 1; }
		sessionBase=s; currentSample=s; currentLeftRight=0.0f; currentUpDown=0.0f; sessionActive=true;
		focusAnchorValid = false;
		AppendDiagnostic("V20: V19 focus-anchor toe-in disabled; Pitch/Yaw/Roll remain fixed for every aperture sample.");
		movementSerial.store(0, std::memory_order_release);
		lastLoggedMovementSerial = 0;
		autoPendingSerial = 0;
		autoFinalPending = false;
		autoLoggedSamples = 0;
		maxRenderPositionError = 0.0f;
		maxRenderForwardLeak = 0.0f;
		maxRotationDrift = 0.0f;
		maxFovDrift = 0.0f;
		zeroOffsetSamples = 0;
		nonZeroOffsetSamples = 0;
		minLR = minUD = std::numeric_limits<float>::max();
		maxLR = maxUD = std::numeric_limits<float>::lowest();
		nonZeroLRCommands = 0;
		nonZeroUDCommands = 0;
		PublishCameraData(true);
		WriteStatus(std::format("IGCS session started (type {}).", type));
		const Basis startBasis = BuildBasis(s.pitch, s.yaw, s.roll);
		const auto startQuat = QuaternionFromBasis(startBasis.right, startBasis.up, startBasis.forward);
		AppendDiagnostic(std::format(
			"SESSION BASE: POS=({:.9f},{:.9f},{:.9f}) PITCH={:.9f} YAW={:.9f} ROLL={:.9f} FOV={:.9f}",
			s.position.x,s.position.y,s.position.z,s.pitch,s.yaw,s.roll,s.fov));
		AppendDiagnostic(std::format(
			"SESSION BASIS: RIGHT=({:.9f},{:.9f},{:.9f}) UP=({:.9f},{:.9f},{:.9f}) FORWARD=({:.9f},{:.9f},{:.9f}) QUAT=({:.9f},{:.9f},{:.9f},{:.9f})",
			startBasis.right.x,startBasis.right.y,startBasis.right.z, startBasis.up.x,startBasis.up.y,startBasis.up.z,
			startBasis.forward.x,startBasis.forward.y,startBasis.forward.z, startQuat.x,startQuat.y,startQuat.z,startQuat.w));
		AppendDiagnostic(std::format(
			"V20 NATIVE ZXY MATRIX ROWS: [{:.9f} {:.9f} {:.9f}] [{:.9f} {:.9f} {:.9f}] [{:.9f} {:.9f} {:.9f}]",
			startBasis.right.x, startBasis.forward.x, startBasis.up.x,
			startBasis.right.y, startBasis.forward.y, startBasis.up.y,
			startBasis.right.z, startBasis.forward.z, startBasis.up.z));
		AppendDiagnostic(std::format(
			"V20 ORTHONORMAL CHECK: |R|={:.9f} |F|={:.9f} |U|={:.9f} R.F={:.9f} R.U={:.9f} F.U={:.9f} cross(R,F).U={:.9f}",
			Length(startBasis.right), Length(startBasis.forward), Length(startBasis.up),
			Dot(startBasis.right,startBasis.forward), Dot(startBasis.right,startBasis.up), Dot(startBasis.forward,startBasis.up),
			Dot(Cross(startBasis.right,startBasis.forward),startBasis.up)));
		AppendDiagnostic("V20 BASIS NOTE: axes come from Photo Mode's native FromEulerAnglesZXY matrix: columns are Right / Forward / Up.");
		AppendDiagnostic("V20 ROTATION POLICY: Pitch/Yaw/Roll are fixed for all samples. No toe-in is applied.");
		DumpCameraAndConnectorState("SESSION START SNAPSHOT");
		return 0;
	}

	void Bridge::EndScreenshotSession()
	{
		if (sessionBase.valid) if (const auto camera = RE::PlayerCamera::GetSingleton()) camera->worldFOV = sessionBase.fov;
		AppendDiagnostic(std::format(
			"SESSION SUMMARY: movementCommands={} renderedSamplesLogged={} LR[min={:.9f},max={:.9f},nonZero={}] UD[min={:.9f},max={:.9f},nonZero={}]",
			movementSerial.load(std::memory_order_acquire), autoLoggedSamples,
			minLR, maxLR, nonZeroLRCommands, minUD, maxUD, nonZeroUDCommands));
		AppendDiagnostic(std::format(
			"V16 COMPARISON SUMMARY: zeroOffsetSamples={} nonZeroOffsetSamples={} maxPositionError={:.9f} maxForwardLeak={:.9f} maxRotationDriftRad={:.9f} maxFovDrift={:.9f}",
			zeroOffsetSamples, nonZeroOffsetSamples, maxRenderPositionError, maxRenderForwardLeak, maxRotationDrift, maxFovDrift));
		AppendDiagnostic("V20 RESULT: native ZXY basis was active and sample orientation remained fixed. If focus drift remains, the remaining cause is projection shift/focus-distance rather than camera-axis reconstruction.");
		DumpCameraAndConnectorState("SESSION END SNAPSHOT");
		sessionActive=false; currentLeftRight=0.0f; currentUpDown=0.0f; currentSample={};
		autoFinalPending = false;
		PublishCameraData(photoModeActive); WriteStatus("IGCS session ended; camera restored.");
	}

	void Bridge::MoveCameraMultishot(float lrRaw, float udRaw, float fov, bool fromStart)
	{
		if (!sessionActive || !sessionBase.valid) return;
		if (fromStart) { currentLeftRight=lrRaw; currentUpDown=udRaw; }
		else { currentLeftRight+=lrRaw; currentUpDown+=udRaw; }
		minLR = std::min(minLR, currentLeftRight);
		maxLR = std::max(maxLR, currentLeftRight);
		minUD = std::min(minUD, currentUpDown);
		maxUD = std::max(maxUD, currentUpDown);
		if (std::abs(currentLeftRight) > 0.000001f) ++nonZeroLRCommands;
		if (std::abs(currentUpDown) > 0.000001f) ++nonZeroUDCommands;
		if (fov > 0.0f) if (const auto camera = RE::PlayerCamera::GetSingleton()) camera->worldFOV = fov;
		const Basis basis = BuildBasis(sessionBase.pitch, sessionBase.yaw, sessionBase.roll);
		const float lr=currentLeftRight*kLeftRightScale*kLeftRightSign;
		const float ud=currentUpDown*kUpDownScale*kUpDownSign;
		currentSample=sessionBase;
		currentSample.position=Add(sessionBase.position, Add(Scale(basis.right,lr), Scale(basis.up,ud)));
		// V20 deliberately keeps the camera orientation fixed. Any remaining focus drift
		// is therefore projection/focus-distance related, not toe-in.
		currentSample.valid=true;
		// Critical V16 fix: keep the connector ABI synchronized with the camera
		// position that Skyrim will actually render for this sample.
		PublishCameraData(true);
		movementSerial.fetch_add(1, std::memory_order_release);
	}


	void Bridge::DiagnosticRenderHook(RE::FreeCameraState* state, const RE::NiPoint3& original)
	{
		if constexpr (!kVerboseDiagnostics) return;
		if (!state || !photoModeActive) return;

		auto pressed = [](int key, bool& wasDown) {
			const bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
			const bool rising = down && !wasDown;
			wasDown = down;
			return rising;
		};

		// Automatic capture for the real IGCSDOF session. Log only once for each
		// distinct MoveCameraMultishot command, with a generous cap to avoid a
		// huge file on long renders.
		if (sessionActive && sessionBase.valid && autoLoggedSamples < 128) {
			const auto serial = movementSerial.load(std::memory_order_acquire);
			if (serial != 0 && serial != lastLoggedMovementSerial) {
				lastLoggedMovementSerial = serial;
				autoPendingSerial = serial;
				autoPreHook = original;
				autoState = state->translation;
				autoLR = currentLeftRight;
				autoUD = currentUpDown;
				autoFov = RE::PlayerCamera::GetSingleton() ? RE::PlayerCamera::GetSingleton()->worldFOV : 0.0f;
				if (std::abs(autoLR) <= 0.000001f && std::abs(autoUD) <= 0.000001f) ++zeroOffsetSamples; else ++nonZeroOffsetSamples;
				const Basis basis = BuildBasis(sessionBase.pitch, sessionBase.yaw, sessionBase.roll);
				const float lr = autoLR * kLeftRightScale * kLeftRightSign;
				const float ud = autoUD * kUpDownScale * kUpDownSign;
				autoExpectedFinal = Add(sessionBase.position, Add(Scale(basis.right, lr), Scale(basis.up, ud)));
				autoFinalPending = true;
				const float rotationDrift = std::sqrt(
					(state->rotation.x-sessionBase.pitch)*(state->rotation.x-sessionBase.pitch) +
					(state->rotation.y-sessionBase.yaw)*(state->rotation.y-sessionBase.yaw));
				maxRotationDrift = std::max(maxRotationDrift, rotationDrift);
				maxFovDrift = std::max(maxFovDrift, std::abs(autoFov-sessionBase.fov));
				AppendDiagnostic(std::format(
					"SESSION SAMPLE {} PRE: STATE=({:.9f},{:.9f},{:.9f}) PREHOOK=({:.9f},{:.9f},{:.9f}) STATE_DIFF=({:.9f},{:.9f},{:.9f}) LR={:.9f} UD={:.9f} EXPECTED=({:.9f},{:.9f},{:.9f}) ROT=({:.9f},{:.9f}) ROT_DRIFT={:.9f} FOV={:.9f} FOV_DRIFT={:.9f}",
					serial, autoState.x,autoState.y,autoState.z, autoPreHook.x,autoPreHook.y,autoPreHook.z,
					autoPreHook.x-autoState.x,autoPreHook.y-autoState.y,autoPreHook.z-autoState.z,
					autoLR,autoUD,autoExpectedFinal.x,autoExpectedFinal.y,autoExpectedFinal.z,
					state->rotation.x,state->rotation.y,rotationDrift,autoFov,std::abs(autoFov-sessionBase.fov)));
			}
		}

		if (diagLiveCapturePending.exchange(false, std::memory_order_acq_rel)) {
			const Basis basis = sessionBase.valid ? BuildBasis(sessionBase.pitch, sessionBase.yaw, sessionBase.roll) : Basis{};
			const float lr = currentLeftRight * kLeftRightScale * kLeftRightSign;
			const float ud = currentUpDown * kUpDownScale * kUpDownSign;
			const auto expected = sessionBase.valid ? Add(sessionBase.position, Add(Scale(basis.right,lr), Scale(basis.up,ud))) : original;
			AppendDiagnostic(std::format(
				"F10 CAPTURE PRE: STATE=({:.9f},{:.9f},{:.9f}) PREHOOK=({:.9f},{:.9f},{:.9f}) SESSION={} LR={:.9f} UD={:.9f} EXPECTED=({:.9f},{:.9f},{:.9f})",
				state->translation.x,state->translation.y,state->translation.z,original.x,original.y,original.z,
				sessionActive,currentLeftRight,currentUpDown,expected.x,expected.y,expected.z));
		}

		if (pressed(VK_F6, diagF6Down)) {
			diagBaseRender = original;
			diagBaseState = state->translation;
			diagPitch = state->rotation.x;
			diagYaw = state->rotation.y;
			diagRoll = MANAGER(PhotoMode)->GetViewRoll();
			diagBasis = BuildBasis(diagPitch, diagYaw, diagRoll);
			diagBaseValid = true;
			diagRightValid = false;
			diagUpValid = false;
			diagInjection = DiagnosticInjection::None;
			diagCaptureFinalPending = false;
			AppendDiagnostic(std::format(
				"F6 BASE: STATE=({:.9f},{:.9f},{:.9f}) PREHOOK=({:.9f},{:.9f},{:.9f}) DIFF=({:.9f},{:.9f},{:.9f}) PITCH={:.9f} YAW={:.9f} ROLL={:.9f}",
				diagBaseState.x, diagBaseState.y, diagBaseState.z,
				diagBaseRender.x, diagBaseRender.y, diagBaseRender.z,
				diagBaseRender.x-diagBaseState.x, diagBaseRender.y-diagBaseState.y, diagBaseRender.z-diagBaseState.z,
				diagPitch, diagYaw, diagRoll));
			AppendDiagnostic(std::format(
				"CALC BASIS: RIGHT=({:.9f},{:.9f},{:.9f}) UP=({:.9f},{:.9f},{:.9f}) FORWARD=({:.9f},{:.9f},{:.9f})",
				diagBasis.right.x, diagBasis.right.y, diagBasis.right.z,
				diagBasis.up.x, diagBasis.up.y, diagBasis.up.z,
				diagBasis.forward.x, diagBasis.forward.y, diagBasis.forward.z));
		}

		if (pressed(VK_F7, diagF7Down)) {
			if (!diagBaseValid) {
				AppendDiagnostic("F7 ignored: press F6 first.");
			} else if (sessionActive) {
				AppendDiagnostic("F7 ignored: end the IGCSDOF session first.");
			} else {
				diagInjection = DiagnosticInjection::Right;
				diagPreHook = original;
				diagExpectedFinal = Add(diagBaseRender, Scale(diagBasis.right, 1.0f));
				diagCaptureFinalPending = true;
				AppendDiagnostic("F7 AUTO RIGHT armed: no manual camera movement required.");
			}
		}

		if (pressed(VK_F8, diagF8Down)) {
			if (!diagBaseValid) {
				AppendDiagnostic("F8 ignored: press F6 first.");
			} else if (sessionActive) {
				AppendDiagnostic("F8 ignored: end the IGCSDOF session first.");
			} else {
				diagInjection = DiagnosticInjection::Up;
				diagPreHook = original;
				diagExpectedFinal = Add(diagBaseRender, Scale(diagBasis.up, 1.0f));
				diagCaptureFinalPending = true;
				AppendDiagnostic("F8 AUTO UP armed: no manual camera movement required.");
			}
		}

		if (pressed(VK_F5, diagF5Down)) {
			if (!diagBaseValid || !diagRightValid || !diagUpValid) {
				AppendDiagnostic("F5 REPORT incomplete: press F6, F7, wait briefly, then F8 and wait briefly.");
			} else {
				const auto rightDelta = Add(diagRightRender, Scale(diagBaseRender, -1.0f));
				const auto upDelta = Add(diagUpRender, Scale(diagBaseRender, -1.0f));
				const auto rightNorm = Normalize(rightDelta);
				const auto upNorm = Normalize(upDelta);
				AppendDiagnostic("========== F5 SKYRIM AUTO-INJECTION REPORT ==========");
				AppendDiagnostic(std::format("RIGHT_FINAL=({:.9f},{:.9f},{:.9f}) DELTA=({:.9f},{:.9f},{:.9f}) NORM=({:.9f},{:.9f},{:.9f}) CALC=({:.9f},{:.9f},{:.9f}) DOT={:.9f}",
					diagRightRender.x,diagRightRender.y,diagRightRender.z,
					rightDelta.x,rightDelta.y,rightDelta.z,rightNorm.x,rightNorm.y,rightNorm.z,
					diagBasis.right.x,diagBasis.right.y,diagBasis.right.z,Dot(rightNorm,diagBasis.right)));
				AppendDiagnostic(std::format("UP_FINAL=({:.9f},{:.9f},{:.9f}) DELTA=({:.9f},{:.9f},{:.9f}) NORM=({:.9f},{:.9f},{:.9f}) CALC=({:.9f},{:.9f},{:.9f}) DOT={:.9f}",
					diagUpRender.x,diagUpRender.y,diagUpRender.z,
					upDelta.x,upDelta.y,upDelta.z,upNorm.x,upNorm.y,upNorm.z,
					diagBasis.up.x,diagBasis.up.y,diagBasis.up.z,Dot(upNorm,diagBasis.up)));
				AppendDiagnostic(std::format("ORTHOGONALITY RIGHT_DOT_UP={:.9f}", Dot(rightNorm,upNorm)));
				AppendDiagnostic("======================================================");
			}
		}

	}

	void Bridge::DiagnosticAfterOverride(const RE::NiPoint3& finalTranslation)
	{
		if constexpr (!kVerboseDiagnostics) return;
		if (runtimeTestActive && runtimeLogPending) {
			const auto error = Add(finalTranslation, Scale(runtimeExpected, -1.0f));
			const float errorLength = Length(error);
			const Basis basis = BuildBasis(runtimePose.pitch, runtimePose.yaw, runtimePose.roll);
			const auto delta = Add(finalTranslation, Scale(runtimeBase.position, -1.0f));
			const float measuredLR = Dot(delta, basis.right);
			const float measuredUD = Dot(delta, basis.up);
			const float forwardLeak = Dot(delta, basis.forward);
			const float coordinateScale = std::max({ std::abs(runtimeBase.position.x), std::abs(runtimeBase.position.y), std::abs(runtimeBase.position.z), 1.0f });
			const float adaptiveTolerance = std::max(0.001f, std::nextafter(coordinateScale, std::numeric_limits<float>::infinity()) - coordinateScale) * 2.0f;
			const bool pass = errorLength <= adaptiveTolerance && std::abs(forwardLeak) <= adaptiveTolerance;
			if (pass) ++runtimePass; else ++runtimeFail;
			AppendDiagnostic(std::format(
				"RUNTIME RESULT: FINAL=({:.9f},{:.9f},{:.9f}) measuredLR={:.6f} measuredUD={:.6f} forwardLeak={:.9f} errorLen={:.9f} {}",
				finalTranslation.x, finalTranslation.y, finalTranslation.z, measuredLR, measuredUD, forwardLeak, errorLength, pass ? "PASS" : "FAIL"));
			runtimeLogPending = false;
		}
		if (autoFinalPending) {
			const auto error = Add(finalTranslation, Scale(autoExpectedFinal, -1.0f));
			const float errorLen = Length(error);
			const Basis basis = BuildBasis(sessionBase.pitch, sessionBase.yaw, sessionBase.roll);
			const auto renderedDelta = Add(finalTranslation, Scale(sessionBase.position, -1.0f));
			const float renderedLR = Dot(renderedDelta, basis.right);
			const float renderedUD = Dot(renderedDelta, basis.up);
			const float forwardLeak = Dot(renderedDelta, basis.forward);
			maxRenderPositionError = std::max(maxRenderPositionError, errorLen);
			maxRenderForwardLeak = std::max(maxRenderForwardLeak, std::abs(forwardLeak));
			AppendDiagnostic(std::format(
				"SESSION SAMPLE {} FINAL: FINAL=({:.9f},{:.9f},{:.9f}) ERROR=({:.9f},{:.9f},{:.9f}) ERROR_LEN={:.9f} RENDERED_LR={:.9f} RENDERED_UD={:.9f} FORWARD_LEAK={:.9f}",
				autoPendingSerial,finalTranslation.x,finalTranslation.y,finalTranslation.z,error.x,error.y,error.z,errorLen,renderedLR,renderedUD,forwardLeak));

			// V18 focus-position audit. A focus anchor is placed on the original center ray
			// at each candidate distance. We project that same world point through the
			// actually rendered sample camera and report its screen drift. This directly
			// measures the visible up/down and left/right motion seen in the magnifier.
			constexpr float focusDistances[] = { 1.0f, 5.0f, 10.0f, 25.0f, 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f };
			RECT clientRect{};
			const HWND hwnd = GetForegroundWindow();
			const bool haveViewport = hwnd && GetClientRect(hwnd, &clientRect);
			const float viewportWidth = haveViewport ? static_cast<float>(clientRect.right - clientRect.left) : 0.0f;
			const float viewportHeight = haveViewport ? static_cast<float>(clientRect.bottom - clientRect.top) : 0.0f;
			const float aspect = viewportHeight > 0.0f ? viewportWidth / viewportHeight : 1.0f;
			const float tanHalfHorizontalFov = std::tan((sessionBase.fov * kDegToRad) * 0.5f);
			for (const float d : focusDistances) {
				const auto planePoint = Add(sessionBase.position, Scale(basis.forward, d));
				const float signedDistanceFromSample = Dot(Add(planePoint, Scale(finalTranslation, -1.0f)), basis.forward);
				const float planeDistanceError = signedDistanceFromSample - d;
				const auto sampleToAnchor = Add(planePoint, Scale(finalTranslation, -1.0f));
				const float cameraX = Dot(sampleToAnchor, basis.right);
				const float cameraY = Dot(sampleToAnchor, basis.up);
				const float cameraZ = Dot(sampleToAnchor, basis.forward);
				float ndcX = 0.0f;
				float ndcY = 0.0f;
				float pixelX = 0.0f;
				float pixelY = 0.0f;
				if (std::abs(cameraZ) > 1.0e-6f && tanHalfHorizontalFov > 1.0e-6f) {
					ndcX = (cameraX / cameraZ) / tanHalfHorizontalFov;
					ndcY = (cameraY / cameraZ) * aspect / tanHalfHorizontalFov;
					if (viewportWidth > 0.0f && viewportHeight > 0.0f) {
						pixelX = ndcX * viewportWidth * 0.5f;
						pixelY = -ndcY * viewportHeight * 0.5f;
					}
				}
				AppendDiagnostic(std::format(
					"FOCUS_POSITION sample={} distance={:.3f} anchorWorld=({:.9f},{:.9f},{:.9f}) camera=({:.9f},{:.9f},{:.9f}) ndcDelta=({:.9f},{:.9f}) pixelDelta=({:.3f},{:.3f}) viewport=({:.0f}x{:.0f})",
					autoPendingSerial,d,planePoint.x,planePoint.y,planePoint.z,cameraX,cameraY,cameraZ,ndcX,ndcY,pixelX,pixelY,viewportWidth,viewportHeight));
				AppendDiagnostic(std::format(
					"FOCUS_PLANE sample={} referenceDistance={:.3f} signedDistanceFromSample={:.9f} distanceError={:.9f} planeNormalDotForward={:.9f}",
					autoPendingSerial,d,signedDistanceFromSample,planeDistanceError,Dot(basis.forward,basis.forward)));
			}
			// Compare all orientation representations published to the connector during the real render.
			if (cameraToolsBuffer) {
				const Quaternion q{ReadFloat(cameraToolsBuffer,20),ReadFloat(cameraToolsBuffer,24),ReadFloat(cameraToolsBuffer,28),ReadFloat(cameraToolsBuffer,32)};
				RE::NiPoint3 qr{},qu{},qb{}; BasisFromQuaternion(q,qr,qu,qb); const RE::NiPoint3 qf{-qb.x,-qb.y,-qb.z};
				const RE::NiPoint3 bu{ReadFloat(cameraToolsBuffer,164),ReadFloat(cameraToolsBuffer,168),ReadFloat(cameraToolsBuffer,172)};
				const RE::NiPoint3 br{ReadFloat(cameraToolsBuffer,176),ReadFloat(cameraToolsBuffer,180),ReadFloat(cameraToolsBuffer,184)};
				const RE::NiPoint3 bf{ReadFloat(cameraToolsBuffer,188),ReadFloat(cameraToolsBuffer,192),ReadFloat(cameraToolsBuffer,196)};
				AppendDiagnostic(std::format(
					"ORIENTATION_COMPARE sample={} CALC_vs_BUFFER R={:.9f} U={:.9f} F={:.9f} QUAT_vs_BUFFER R={:.9f} U={:.9f} F={:.9f} BUFFER_HAND={:.9f}",
					autoPendingSerial,Dot(Normalize(basis.right),Normalize(br)),Dot(Normalize(basis.up),Normalize(bu)),Dot(Normalize(basis.forward),Normalize(bf)),
					Dot(Normalize(qr),Normalize(br)),Dot(Normalize(qu),Normalize(bu)),Dot(Normalize(qf),Normalize(bf)),Dot(Cross(br,bu),bf)));
			}
			autoFinalPending = false;
			++autoLoggedSamples;
		}

		if (!diagCaptureFinalPending || diagInjection == DiagnosticInjection::None) return;

		const auto error = Add(finalTranslation, Scale(diagExpectedFinal, -1.0f));
		const auto delta = Add(finalTranslation, Scale(diagBaseRender, -1.0f));
		if (diagInjection == DiagnosticInjection::Right) {
			diagRightRender = finalTranslation;
			diagRightValid = true;
			AppendDiagnostic(std::format(
				"F7 AUTO RIGHT RESULT: PREHOOK=({:.9f},{:.9f},{:.9f}) EXPECTED=({:.9f},{:.9f},{:.9f}) FINAL=({:.9f},{:.9f},{:.9f}) DELTA=({:.9f},{:.9f},{:.9f}) ERROR=({:.9f},{:.9f},{:.9f})",
				diagPreHook.x,diagPreHook.y,diagPreHook.z,
				diagExpectedFinal.x,diagExpectedFinal.y,diagExpectedFinal.z,
				finalTranslation.x,finalTranslation.y,finalTranslation.z,
				delta.x,delta.y,delta.z,error.x,error.y,error.z));
		} else {
			diagUpRender = finalTranslation;
			diagUpValid = true;
			AppendDiagnostic(std::format(
				"F8 AUTO UP RESULT: PREHOOK=({:.9f},{:.9f},{:.9f}) EXPECTED=({:.9f},{:.9f},{:.9f}) FINAL=({:.9f},{:.9f},{:.9f}) DELTA=({:.9f},{:.9f},{:.9f}) ERROR=({:.9f},{:.9f},{:.9f})",
				diagPreHook.x,diagPreHook.y,diagPreHook.z,
				diagExpectedFinal.x,diagExpectedFinal.y,diagExpectedFinal.z,
				finalTranslation.x,finalTranslation.y,finalTranslation.z,
				delta.x,delta.y,delta.z,error.x,error.y,error.z));
		}
		diagCaptureFinalPending = false;
		diagInjection = DiagnosticInjection::None;
	}

	bool Bridge::OverrideRenderedTranslation(RE::NiPoint3& t) const
	{
		if (runtimeTestActive && runtimePose.valid) {
			t = runtimeExpected;
			return true;
		}
		if (!sessionActive && diagCaptureFinalPending && diagInjection != DiagnosticInjection::None) {
			t = diagExpectedFinal;
			return true;
		}
		if (!sessionActive || !sessionBase.valid) return false;
		const Basis basis = BuildBasis(sessionBase.pitch, sessionBase.yaw, sessionBase.roll);
		const float lr=currentLeftRight*kLeftRightScale*kLeftRightSign;
		const float ud=currentUpDown*kUpDownScale*kUpDownSign;
		t=Add(sessionBase.position, Add(Scale(basis.right,lr), Scale(basis.up,ud)));
		return true;
	}

	void Bridge::MoveCameraPanorama(float angle)
	{
		if (!sessionActive || !sessionBase.valid) return;
		CameraSnapshot s=sessionBase; s.yaw += angle*kDegToRad; s.valid=true;
		if (ApplyCamera(s)) currentSample=s;
	}

	void Bridge::LogHookInstallation(std::uintptr_t vtable, std::size_t index)
	{
		Initialize(); WriteStatus(std::format("GetFreeCameraTranslation hook installed: vtable=0x{:016X}, index=0x{:X}", vtable, index));
	}

	void Bridge::WriteStatus(std::string_view text) const
	{
		std::ofstream output(statusFile, std::ios::app); if (output) output << text << '\n';
		logger::info("[Skyrim IGCSDOF Direct] {}", text);
	}

	void Bridge::AppendDiagnostic(std::string_view text) const
	{
		if constexpr (!kVerboseDiagnostics) return;
		std::ofstream output(statusFile, std::ios::app);
		if (output) output << text << '\n';
		logger::info("[Skyrim IGCSDOF DIAG] {}", text);
	}
}

extern "C" __declspec(dllexport) std::uint8_t __cdecl IGCS_StartScreenshotSession(std::uint8_t t)
{ return PhotoMode::IGCSBridge::Bridge::GetSingleton()->StartScreenshotSession(t); }
extern "C" __declspec(dllexport) void __cdecl IGCS_EndScreenshotSession()
{ PhotoMode::IGCSBridge::Bridge::GetSingleton()->EndScreenshotSession(); }
extern "C" __declspec(dllexport) void __cdecl IGCS_MoveCameraPanorama(float a)
{ PhotoMode::IGCSBridge::Bridge::GetSingleton()->MoveCameraPanorama(a); }
extern "C" __declspec(dllexport) void __cdecl IGCS_MoveCameraMultishot(float lr, float ud, float fov, bool fromStart)
{ PhotoMode::IGCSBridge::Bridge::GetSingleton()->MoveCameraMultishot(lr, ud, fov, fromStart); }
