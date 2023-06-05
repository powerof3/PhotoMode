#pragma once

namespace ImGui
{
	bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items = -1);

	void PushMultiItemsWidthsAndLabels(const char* const labels[], int components, float w_full);

	bool DragIntNEx(const char* const labels[], int* v, int components, float v_speed, const int v_min[], const int v_max[], const char* display_format[], ImGuiSliderFlags flags = 0);
	bool DragInt2Ex(const char* const labels[], int* v, float v_speed, const int v_min[], const int v_max[], const char* display_format[], ImGuiSliderFlags flags = 0);

	void AlignForWidth(float width, float alignment = 0.5f);

    void AlignedImage(ImTextureID texID, const ImVec2& texture_size, const ImVec2& min, const ImVec2& max, const ImVec2& align);

	std::string LabelPrefix(const char* label);
	std::string LabelPrefix(const std::string& label);

	void CenteredText(const char* label, bool vertical = false);
	bool CenteredTextWithArrows(const char* label, const char* centerText);

	bool OnOffToggle(const char* label, bool* a_toggle, const char* on, const char* off);

	template <class E, std::size_t N>
	bool EnumSlider(const char* label, E* index, std::array<const char*, N> a_enum)
	{
		bool value_changed = false;

		std::int32_t uIndex;
		if constexpr (std::is_enum_v<E>) {
			uIndex = stl::to_underlying(*index);
		} else {
			uIndex = *index;
		}
		if (CenteredTextWithArrows(LabelPrefix(label).c_str(), TRANSLATE(a_enum[uIndex]))) {
			const bool pressedLeft = IsKeyPressed(ImGuiKey_LeftArrow) || IsKeyPressed(ImGuiKey_GamepadDpadLeft);

			const bool pressedRight = IsKeyPressed(ImGuiKey_RightArrow) ||
			                          IsKeyPressed(ImGuiKey_GamepadDpadRight) ||
			                          IsKeyPressed(ImGuiKey_Space) ||
			                          IsKeyPressed(ImGuiKey_Enter) ||
			                          IsKeyPressed(ImGuiKey_NavGamepadActivate);

			if (pressedLeft) {
				uIndex = (uIndex - 1 + N) % N;
			}
			if (pressedRight) {
				uIndex = (uIndex + 1) % N;
			}
			if (pressedLeft || pressedRight) {
				value_changed = true;
				*index = static_cast<E>(uIndex);
			}
		}
		return value_changed;
	}

	bool ThinSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags);
	bool ThinSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags);

	void ActivateOnHover();
	bool OpenTabOnHover(const char* a_label, ImGuiTabItemFlags flags = 0);

	template <class T>
	bool DragOnHover(const char* a_label, T* v, float v_speed = 1.0f, T v_min = 0, T v_max = 100, const char* format = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp)
	{
		const auto newLabel = LabelPrefix(a_label);

		bool result;
		if constexpr (std::is_floating_point_v<T>) {
			result = ImGui::DragFloat(newLabel.c_str(), v, v_speed, v_min, v_max, format ? format : "%.2f", flags);
		} else {
			result = ImGui::DragInt(newLabel.c_str(), v, v_speed, v_min, v_max, format ? format : "%d", flags);
		}
		ActivateOnHover();

		return result;
	}

	template <class T>
	bool Slider(const char* label, T* v, T v_min, T v_max, const char* format = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp)
	{
		const auto newLabel = LabelPrefix(label);

		bool result;
		if constexpr (std::is_floating_point_v<T>) {
			result = ImGui::ThinSliderFloat(newLabel.c_str(), v, v_min, v_max, format ? format : "%.2f", flags);
		} else {
			result = ImGui::ThinSliderInt(newLabel.c_str(), v, v_min, v_max, format ? format : "%d", flags);
		}
		ActivateOnHover();

		return result;
	}
}
