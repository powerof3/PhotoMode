#pragma once

namespace ImGui
{
	int   IndexOfKey(const std::vector<std::pair<int, double>>& pair_list, int key);
	float CalcMaxPopupHeightFromItemCount(int items_count);

	void AlignForWidth(float width, float alignment = 0.5f);

	void ExtendWindowPastBorder();

	void        LeftAlignedTextImpl(const char* label, const std::string& newLabel = "");
	std::string LeftAlignedText(const char* label);
	void        CenteredText(const char* label, bool vertical = false);

	bool IsWidgetFocused();
	bool IsWidgetFocused(std::string_view label);
	bool IsWidgetFocused(ImGuiID id);

	ImVec2 GetNativeViewportSize();
	ImVec2 GetNativeViewportPos();
	ImVec2 GetNativeViewportCenter();
}
