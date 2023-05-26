#include "Renderer.h"
#include "PhotoMode/Manager.h"

namespace PhotoMode::Renderer
{
	struct WndProc
	{
		static LRESULT thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			auto& io = ImGui::GetIO();
			if (uMsg == WM_KILLFOCUS) {
				io.ClearInputCharacters();
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
				const auto swapChain = renderer->data.renderWindows[0].swapChain;
				if (!swapChain) {
					logger::error("couldn't find swapChain");
					return;
				}

				DXGI_SWAP_CHAIN_DESC desc{};
				if (FAILED(swapChain->GetDesc(std::addressof(desc)))) {
					logger::error("IDXGISwapChain::GetDesc failed.");
					return;
				}

				const auto device = renderer->data.forwarder;
				const auto context = renderer->data.context;

				logger::info("Initializing ImGui..."sv);

				ImGui::CreateContext();

				auto& io = ImGui::GetIO();
				io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
				io.IniFilename = nullptr;

				auto& style = ImGui::GetStyle();
				style.FrameRounding = 4.0f;
				style.GrabRounding = 4.0f;
				style.WindowRounding = 4.0f;

				style.Colors[ImGuiCol_WindowBg] = { 0.0, 0.0, 0.0, 0.62f };
				style.Colors[ImGuiCol_Border] = { 0.6f, 0.6f, 0.6f, 1.0f };

				if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
					logger::error("ImGui initialization failed (Win32)");
					return;
				}
				if (!ImGui_ImplDX11_Init(device, context)) {
					logger::error("ImGui initialization failed (DX11)"sv);
					return;
				}

				io.FontDefault = io.Fonts->AddFontFromFileTTF(R"(Data\Fonts\Jost-Medium.ttf)", 22);
				selectedFont = io.Fonts->AddFontFromFileTTF(R"(Data\Fonts\Jost-Medium.ttf)", 26);

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

	// DXGIPresentHook
	struct StopTimer
	{
		static void thunk(std::uint32_t a_timer)
		{
			func(a_timer);

			// Skip if Imgui is not loaded
			if (!initialized.load()) {
				return;
			}

			const auto photoMode = get<Manager>();

			if (!photoMode->IsActive()) {
				return;
			}

			if (!photoMode->GetValid()) {
				photoMode->Deactivate();
				return;
			}

			photoMode->OnFrameUpdate();

			//Predraw UI

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			photoMode->Draw();

			ImGui::EndFrame();
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHook()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(75595, 77226), OFFSET(0x9, 0x275) };  // BSGraphics::InitD3D
		stl::write_thunk_call<CreateD3DAndSwapChain>(target.address());

		REL::Relocation<std::uintptr_t> target2{ RELOCATION_ID(75461, 77246), 0x9 };  // BSGraphics::Renderer::End
		stl::write_thunk_call<StopTimer>(target2.address());
	}
}
