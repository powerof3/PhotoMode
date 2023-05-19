#pragma once

namespace ImGui
{
	bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items = -1);

	void PushMultiItemsWidthsAndLabels(const char* const labels[], int components, float w_full);

	bool DragIntNEx(const char* const labels[], int* v, int components, float v_speed, const int v_min[], const int v_max[], const char* display_format[], ImGuiSliderFlags flags = 0);
	bool DragInt2Ex(const char* const labels[], int* v, float v_speed, const int v_min[], const int v_max[], const char* display_format[], ImGuiSliderFlags flags = 0);

	std::string LabelPrefix(const char* label);
	std::string LabelPrefix(const std::string& label);

	void CenterLabel(const char* label);
	void CenteredTextWithArrows(const char* label, const char* centerText);

	bool OnOffToggle(const char* label, bool* a_toggle, const char* on = "YES", const char* off = "NO");

	template <class E, std::size_t N>
	void EnumSlider(const char* label, E* index, std::array<const char*, N> a_enum)
	{
		std::int32_t uIndex = stl::to_underlying(*index);
		CenteredTextWithArrows(LabelPrefix(label).c_str(), a_enum[uIndex]);
		if (IsItemHovered()) {
			const bool pressedLeft = IsKeyPressed(ImGuiKey_LeftArrow);
			const bool pressedRight = IsKeyPressed(ImGuiKey_RightArrow) || IsKeyPressed(ImGuiKey_Space) || IsKeyPressed(ImGuiKey_Enter);
			if (pressedLeft) {
				uIndex = (uIndex + 1) % N;
			}
			if (pressedRight) {
				uIndex = (uIndex - 1 + N) % N;
			}
			if (pressedLeft || pressedRight) {
				*index = static_cast<E>(uIndex);
			}
		}
	}

	bool ThinSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags);
	bool ThinSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags);

	void ActivateOnHover(const char* a_label);
    bool OpenTabOnHover(const char* a_label, ImGuiTabItemFlags flags = 0);

    template <class T>
    bool DragOnHover(const char* a_label, T* v, float v_speed = 1.0f, T v_min = 0, T v_max = 100, const char* format = nullptr)
    {
		const auto newLabel = LabelPrefix(a_label);

		bool result;
		if constexpr (std::is_floating_point_v<T>) {
			result = ImGui::DragFloat(newLabel.c_str(), v, v_speed, v_min, v_max, format ? format : "%f", ImGuiSliderFlags_AlwaysClamp);
		} else {
			result = ImGui::DragInt(newLabel.c_str(), v, v_speed, v_min, v_max, format ? format : "%d", ImGuiSliderFlags_AlwaysClamp);
		}
		ActivateOnHover(newLabel.c_str());

        return result;
    }

    template <class T>
	bool Slider(const char* label, T* v, T v_min, T v_max)
	{
        const auto newLabel = LabelPrefix(label);

	    bool result;
		if constexpr (std::is_floating_point_v<T>) {
			result = ImGui::ThinSliderFloat(newLabel.c_str(), v, v_min, v_max, "%f", ImGuiSliderFlags_AlwaysClamp);
		} else {
			result = ImGui::ThinSliderInt(newLabel.c_str(), v, v_min, v_max, "%d", ImGuiSliderFlags_AlwaysClamp);
		}
		ActivateOnHover(newLabel.c_str());

		return result;
	}
}
