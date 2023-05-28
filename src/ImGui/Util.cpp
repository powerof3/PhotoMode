#include "Util.h"

#include "Renderer.h"

namespace ImGui
{
	// Source: https://gist.github.com/idbrii/5ddb2135ca122a0ec240ce046d9e6030
	//
	// Author: David Briscoe
	//
	// Modified ComboWithFilter with rapidFuzz
	// Using dear imgui, v1.89 WIP
	//
	// Adds arrow/pgup/pgdn navigation, Enter to confirm, max_height_in_items, and
	// fixed focus on open and avoids drawing past window edges.
	//
	// Licensed as CC0/public domain.
	//
	// Posted in issue: https://github.com/ocornut/imgui/issues/1658#issuecomment-1086193100

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
		if (items_count <= 0) {
			return FLT_MAX;
		}
		return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y +
		       (g.Style.WindowPadding.y * 2);
	}

	bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items)
	{
		ImGuiContext& g = *GImGui;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) {
			return false;
		}

		const int items_count = static_cast<int>(items.size());

		// Use imgui Items_ getters to support more input formats.
		const char* preview_value = nullptr;
		if (*current_item >= 0 && *current_item < items_count) {
			preview_value = items[*current_item].c_str();
		}

		static int  focus_idx = -1;
		static char pattern_buffer[MAX_PATH] = { 0 };

		bool value_changed = false;

		const ImGuiID id = window->GetID(label);
		const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);  // copied from BeginCombo
		const bool    is_already_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
		const bool    is_filtering = is_already_open && pattern_buffer[0] != '\0';

		int show_count = items_count;

		std::vector<std::pair<int, double>> itemScoreVector;
		if (is_filtering) {
			// Filter before opening to ensure we show the correct size window.
			// We won't get in here unless the popup is open.
			for (int i = 0; i < items_count; i++) {
				auto score = rapidfuzz::fuzz::partial_token_set_ratio(pattern_buffer, items[i].c_str());
				if (score >= 50.0) {
					itemScoreVector.push_back(std::make_pair(i, score));
				}
			}
			std::ranges::sort(itemScoreVector, [](const auto& a, const auto& b) {
				return (b.second < a.second);
			});
			const int current_score_idx = IndexOfKey(itemScoreVector, focus_idx);
			if (current_score_idx < 0 && !itemScoreVector.empty()) {
				focus_idx = itemScoreVector[0].first;
			}
			show_count = static_cast<int>(itemScoreVector.size());
		}

		// Define the height to ensure our size calculation is valid.
		if (popup_max_height_in_items == -1) {
			popup_max_height_in_items = 5;
		}
		popup_max_height_in_items = ImMin(popup_max_height_in_items, show_count);

		if (!(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint)) {
			const int numItems = popup_max_height_in_items + 2;  // extra for search bar
			SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX,
														   CalcMaxPopupHeightFromItemCount(numItems)));
		}

		if (!BeginCombo(label, preview_value, ImGuiComboFlags_None)) {
			return false;
		}

		if (!is_already_open) {
			focus_idx = *current_item;
			memset(pattern_buffer, 0, IM_ARRAYSIZE(pattern_buffer));
		}

		ImGui::PushStyleColor(ImGuiCol_FrameBg, static_cast<ImVec4>(ImColor(240, 240, 240, 255)));
		ImGui::PushStyleColor(ImGuiCol_Text, static_cast<ImVec4>(ImColor(0, 0, 0, 255)));
		ImGui::PushStyleColor(ImGuiCol_NavHighlight, static_cast<ImVec4>(ImColor(0, 0, 0, 0)));
		ImGui::PushItemWidth(-FLT_MIN);
		// Filter input
		if (!is_already_open) {
			ImGui::SetKeyboardFocusHere();
		}
		InputText("##ComboWithFilter_inputText", pattern_buffer, MAX_PATH, ImGuiInputTextFlags_AutoSelectAll);

		ImGui::PopStyleColor(3);

		int move_delta = 0;
		if (IsKeyPressed(ImGuiKey_UpArrow) || IsKeyPressed(ImGuiKey_GamepadDpadUp)) {
			--move_delta;
		} else if (IsKeyPressed(ImGuiKey_DownArrow) || IsKeyPressed(ImGuiKey_GamepadDpadDown)) {
			++move_delta;
		} else if (IsKeyPressed(ImGuiKey_PageUp)) {
			move_delta -= popup_max_height_in_items;
		} else if (IsKeyPressed(ImGuiKey_PageDown)) {
			move_delta += popup_max_height_in_items;
		}

		if (move_delta != 0) {
			if (is_filtering) {
				int current_score_idx = IndexOfKey(itemScoreVector, focus_idx);
				if (current_score_idx >= 0) {
					const int count = static_cast<int>(itemScoreVector.size());
					current_score_idx = ImClamp(current_score_idx + move_delta, 0, count - 1);
					focus_idx = itemScoreVector[current_score_idx].first;
				}
			} else {
				focus_idx = ImClamp(focus_idx + move_delta, 0, items_count - 1);
			}
		}

		// Copied from ListBoxHeader
		// If popup_max_height_in_items == -1, default height is maximum 7.
		const float height_in_items_f = (popup_max_height_in_items < 0 ? ImMin(items_count, 7) :
																		 popup_max_height_in_items) +
		                                0.25f;
		ImVec2 size;
		size.x = 0.0f;
		size.y = GetTextLineHeightWithSpacing() * height_in_items_f + g.Style.FramePadding.y * 2.0f;

		ImGui::PushStyleColor(ImGuiCol_NavHighlight, static_cast<ImVec4>(ImColor(0, 0, 0, 0)));
		if (ImGui::BeginListBox("##ComboWithFilter_itemList", size)) {
			for (int i = 0; i < show_count; i++) {
				int idx = is_filtering ? itemScoreVector[i].first : i;
				PushID(reinterpret_cast<void*>(static_cast<intptr_t>(idx)));
				const bool  item_selected = (idx == focus_idx);
				const char* item_text = items[idx].c_str();
				if (Selectable(item_text, item_selected)) {
					value_changed = true;
					*current_item = idx;
					CloseCurrentPopup();
				}

				if (item_selected) {
					SetItemDefaultFocus();
					// SetItemDefaultFocus doesn't work so also check IsWindowAppearing.
					if (move_delta != 0 || IsWindowAppearing()) {
						SetScrollHereY();
					}
				}
				PopID();
			}
			ImGui::EndListBox();

			if (IsKeyPressed(ImGuiKey_Enter) || IsKeyPressed(ImGuiKey_Space) || IsKeyPressed(ImGuiKey_NavGamepadActivate)) {
				value_changed = true;
				*current_item = focus_idx;
				CloseCurrentPopup();
			} else if (IsKeyPressed(ImGuiKey_Escape) || IsKeyPressed(ImGuiKey_NavGamepadCancel)) {
				value_changed = false;
				CloseCurrentPopup();
			}
		}
		ImGui::PopItemWidth();
		ImGui::PopStyleColor();
		ImGui::EndCombo();

		if (value_changed) {
			MarkItemEdited(g.LastItemData.ID);
		}

		return value_changed;
	}

	// https://github.com/ocornut/imgui/issues/1831#issuecomment-391324007
	void PushMultiItemsWidthsAndLabels(const char* const labels[], const int components, float w_full)
	{
		ImGuiWindow*      window = GetCurrentWindow();
		const ImGuiStyle& style = GImGui->Style;
		if (w_full <= 0.0f)
			w_full = GetContentRegionAvail().x;

		const float w_item_one = ImMax(1.0f, (w_full - (style.ItemInnerSpacing.x * 2.0f) * (components - 1)) / static_cast<float>(components)) - style.ItemInnerSpacing.x;
		for (int i = 0; i < components; i++)
			window->DC.ItemWidthStack.push_back(w_item_one - CalcTextSize(labels[i]).x);
		window->DC.ItemWidth = window->DC.ItemWidthStack.back();
	}

	bool DragIntNEx(const char* const labels[], int* v, const int components, const float v_speed, const int v_min[], const int v_max[], const char* display_format[], const ImGuiSliderFlags flags)
	{
		const ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;

		bool value_changed = false;
		BeginGroup();

		PushMultiItemsWidthsAndLabels(labels, components, 0.0f);
		for (int i = 0; i < components; i++) {
			PushID(labels[i]);
			PushID(i);
			TextUnformatted(labels[i], FindRenderedTextEnd(labels[i]));
			SameLine();
			value_changed |= DragInt("", &v[i], v_speed, v_min[i], v_max[i], display_format[i], flags);
			SameLine(0, g.Style.ItemInnerSpacing.x);
			PopID();
			PopID();
			PopItemWidth();
		}

		EndGroup();

		return value_changed;
	}

	bool DragInt2Ex(const char* const labels[], int* v, float v_speed, const int v_min[], const int v_max[], const char* display_format[], ImGuiSliderFlags flags)
	{
		return DragIntNEx(labels, v, 2, v_speed, v_min, v_max, display_format, flags);
	}

	// https://github.com/libigl/libigl/issues/1300#issuecomment-1310174619
	std::string LabelPrefix(const char* const label)
	{
		if (strlen(label) == 0) {
			return label;
		}

		const float width = CalcItemWidth();
		const float x = GetCursorPosX();

		Text(label);
		SameLine();
		SetCursorPosX(x + width * 0.5f + GetStyle().ItemInnerSpacing.x);
		SetNextItemWidth(-1);

		return "##"s + label;
	}

	std::string LabelPrefix(const std::string& label)
	{
		return LabelPrefix(label.c_str());
	}

	void CenterLabel(const char* label)
	{
		const auto windowWidth = ImGui::GetWindowSize().x;
		const auto textWidth = ImGui::CalcTextSize(label).x;

		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text(label);
	}

	void ImGui::CenteredTextWithArrows(const char* label, const char* centerText)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext&     g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID     id = window->GetID(label);
		const float       w = CalcItemWidth();

		const ImVec2 label_size = CalcTextSize(label, nullptr, true);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb, false))
			return;

		RenderNavHighlight(frame_bb, id);

		const bool isHovered = IsItemHovered();
		if (!isHovered) {
			PushStyleColor(ImGuiCol_Text, ImVec4{ 0.60f, 0.60f, 0.60f, 1.0f });
		}

		if (isHovered) {
			PushFont(PhotoMode::Renderer::selectedFont);
			RenderTextClipped(frame_bb.Min, frame_bb.Max, centerText, nullptr, nullptr, ImVec2(0.5f, 0.5f));
		} else {
			RenderTextClipped(frame_bb.Min, frame_bb.Max, centerText, nullptr, nullptr, ImVec2(0.5f, 0.5f));
			PushFont(PhotoMode::Renderer::selectedFont);
		}

		RenderTextClipped(frame_bb.Min, frame_bb.Max, "<", nullptr, nullptr, ImVec2(0.01f, 0.5f));
		RenderTextClipped(frame_bb.Min, frame_bb.Max, ">", nullptr, nullptr, ImVec2(0.99f, 0.5f));

		PopFont();
		if (!isHovered) {
			PopStyleColor();
		}

		if (label_size.x > 0.0f) {
			RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);
		}
	}

	bool OnOffToggle(const char* label, bool* a_toggle, const char* on, const char* off)
	{
		CenteredTextWithArrows(LabelPrefix(label).c_str(), *a_toggle ? on : off);
		if (IsItemHovered()) {
			SetItemDefaultFocus();
			if (IsKeyPressed(ImGuiKey_Space) || IsKeyPressed(ImGuiKey_Enter)) {
				*a_toggle = !*a_toggle;
				return true;
			} else if (IsKeyPressed(ImGuiKey_LeftArrow)) {
				*a_toggle = false;
				return true;
			} else if (IsKeyPressed(ImGuiKey_RightArrow)) {
				*a_toggle = true;
				return true;
			}
		}
		return false;
	}

	bool ThinSliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, float sliderThickness)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext&     g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID     id = window->GetID(label);
		const float       w = CalcItemWidth();

		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

		const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
			return false;

		// Default format string when passing NULL
		if (format == NULL)
			format = DataTypeGetInfo(data_type)->PrintFmt;

		const bool hovered = ItemHoverable(frame_bb, id);
		bool       temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
		if (!temp_input_is_active) {
			// Tabbing or CTRL-clicking on Slider turns it into an input box
			const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
			const bool clicked = hovered && IsMouseClicked(0, id);
			const bool make_active = (input_requested_by_tabbing || clicked || g.NavActivateId == id);
			if (make_active && clicked)
				SetKeyOwner(ImGuiKey_MouseLeft, id);
			if (make_active && temp_input_allowed)
				if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
					temp_input_is_active = true;

			if (make_active && !temp_input_is_active) {
				SetActiveID(id, window);
				SetFocusID(id, window);
				FocusWindow(window);
				g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
			}
		}

		if (temp_input_is_active) {
			// Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
			const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
			return TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
		}

		// Draw frame
		const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered :
																								  ImGuiCol_FrameBg);
		const ImU32 frame_col_after = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered :
																												  ImGuiCol_FrameBg);
		RenderNavHighlight(frame_bb, id);

		// Slider behavior
		ImRect     grab_bb;
		const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
		if (value_changed)
			MarkItemEdited(id);

		ImRect draw_bb = frame_bb;
		if (sliderThickness != 1.0f) {
			const auto shrink_amount = static_cast<float>(static_cast<int>((frame_bb.Max.y - frame_bb.Min.y) * 0.5f * (1.0f - sliderThickness)));
			draw_bb.Min.y += shrink_amount;
			draw_bb.Max.y -= shrink_amount;
		}

		// Render track
		window->DrawList->AddRectFilled(draw_bb.Min, ImVec2(grab_bb.Min.x + (grab_bb.Max.x - grab_bb.Min.x) * 0.65f, draw_bb.Max.y), frame_col, style.FrameRounding, ImDrawCornerFlags_Left);
		window->DrawList->AddRectFilled(ImVec2(grab_bb.Max.x - (grab_bb.Max.x - grab_bb.Min.x) * 0.35f, draw_bb.Min.y), draw_bb.Max, frame_col_after, style.FrameRounding, ImDrawCornerFlags_Right);

		// Render grab
		if (grab_bb.Max.x > grab_bb.Min.x)
			window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

		// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
		char        value_buf[64];
		const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
		if (g.LogEnabled)
			LogSetNextTextDecoration("{", "}");
		RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

		if (label_size.x > 0.0f)
			RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
		return value_changed;
	}

	bool ThinSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return ThinSliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags, 0.5f);
	}

	bool ThinSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
	{
		return ThinSliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags, 0.5f);
	}

	void ActivateOnHover()
	{
		if (IsItemFocused()) {
			ActivateItem(GetItemID());
		}
	}

	bool OpenTabOnHover(const char* a_label, const ImGuiTabItemFlags flags)
	{
		const bool selected = BeginTabItem(a_label, nullptr, flags);
		if (!selected) {
			ActivateOnHover();
		}
		return selected;
	}
}
