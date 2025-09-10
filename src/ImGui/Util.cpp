#include "Util.h"

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

	void ExtendWindowPastBorder()
	{
		const ImGuiWindow* window = GetCurrentWindowRead();
		const ImGuiWindow* rootWindow = FindWindowByName("##Main");

		const auto borderSize = window->WindowBorderSize;
		const auto newWindowPos = ImVec2{ window->Pos.x - borderSize, window->Pos.y - borderSize };

		rootWindow->DrawList->AddRect(newWindowPos, newWindowPos + ImVec2(window->Size.x + 2 * borderSize, window->Size.y + 2 * borderSize), GetColorU32(ImGuiCol_WindowBg), 0.0f, 0, borderSize);
	}

	void LeftAlignedTextImpl(const char* label, const std::string& newLabel)
	{
		const float width = CalcItemWidth();
		const float x = GetCursorPosX();

		const bool hovered = IsWidgetFocused(newLabel.empty() ? label : newLabel.c_str());

		if (!hovered) {
			PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
		}
		TextUnformatted(label);
		if (!hovered) {
			PopStyleColor();
		}

		SameLine();
		SetCursorPosX(x + width * 0.5f + GetStyle().ItemInnerSpacing.x);
		SetNextItemWidth(-1);
	}

	std::string LeftAlignedText(const char* label)
	{
		const auto newLabel = "##"s + label;
		LeftAlignedTextImpl(label, newLabel);
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

		ImGui::TextUnformatted(label);
	}

	bool IsWidgetFocused()
	{
		return IsWidgetFocused(GetItemID());
	}

	bool IsWidgetFocused(std::string_view label)
	{
		const auto id = GetCurrentWindow()->GetID(label.data());
		return IsWidgetFocused(id);
	}

	bool IsWidgetFocused(ImGuiID id)
	{
		return GetHoveredID() == id && (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) == 0;
	}

	ImVec2 GetNativeViewportPos()
	{
		return GetMainViewport()->Pos;  // always 0, 0
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
