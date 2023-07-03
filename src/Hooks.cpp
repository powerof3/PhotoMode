#include "Hooks.h"

#include "Input.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/LoadScreen.h"
#include "Screenshots/Manager.h"

namespace PhotoMode
{
	struct FromEulerAnglesZXY
	{
		static void thunk(RE::NiMatrix3* a_matrix, float a_z, float a_x, float a_y)
		{
			return func(a_matrix, a_z, a_x, MANAGER(PhotoMode)->GetViewRoll(a_y));
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// TESDataHandler idle array is not populated
	struct SetFormEditorID
	{
		static bool thunk(RE::TESIdleForm* a_this, const char* a_str)
		{
			if (!clib_util::string::is_empty(a_str)) {
				if (const std::string str(a_str); !str.starts_with("pa_")) {  // paired anims
					idles.AddForm(a_str, a_this);
				}
			}
			return func(a_this, a_str);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            size{ 0x33 };
	};

	void InstallHooks()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(49814, 50744), 0x1B };  // FreeCamera::GetRotation
		stl::write_thunk_call<FromEulerAnglesZXY>(target.address());

		stl::write_vfunc<RE::TESIdleForm, SetFormEditorID>();
	}
}

namespace Screenshot
{
	struct SaveD3D11TextureToFile
	{
		static void thunk(ID3D11Texture2D* a_texture_2d, char const* a_path, RE::BSGraphics::TextureFileFormat a_format)
		{
			bool skipVanillaScreenshot = false;

			if (MANAGER(Input)->IsScreenshotQueued()) {
				skipVanillaScreenshot = MANAGER(Screenshot)->TakeScreenshot(a_texture_2d, a_path);
			}

			if (!skipVanillaScreenshot) {
				func(a_texture_2d, a_path, a_format);
			}

			// enable UI if hidden
			MANAGER(Input)->OnScreenshotFinish();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHooks()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(75522, 77316), 0x60 };  // Main::Swap
		stl::write_thunk_call<SaveD3D11TextureToFile>(target.address());
	}
}

namespace LoadScreen
{
	struct GetLoadScreenModel
	{
		static RE::TESModelTextureSwap* thunk([[maybe_unused]] RE::TESLoadScreen* a_loadScreen)
		{
			if (const auto screenshotModel = MANAGER(LoadScreen)->LoadScreenshotModel()) {
				return screenshotModel;
			}
			return func(a_loadScreen);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct InitLoadScreen3D
	{
		static void thunk(RE::MistMenu* a_this, float a_scale, const RE::NiPoint3& a_rotateOffset, const RE::NiPoint3& a_translateOffset, const char* a_cameraShotPath)
		{
			if (auto transform = MANAGER(LoadScreen)->GetModelTransform()) {
				func(a_this, transform->scale, transform->rotationalOffset, transform->translateOffset, MANAGER(LoadScreen)->GetCameraShotPath(a_cameraShotPath));
				if (const auto canvas = a_this->loadScreenModel ? a_this->loadScreenModel->GetObjectByName("Canvas:0") : nullptr) {
					MANAGER(LoadScreen)->ApplyScreenshotTexture(canvas->AsGeometry());
				}
			} else {
				func(a_this, a_scale, a_rotateOffset, a_translateOffset, a_cameraShotPath);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHooks()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(51048, 51929), OFFSET(0x384, 0x27B) };
		stl::write_thunk_call<GetLoadScreenModel>(target.address());

		REL::Relocation<std::uintptr_t> target2{ RELOCATION_ID(51454, 52313), OFFSET(0x1D1, 0x1C3) };
		stl::write_thunk_call<InitLoadScreen3D>(target2.address());
	}
}

void Hooks::Install()
{
	PhotoMode::InstallHooks();
	Screenshot::InstallHooks();
	LoadScreen::InstallHooks();
}
