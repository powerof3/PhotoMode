#pragma once

#include "Styles.h"
#include "Util.h"

namespace ImGui
{
	bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items = -1);

	bool CheckBox(const char* label, bool* a_toggle);
	
	std::tuple<bool, bool, bool> CenteredTextWithArrows(const char* label, std::string_view centerText);  // [hovered, clickedLeftArrow, clickedRightArrow]

	template <class E>
	bool EnumSlider(const char* label, E* index, const std::ranges::common_range auto& a_enum, bool a_translate = true)
	{
		bool value_changed = false;

		std::size_t uIndex;
		if constexpr (std::is_enum_v<E>) {
			uIndex = std::to_underlying(*index);
		} else {
			uIndex = *index;
		}

		LeftAlignedTextImpl(label);
		auto [hovered, clickedLeft, clickedRight] = CenteredTextWithArrows(label, a_translate ? TRANSLATE(a_enum[uIndex]) : a_enum[uIndex]);

		if (hovered || IsWidgetFocused(label)) {
			const bool pressedLeft = clickedLeft || IsKeyPressed(ImGuiKey_LeftArrow) || IsKeyPressed(ImGuiKey_GamepadDpadLeft);
			const bool pressedRight = clickedRight || IsKeyPressed(ImGuiKey_RightArrow) || IsKeyPressed(ImGuiKey_GamepadDpadRight);
			if (pressedLeft) {
				uIndex = (uIndex - 1 + a_enum.size()) % a_enum.size();
			}
			if (pressedRight) {
				uIndex = (uIndex + 1) % a_enum.size();
			}
			if (pressedLeft || pressedRight) {
				value_changed = true;
				*index = static_cast<E>(uIndex);
				RE::PlaySound("UIMenuPrevNext");
			}
		}

		return value_changed;
	}

	bool BeginTabItemEx(const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0);

	bool OutlineButton(const char* label, bool* wasFocused = nullptr);

	bool DragScalarEx(const char* label, ImGuiDataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags);
	template <class T>
	bool DragOnHover(const char* label, T* v, float v_speed = 1.0f, T v_min = 0, T v_max = 100, const char* format = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp)
	{
		const auto newLabel = LeftAlignedText(label);

		PushStyleColor(ImGuiCol_FrameBg, GetUserStyleColorU32(USER_STYLE::kFrameBG_Widget));
		PushStyleColor(ImGuiCol_FrameBgHovered, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));
		PushStyleColor(ImGuiCol_FrameBgActive, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));

		bool result;
		if constexpr (std::is_floating_point_v<T>) {
			result = DragScalarEx(newLabel.c_str(), ImGuiDataType_Float, v, v_speed, &v_min, &v_max, format ? format : "%.2f", flags);
		} else {
			result = DragScalarEx(newLabel.c_str(), ImGuiDataType_S32, v, v_speed, &v_min, &v_max, format ? format : "%d", flags);
		}
		if (result) {
			RE::PlaySound("UIMenuPrevNext");
		}
		ActivateOnHover();

		PopStyleColor(3);

		return result;
	}

	bool ThinSliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, float sliderThickness);
	template <class T>
	bool Slider(const char* label, T* v, T v_min, T v_max, const char* format = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp)
	{
		const auto newLabel = LeftAlignedText(label);

		PushStyleColor(ImGuiCol_FrameBg, GetUserStyleColorU32(USER_STYLE::kFrameBG_Widget));
		PushStyleColor(ImGuiCol_FrameBgHovered, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));
		PushStyleColor(ImGuiCol_FrameBgActive, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));

		bool result;
		if constexpr (std::is_floating_point_v<T>) {
			result = ThinSliderScalar(newLabel.c_str(), ImGuiDataType_Float, v, &v_min, &v_max, format ? format : "%.2f", flags, 0.5f);
		} else {
			result = ThinSliderScalar(newLabel.c_str(), ImGuiDataType_S32, v, &v_min, &v_max, format ? format : "%d", flags, 0.5f);
		}
		if (result) {
			RE::PlaySound("UIMenuPrevNext");
		}
		ActivateOnHover();

		PopStyleColor(3);

		return result;
	}
}
