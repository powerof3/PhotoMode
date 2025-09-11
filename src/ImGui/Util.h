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

	bool                   FramelessImageButton(const char* str_id, ImTextureID user_texture_id, const ImVec2& image_size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	bool                   AlignedImage(ID3D11ShaderResourceView* texID, const ImVec2& texture_size, const ImVec2& min, const ImVec2& max, const ImVec2& align, ImU32 colour);

	bool IsWidgetFocused();
	bool IsWidgetFocused(std::string_view label);
	bool IsWidgetFocused(ImGuiID id);

	ImVec2 GetNativeViewportSize();
	ImVec2 GetNativeViewportPos();
	ImVec2 GetNativeViewportCenter();
}
