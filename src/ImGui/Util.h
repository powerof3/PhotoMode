#pragma once

namespace ImGui
{
	int   IndexOfKey(const std::vector<std::pair<int, double>>& pair_list, int key);
	float CalcMaxPopupHeightFromItemCount(int items_count);

	void AlignForWidth(float width, float alignment = 0.5f);
	void AlignedImage(ImTextureID texID, const ImVec2& texture_size, const ImVec2& min, const ImVec2& max, const ImVec2& align, ImU32 colour);

	void ExtendWindowPastBorder();

	std::string LeftAlignedText(const char* label);

	void CenteredText(const char* label, bool vertical = false);

	bool ActivateOnHover();
}
