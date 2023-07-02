#include "Styles.h"

#include "Renderer.h"

void ImGui::DrawStyleEditor()
{
	ImGui::Begin("Style Editor", nullptr, 0);
	{
		ImGui::ShowStyleEditor(&vanillaStyle);
	}
	ImGui::End();
}

void ImGui::StyleVanilla()
{
	constexpr auto bg_alpha = 0.68f;
	constexpr auto disabled_alpha = 0.30f;

	constexpr auto background = ImVec4(0.0f, 0.0f, 0.0f, bg_alpha);
	constexpr auto border = ImVec4(0.396f, 0.404f, 0.412f, bg_alpha);

	auto& style = GetStyle();
	auto& colors = style.Colors;

	style.WindowBorderSize = 3.0f;
	style.TabRounding = 0.0f;

	colors[ImGuiCol_WindowBg] = background;
	colors[ImGuiCol_ChildBg] = background;

	colors[ImGuiCol_Border] = border;
	colors[ImGuiCol_Separator] = border;

	colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, disabled_alpha);

	colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, bg_alpha);
	colors[ImGuiCol_FrameBgHovered] = colors[ImGuiCol_FrameBg];
	colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_FrameBg];

	colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.245f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.531f);

	colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, bg_alpha);
	colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_Button];
	colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_Button];

	//colors[ImGuiCol_Header] = colors[ImGuiCol_TextDisabled];
	colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
	colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);

	colors[ImGuiCol_Tab] = ImVec4(0.2f, 0.2f, 0.2f, disabled_alpha);
	colors[ImGuiCol_TabHovered] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_TabActive] = colors[ImGuiCol_TabHovered];
	colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
	colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

	colors[ImGuiCol_NavHighlight] = ImVec4();

	style.ScaleAllSizes(Renderer::GetResolutionScale());

	vanillaStyle = style;
}
