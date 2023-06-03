#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <codecvt>
#include <wrl/client.h>

#include <ClibUtil/RNG.hpp>
#include <ClibUtil/hotkeys.hpp>
#include <ClibUtil/simpleINI.hpp>
#include <ClibUtil/singleton.hpp>
#include <ClibUtil/string.hpp>
#include <DirectXMath.h>
#include <DirectXTex.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <ankerl/unordered_dense.h>
#include <rapidfuzz/rapidfuzz_all.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <srell.hpp>
#include <xbyak/xbyak.h>

#define DLLEXPORT __declspec(dllexport)

using namespace std::literals;

namespace logger = SKSE::log;
namespace ini = clib_util::ini;

using namespace clib_util::singleton;
using namespace clib_util::hotkeys;

using RNG = clib_util::RNG;
using EventResult = RE::BSEventNotifyControl;
using KEY = RE::BSWin32KeyboardDevice::Key;
using GAMEPAD_DIRECTX = RE::BSWin32GamepadDevice::Key;
using GAMEPAD_ORBIS = RE::BSPCOrbisGamepadDevice::Key;

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

template <class K, class D>
using Map = ankerl::unordered_dense::segmented_map<K, D>;

struct string_hash
{
	using is_transparent = void;  // enable heterogeneous overloads
	using is_avalanching = void;  // mark class as high quality avalanching hash

	[[nodiscard]] std::uint64_t operator()(std::string_view str) const noexcept
	{
		return ankerl::unordered_dense::hash<std::string_view>{}(str);
	}
};

template <class D>
using StringMap = ankerl::unordered_dense::segmented_map<std::string, D, string_hash, std::equal_to<>>;

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		SKSE::AllocTrampoline(14);

		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class F, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(T::size, T::thunk);
	}
}

#ifdef SKYRIM_AE
#	define OFFSET(se, ae) ae
#else
#	define OFFSET(se, ae) se
#endif

#include "Cache.h"
#include "Translation.h"
#include "Version.h"
