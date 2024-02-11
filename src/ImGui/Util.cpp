#include "Util.h"

#include "Renderer.h"

namespace ImGui
{
	int IndexOfKey(const std::vector<std::pair<int, double>>& pair_list, const int key)
	{
		for (size_t i = 0; i < pair_list.size(); ++i) {
			auto& p = pair_list[i];
			if (p.first == key) {
				return static_cast<int>(i);
			}
		}
		return -1;
	}

	// Copied from imgui_widgets.cpp
	float CalcMaxPopupHeightFromItemCount(const int items_count)
	{
		ImGuiContext& g = *GImGui;
		if (items_count <= 0)
			return FLT_MAX;
		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
	}

	// https://github.com/ocornut/imgui/discussions/3862
	void AlignForWidth(float width, float alignment)
	{
		float avail = GetContentRegionAvail().x;
		float off = (avail - width) * alignment;

		if (off > 0.0f) {
			SetCursorPosX(GetCursorPosX() + off);
		}
	}

	void AlignedImage(ImTextureID texID, const ImVec2& texture_size, const ImVec2& min, const ImVec2& max, const ImVec2& align, ImU32 colour)
	{
		const ImGuiWindow* window = GetCurrentWindow();
		ImVec2             pos = min;

		if (align.x > 0.0f)
			pos.x = ImMax(pos.x, pos.x + (max.x - pos.x - texture_size.x) * align.x);
		if (align.y > 0.0f)
			pos.y = ImMax(pos.y, pos.y + (max.y - pos.y - texture_size.y) * align.y);

		window->DrawList->AddImage(texID, pos, pos + texture_size, ImVec2(0, 0), ImVec2(1, 1), colour);
	}

	void ExtendWindowPastBorder()
	{
		const ImGuiWindow* window = GetCurrentWindowRead();
		const ImGuiWindow* rootWindow = FindWindowByName("##Main");

		const auto borderSize = window->WindowBorderSize;
		const auto newWindowPos = ImVec2{ window->Pos.x - borderSize, window->Pos.y - borderSize };

		rootWindow->DrawList->AddRect(newWindowPos, newWindowPos + ImVec2(window->Size.x + 2 * borderSize, window->Size.y + 2 * borderSize), GetColorU32(ImGuiCol_WindowBg), 0.0f, 0, borderSize);
	}

	std::string LeftAlignedText(const char* label)
	{
		const float width = CalcItemWidth();
		const float x = GetCursorPosX();

		const auto newLabel = "##"s + label;

		const auto hovered = GetFocusID() == GetCurrentWindow()->GetID(newLabel.c_str());
		if (!hovered) {
			PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
		}
		Text(label);
		if (!hovered) {
			PopStyleColor();
		}

		SameLine();
		SetCursorPosX(x + width * 0.5f + GetStyle().ItemInnerSpacing.x);
		SetNextItemWidth(-1);

		return newLabel;
	}

	void CenteredText(const char* label, bool vertical)
	{
		const auto windowSize = ImGui::GetWindowSize();
		const auto textSize = ImGui::CalcTextSize(label);

		if (vertical) {
			ImGui::SetCursorPosY((windowSize.y - textSize.y) * 0.5f);
		} else {
			ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
		}

		ImGui::Text(label);
	}

	bool ActivateOnHover()
	{
		if (!IsItemActive()) {
			if (IsItemFocused()) {
				ActivateItemByID(GetItemID());
				return true;
			}
		} else {
			UnfocusOnEscape();
		}

		return false;
	}

	void UnfocusOnEscape()
	{
		ImGuiContext& g = *GImGui;
		if (IsKeyDown(ImGuiKey_Escape) || IsKeyDown(ImGuiKey_NavGamepadCancel)) {
			g.NavId = 0;
			g.NavDisableHighlight = true;
			SetWindowFocus(nullptr);
		}
	}

	ImVec2 GetNativeViewportPos()
	{
		return GetMainViewport()->Pos; // always 0, 0
	}

	ImVec2 GetNativeViewportSize()
	{
		return GetMainViewport()->Size;
	}

	ImVec2 GetNativeViewportCenter()
	{
		const auto Size = GetNativeViewportSize();
		return { Size.x * 0.5f, Size.y * 0.5f };
	}
}
