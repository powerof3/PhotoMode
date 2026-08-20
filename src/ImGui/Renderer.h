#pragma once

namespace ImGui::Renderer
{
	template <class F>
	static void RenderFrame(F&& a_func)
	{
		ImGui_ImplDX11_NewFrame();
		SKSE::ImGui_ImplWin32_NewFrame();
		{
			// trick imgui into rendering at game's real resolution (ie. if upscaled with Display Tweaks)
			static const auto screenSize = RE::BSGraphics::Renderer::GetScreenSize();

			auto& io = ImGui::GetIO();
			io.DisplaySize.x = static_cast<float>(screenSize.width);
			io.DisplaySize.y = static_cast<float>(screenSize.height);
		}
		ImGui::NewFrame();
		{
			GImGui->NavWindowingTarget = nullptr;  // disable windowing
			
			a_func();
		}
		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	inline std::atomic initialized{ false };

	namespace DisplayTweaks
	{
		inline float resolutionScale{ 1.0f };
		inline bool  borderlessUpscale{ false };
	}

	float GetResolutionScale();

	void LoadSettings(const CSimpleIniA& a_ini);
	void Install();
}
