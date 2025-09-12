#include "Util.h"

#include "Input.h"

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

	bool FramelessImageButton(const char* str_id, ImTextureID user_texture_id, const ImVec2& image_size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		PushStyleColor(ImGuiCol_Button, ImVec4());
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());

		auto result = ImageButton(str_id, user_texture_id, image_size, uv0, uv1, bg_col, tint_col);

		PopStyleColor(3);

		return result;
	}

	bool AlignedImage(ID3D11ShaderResourceView* texID, const ImVec2& texture_size, const ImVec2& min, const ImVec2& max, const ImVec2& align, ImU32 colour)
	{
		ImVec2 pos = min;

		if (align.x > 0.0f)
			pos.x = ImMax(pos.x, pos.x + (max.x - pos.x - texture_size.x) * align.x);
		if (align.y > 0.0f)
			pos.y = ImMax(pos.y, pos.y + (max.y - pos.y - texture_size.y) * align.y);

		GetCurrentWindow()->DrawList->AddImage((ImU64)texID, pos, pos + texture_size, ImVec2(0, 0), ImVec2(1, 1), colour);

		return MANAGER(Input)->IsInputKBM() ? IsMouseHoveringRect(pos, pos + texture_size) && IsMouseClicked(0) && (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) == 0 : false;
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
		bool isKBM = MANAGER(Input)->IsInputKBM();
		return (isKBM ? GetHoveredID() == id : GetFocusID() == id) &&
		       (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) == 0;
	}

	bool ActivateOnHover()
	{
		if (MANAGER(Input)->IsInputGamepad()) {
			if (!IsItemActive()) {
				if (IsItemFocused()) {
					ActivateItemByID(GetItemID());
					return true;
				}
			} else {
				UnfocusOnEscape();
			}
		}

		return false;
	}

	void UnfocusOnEscape()
	{
		if (MANAGER(Input)->IsInputGamepad()) {
			ImGuiContext& g = *GImGui;
			if (IsKeyDown(ImGuiKey_NavGamepadCancel)) {
				g.NavId = 0;
				g.NavCursorVisible = false;
				SetWindowFocus(nullptr);
			}
		}
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
