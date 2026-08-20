#include "Renderer.h"
#include "IconsFonts.h"
#include "Styles.h"

#include "Gallery/Manager.h"
#include "PhotoMode/Manager.h"

namespace ImGui::Renderer
{
	float GetResolutionScale()
	{
		static auto height = RE::BSGraphics::Renderer::GetScreenSize().height;
		return DisplayTweaks::borderlessUpscale ? DisplayTweaks::resolutionScale : height / 1080.0f;
	}

	void LoadSettings(const CSimpleIniA& a_ini)
	{
		DisplayTweaks::resolutionScale = static_cast<float>(a_ini.GetDoubleValue("Render", "ResolutionScale", static_cast<double>(DisplayTweaks::resolutionScale)));
		DisplayTweaks::borderlessUpscale = a_ini.GetBoolValue("Render", "BorderlessUpscale", DisplayTweaks::borderlessUpscale);
	}

	struct WndProc
	{
		static LRESULT thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			auto& io = ImGui::GetIO();
			if (uMsg == WM_KILLFOCUS) {
				io.ClearInputKeys();
			}

			return func(hWnd, uMsg, wParam, lParam);
		}
		static inline WNDPROC func;
	};

	struct CreateD3DAndSwapChain
	{
		static void thunk()
		{
			func();

			if (const auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
				const auto swapChain = reinterpret_cast<IDXGISwapChain*>(renderer->data.renderWindows[0].swapChain);
				if (!swapChain) {
					logger::error("couldn't find swapChain");
					return;
				}

				DXGI_SWAP_CHAIN_DESC desc{};
				if (FAILED(swapChain->GetDesc(std::addressof(desc)))) {
					logger::error("IDXGISwapChain::GetDesc failed.");
					return;
				}

				const auto device = reinterpret_cast<ID3D11Device*>(renderer->data.forwarder);
				const auto context = reinterpret_cast<ID3D11DeviceContext*>(renderer->data.context);

				logger::info("Initializing ImGui..."sv);

				ImGui::CreateContext();

				auto& io = ImGui::GetIO();
				io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
				io.IniFilename = nullptr;

				if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
					logger::error("ImGui initialization failed (Win32)");
					return;
				}
				if (!ImGui_ImplDX11_Init(device, context)) {
					logger::error("ImGui initialization failed (DX11)"sv);
					return;
				}

				ImGui::Styles::GetSingleton()->OnStyleRefresh();

				logger::info("ImGui initialized.");

				initialized.store(true);

				WndProc::func = reinterpret_cast<WNDPROC>(
					SetWindowLongPtrA(
						desc.OutputWindow,
						GWLP_WNDPROC,
						reinterpret_cast<LONG_PTR>(WndProc::thunk)));
				if (!WndProc::func) {
					logger::error("SetWindowLongPtrA failed!");
				}
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// IMenu::PostDisplay
	struct PostDisplay
	{
		static void thunk(RE::IMenu* a_menu)
		{
			// Skip if Imgui is not loaded
			if (!initialized.load()) {
				return func(a_menu);
			}

			const auto photoMode = MANAGER(PhotoMode);
			const auto gallery = MANAGER(Gallery);

			const bool photoModeActive = photoMode->IsActive() && photoMode->OnFrameUpdate();
			const bool galleryActive = gallery->IsActive() && gallery->OnFrameUpdate();

			if (!photoModeActive && !galleryActive) {
				return func(a_menu);
			}

			RenderFrame([&] {
				if (photoModeActive) {
					photoMode->Draw();
				} else {
					gallery->Draw();
				}
			});

			return func(a_menu);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx{ 0x6 };
	};

	struct StopTimer
	{
		static void thunk(std::uint32_t timer)
		{
			func(timer);

			// Skip if Imgui is not loaded
			if (!initialized.load()) {
				return;
			}

			const auto photoMode = MANAGER(PhotoMode);
			if (!(photoMode->IsActive() && photoMode->IsHidden() && photoMode->HasOverlay())) {
				return;
			}

			RenderFrame([&] {
				photoMode->DrawOverlays();
			});
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(75595, 77226), OFFSET(0x9, 0x275) };  // BSGraphics::InitD3D
		stl::write_thunk_call<CreateD3DAndSwapChain>(target.address());

		REL::Relocation<std::uintptr_t> target2{ RELOCATION_ID(75461, 77246), 0x9 };  // BSGraphics::Renderer::End
		stl::write_thunk_call<StopTimer>(target2.address());

		stl::write_vfunc<RE::HUDMenu, PostDisplay>();
	}
}
