#include "Widgets.h"

#include "IconsFonts.h"

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
				auto score = rapidfuzz::fuzz::partial_ratio(pattern_buffer, items[i].c_str());
				if (score >= 60.0) {
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

		ImGui::PushStyleColor(ImGuiCol_FrameBg, static_cast<ImVec4>(ImColor(0, 0, 0, 255)));
		ImGui::PushStyleColor(ImGuiCol_Text, static_cast<ImVec4>(ImColor(255, 255, 255, 204)));
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
			RE::PlaySound("UIMenuPrevNext");
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
					RE::PlaySound("UIMenuFocus");
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
				RE::PlaySound("UIMenuOK");
			} else if (IsKeyPressed(ImGuiKey_Escape) || IsKeyPressed(ImGuiKey_NavGamepadCancel)) {
				value_changed = false;
				CloseCurrentPopup();
			}
		}
		ImGui::PopStyleColor();
		ImGui::PopItemWidth();
		ImGui::EndCombo();

		if (value_changed) {
			MarkItemEdited(g.LastItemData.ID);
		}

		return value_changed;
	}

	bool ImGui::CenteredTextWithArrows(const char* label, const char* centerText)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext&     g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID     id = window->GetID(label);
		const float       w = CalcItemWidth();

		const ImVec2 label_size = CalcTextSize(label, nullptr, true);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb, false))
			return false;

		// RenderNavHighlight(frame_bb, id);

		const bool isHovered = GetFocusID() == id;
		if (!isHovered) {
			PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
		}

		PushFont(MANAGER(IconFont)->GetLargeFont());
		RenderTextClipped(frame_bb.Min, frame_bb.Max, centerText, nullptr, nullptr, ImVec2(0.5f, 0.5f));
		PopFont();

		if (!isHovered) {
			PopStyleColor();
		}

		// Draw arrows
		static auto leftArrow = MANAGER(IconFont)->GetStepperLeft();
		static auto rightArrow = MANAGER(IconFont)->GetStepperRight();

		const auto color = isHovered ? GetColorU32(ImGuiCol_Text) : GetColorU32(ImGuiCol_TextDisabled);

		AlignedImage(leftArrow->srView, leftArrow->size, frame_bb.Min, frame_bb.Max, ImVec2(0, 0.5f), color);
		AlignedImage(rightArrow->srView, rightArrow->size, frame_bb.Min, frame_bb.Max, ImVec2(1.0, 0.5f), color);

		return isHovered;
	}

	bool CheckBox(const char* label, bool* a_toggle)
	{
		bool selected = false;

		static auto checkbox = MANAGER(IconFont)->GetCheckbox();
		static auto checkboxFilled = MANAGER(IconFont)->GetCheckboxFilled();

		const auto newLabel = LeftAlignedText(label);

		AlignForWidth(checkbox->size.x);

		PushStyleColor(ImGuiCol_Button, ImVec4());
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());

		ImageButton(newLabel.c_str(), *a_toggle ? checkboxFilled->srView : checkbox->srView, checkbox->size, ImVec2(), ImVec2(1, 1), ImVec4(),
			GetFocusID() == GetCurrentWindow()->GetID(newLabel.c_str()) ? ImVec4(1, 1, 1, 1) : GetStyle().Colors[ImGuiCol_TextDisabled]);

		PopStyleColor(3);

		if (IsItemFocused()) {
			if (IsKeyPressed(ImGuiKey_Space) || IsKeyPressed(ImGuiKey_Enter) || IsKeyPressed(ImGuiKey_NavGamepadActivate)) {
				*a_toggle = !*a_toggle;
				selected = true;
			} else {
				UnfocusOnEscape();
			}
		}

		if (selected) {
			RE::PlaySound("UIMenuPrevNext");
		}
		return selected;
	}

	bool DragScalarEx(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
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
			// Tabbing or CTRL-clicking on Drag turns it into an InputText
			const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
			const bool clicked = hovered && IsMouseClicked(0, id);
			const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(ImGuiKey_MouseLeft, id));
			const bool make_active = (input_requested_by_tabbing || clicked || double_clicked || g.NavActivateId == id);
			if (make_active && (clicked || double_clicked))
				SetKeyOwner(ImGuiKey_MouseLeft, id);
			if (make_active && temp_input_allowed)
				if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || double_clicked || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
					temp_input_is_active = true;

			// (Optional) simple click (without moving) turns Drag into an InputText
			if (g.IO.ConfigDragClickToInputText && temp_input_allowed && !temp_input_is_active)
				if (g.ActiveId == id && hovered && g.IO.MouseReleased[0] && !IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * 0.50f)) {
					g.NavActivateId = id;
					g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
					temp_input_is_active = true;
				}

			if (make_active && !temp_input_is_active) {
				SetActiveID(id, window);
				SetFocusID(id, window);
				FocusWindow(window);
				g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
			}
		}

		if (temp_input_is_active) {
			// Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
			const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0 && (p_min == NULL || p_max == NULL || DataTypeCompare(data_type, p_min, p_max) < 0);
			return TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
		}

		// Draw frame
		const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered :
                                                                                                  ImGuiCol_FrameBg);
		//RenderNavHighlight(frame_bb, id);
		RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);
		window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, g.ActiveId == id ? IM_COL32(255, 255, 255, 204) : IM_COL32(255, 255, 255, 62), g.Style.FrameRounding, 0, 1.5f);

		// Drag behavior
		const bool value_changed = DragBehavior(id, data_type, p_data, v_speed, p_min, p_max, format, flags);
		if (value_changed)
			MarkItemEdited(id);

		// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
		const bool isHovered = GetFocusID() == id;
		if (!isHovered) {
			PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
		}

		char        value_buf[64];
		const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
		if (g.LogEnabled)
			LogSetNextTextDecoration("{", "}");
		RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

		if (!isHovered) {
			PopStyleColor();
		}

		/*if (label_size.x > 0.0f)
			RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);*/

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
		return value_changed;
	}

	bool DragFloatEx(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalarEx(label, ImGuiDataType_Float, v, v_speed, &v_min, &v_max, format, flags);
	}

	bool DragIntEx(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
	{
		return DragScalarEx(label, ImGuiDataType_S32, v, v_speed, &v_min, &v_max, format, flags);
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

		const ImVec2 label_size = CalcTextSize(label, nullptr, true);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

		const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
			return false;

		// Default format string when passing NULL
		if (format == nullptr)
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
			return TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : nullptr, is_clamp_input ? p_max : nullptr);
		}

		// Draw frame
		const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered :
                                                                                                  ImGuiCol_FrameBg);
		// RenderNavHighlight(frame_bb, id);
		// RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

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
		const bool isHovered = GetFocusID() == id;

		window->DrawList->AddRect(draw_bb.Min, draw_bb.Max, isHovered ? IM_COL32(255, 255, 255, 204) : IM_COL32(255, 255, 255, 62), g.Style.FrameRounding, 0, 1.75f);

		window->DrawList->AddRectFilled(draw_bb.Min, ImVec2(grab_bb.Min.x + (grab_bb.Max.x - grab_bb.Min.x) * 0.65f, draw_bb.Max.y), frame_col, style.FrameRounding, ImDrawCornerFlags_Left);
		window->DrawList->AddRectFilled(ImVec2(grab_bb.Max.x - (grab_bb.Max.x - grab_bb.Min.x) * 0.35f, draw_bb.Min.y), draw_bb.Max, frame_col, style.FrameRounding, ImDrawCornerFlags_Right);

		// Render grab
		if (grab_bb.Max.x > grab_bb.Min.x)
			window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

		// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
		if (!isHovered) {
			PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
		}

		char        value_buf[64];
		const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
		if (g.LogEnabled)
			LogSetNextTextDecoration("{", "}");
		RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, nullptr, ImVec2(0.5f, 0.5f));

		if (!isHovered) {
			PopStyleColor();
		}

		/*if (label_size.x > 0.0f)
			RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);*/

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

	// Copied from imgui_widgets.cpp
	static ImGuiPtrOrIndex GetTabBarRefFromTabBar(ImGuiTabBar* tab_bar)
	{
		ImGuiContext& g = *GImGui;
		if (g.TabBars.Contains(tab_bar))
			return ImGuiPtrOrIndex(g.TabBars.GetIndex(tab_bar));
		return ImGuiPtrOrIndex(tab_bar);
	}

	// Copied from imgui_widgets.cpp
	static int TabItemComparerByBeginOrder(const void* lhs, const void* rhs)
	{
		const ImGuiTabItem* a = (const ImGuiTabItem*)lhs;
		const ImGuiTabItem* b = (const ImGuiTabItem*)rhs;
		return (int)(a->BeginOrder - b->BeginOrder);
	}

	// Copy of BeginTabBar but with custom separator
	bool BeginTabBarCustomEx(ImGuiTabBar* tab_bar, const ImRect& tab_bar_bb, ImGuiTabBarFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow*  window = g.CurrentWindow;
		if (window->SkipItems)
			return false;

		// Add to stack
		g.CurrentTabBarStack.push_back(GetTabBarRefFromTabBar(tab_bar));
		g.CurrentTabBar = tab_bar;

		// Append with multiple BeginTabBar()/EndTabBar() pairs.
		tab_bar->BackupCursorPos = window->DC.CursorPos;
		if (tab_bar->CurrFrameVisible == g.FrameCount) {
			window->DC.CursorPos = ImVec2(tab_bar->BarRect.Min.x, tab_bar->BarRect.Max.y + tab_bar->ItemSpacingY);
			tab_bar->BeginCount++;
			return true;
		}

		// Ensure correct ordering when toggling ImGuiTabBarFlags_Reorderable flag, or when a new tab was added while being not reorderable
		if ((flags & ImGuiTabBarFlags_Reorderable) != (tab_bar->Flags & ImGuiTabBarFlags_Reorderable) || (tab_bar->TabsAddedNew && !(flags & ImGuiTabBarFlags_Reorderable)))
			ImQsort(tab_bar->Tabs.Data, tab_bar->Tabs.Size, sizeof(ImGuiTabItem), TabItemComparerByBeginOrder);
		tab_bar->TabsAddedNew = false;

		// Flags
		if ((flags & ImGuiTabBarFlags_FittingPolicyMask_) == 0)
			flags |= ImGuiTabBarFlags_FittingPolicyDefault_;

		tab_bar->Flags = flags;
		tab_bar->BarRect = tab_bar_bb;
		tab_bar->WantLayout = true;  // Layout will be done on the first call to ItemTab()
		tab_bar->PrevFrameVisible = tab_bar->CurrFrameVisible;
		tab_bar->CurrFrameVisible = g.FrameCount;
		tab_bar->PrevTabsContentsHeight = tab_bar->CurrTabsContentsHeight;
		tab_bar->CurrTabsContentsHeight = 0.0f;
		tab_bar->ItemSpacingY = g.Style.ItemSpacing.y;
		tab_bar->FramePadding = g.Style.FramePadding;
		tab_bar->TabsActiveCount = 0;
		tab_bar->LastTabItemIdx = -1;
		tab_bar->BeginCount = 1;

		// Set cursor pos in a way which only be used in the off-chance the user erroneously submits item before BeginTabItem(): items will overlap
		window->DC.CursorPos = ImVec2(tab_bar->BarRect.Min.x, tab_bar->BarRect.Max.y + tab_bar->ItemSpacingY);

		// Draw separator using ImGuiCol_Separator
		const ImU32 col = GetColorU32(ImGuiCol_Separator);
		const float y = tab_bar->BarRect.Max.y - 1.0f;
		{
			const float separator_min_x = tab_bar->BarRect.Min.x - IM_FLOOR(window->WindowPadding.x * 0.5f);
			const float separator_max_x = tab_bar->BarRect.Max.x + IM_FLOOR(window->WindowPadding.x * 0.5f);
			window->DrawList->AddLine(ImVec2(separator_min_x, y), ImVec2(separator_max_x, y), col, 2.0f);
		}
		return true;
	}

	// Copy of BeginTabBar but with custom separator
	bool BeginTabBarCustom(const char* str_id, ImGuiTabBarFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow*  window = g.CurrentWindow;
		if (window->SkipItems)
			return false;

		ImGuiID      id = window->GetID(str_id);
		ImGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(id);
		ImRect       tab_bar_bb = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->WorkRect.Max.x, window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
		tab_bar->ID = id;
		return BeginTabBarCustomEx(tab_bar, tab_bar_bb, flags | ImGuiTabBarFlags_IsFocused);
	}

	bool BeginTabItemEx(const char* label, ImGuiID* active_tab, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0)
	{
		ImGuiID id = ImGui::GetID(label);
		bool    wasActive = *active_tab == id;

		if (!wasActive)
			ImGui::PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);

		const bool isActive = ImGui::BeginTabItem(label, p_open, flags);

		if (!wasActive)
			ImGui::PopStyleColor();

		if (isActive)
			*active_tab = id;

		return isActive;
	}

	bool OpenTabOnHover(const char* a_label, const ImGuiTabItemFlags flags)
	{
		static ImGuiID activeInspectorTab;
		const bool     selected = BeginTabItemEx(a_label, &activeInspectorTab, nullptr, flags);
		if (!selected && ActivateOnHover()) {
			RE::PlaySound("UIJournalTabsSD");
		}
		return selected;
	}
}
