#include "Styles.h"

#include "IconsFonts.h"
#include "Renderer.h"
#include "Settings.h"

namespace ImGui
{
	void Styles::ConvertVec4StylesToU32()
	{
		frameBG_WidgetU32 = ColorConvertFloat4ToU32(user.frameBG_Widget);
		frameBG_WidgetActiveU32 = ColorConvertFloat4ToU32(user.frameBG_WidgetActive);
		gridLinesU32 = ColorConvertFloat4ToU32(user.gridLines);
		sliderBorderU32 = ColorConvertFloat4ToU32(user.sliderBorder);
		sliderBorderActiveU32 = ColorConvertFloat4ToU32(user.sliderBorderActive);
		iconDisabledU32 = ColorConvertFloat4ToU32(user.iconDisabled);
	}

	ImU32 Styles::GetColorU32(USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kIconDisabled:
			return iconDisabledU32;
		case USER_STYLE::kGridLines:
			return gridLinesU32;
		case USER_STYLE::kSliderBorder:
			return sliderBorderU32;
		case USER_STYLE::kSliderBorderActive:
			return sliderBorderActiveU32;
		case USER_STYLE::kFrameBG_Widget:
			return frameBG_WidgetU32;
		case USER_STYLE::kFrameBG_WidgetActive:
			return frameBG_WidgetActiveU32;
		default:
			return ImU32();
		}
	}

	ImVec4 Styles::GetColorVec4(USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kIconDisabled:
			return user.iconDisabled;
		case USER_STYLE::kComboBoxTextBox:
			return user.comboBoxTextBox;
		case USER_STYLE::kComboBoxText:
			return user.comboBoxText;
		default:
			return ImVec4();
		}
	}

	float Styles::GetVar(USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kButtons:
			return user.buttonScale;
		case USER_STYLE::kCheckbox:
			return user.checkboxScale;
		case USER_STYLE::kStepper:
			return user.stepperScale;
		case USER_STYLE::kSeparatorThickness:
			return user.separatorThickness;
		case USER_STYLE::kGridLines:
			return user.gridThickness;
		default:
			return 1.0f;
		}
	}

	void Styles::LoadStyles()
	{
		Settings::GetSingleton()->SerializeStyles([this](auto& ini) {
			LoadStyles(ini);
		});
	}

	void Styles::LoadStyles(CSimpleIniA& a_ini)
	{
#define GET_VALUE(a_value, a_section, a_key)                                                                                                        \
	bool a_value##_hex = false;                                                                                                                     \
	std::tie(user.a_value, a_value##_hex) = ToStyle<decltype(user.a_value)>(a_ini.GetValue(a_section, a_key, ToString(def.a_value, true).c_str())); \
	a_ini.SetValue(a_section, a_key, ToString(user.a_value, a_value##_hex).c_str());

		GET_VALUE(iconDisabled, "Icon", "rDisabledColor");
		GET_VALUE(buttonScale, "Icon", "fButtonScale");
		GET_VALUE(checkboxScale, "Icon", "fCheckboxScale");
		GET_VALUE(stepperScale, "Icon", "fStepperScale");

		GET_VALUE(background, "Window", "rBackgroundColor");
		GET_VALUE(border, "Window", "rBorderColor");
		GET_VALUE(borderSize, "Window", "fBorderSize");

		GET_VALUE(comboBoxTextBox, "ComboBox", "rTextBoxColor");
		GET_VALUE(frameBG, "ComboBox", "rListBoxColor");
		GET_VALUE(comboBoxText, "ComboBox", "rTextColor");
		GET_VALUE(button, "ComboBox", "rArrowButtonColor");

		GET_VALUE(sliderGrab, "Slider", "rColor");
		GET_VALUE(sliderGrabActive, "Slider", "rActiveColor");
		GET_VALUE(sliderBorder, "Slider", "rBorderColor");
		GET_VALUE(sliderBorderActive, "Slider", "rBorderActiveColor");

		GET_VALUE(text, "Text", "rColor");
		GET_VALUE(textDisabled, "Text", "rDisabledColor");

		GET_VALUE(frameBG_Widget, "Widget", "rBackgroundColor");
		GET_VALUE(frameBG_WidgetActive, "Widget", "rBackgroundActiveColor");
		GET_VALUE(header, "Widget", "rHighlightColor");
		GET_VALUE(gridLines, "Widget", "rGridColor");
		GET_VALUE(gridThickness, "Widget", "fGridThickness");
		GET_VALUE(separator, "Widget", "rSeparatorColor");
		GET_VALUE(separatorThickness, "Widget", "fSeparatorThickness");
		GET_VALUE(tab, "Widget", "rTabColor");
		GET_VALUE(tabHovered, "Widget", "rTabActiveColor");

#undef GET_VALUE

		ConvertVec4StylesToU32();
	}

	void Styles::OnStyleRefresh()
	{
		if (!refreshStyle) {
			return;
		}

		LoadStyles();

		ImGuiStyle style;
		auto&      colors = style.Colors;

		style.WindowBorderSize = user.borderSize;
		style.TabBarBorderSize = user.borderSize;
		style.TabRounding = 0.0f;

		colors[ImGuiCol_WindowBg] = user.background;
		colors[ImGuiCol_ChildBg] = user.background;

		colors[ImGuiCol_Border] = user.border;
		colors[ImGuiCol_Separator] = user.separator;

		colors[ImGuiCol_Text] = user.text;
		colors[ImGuiCol_TextDisabled] = user.textDisabled;

		colors[ImGuiCol_FrameBg] = user.frameBG;
		colors[ImGuiCol_FrameBgHovered] = colors[ImGuiCol_FrameBg];
		colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_FrameBg];

		colors[ImGuiCol_SliderGrab] = user.sliderGrab;
		colors[ImGuiCol_SliderGrabActive] = user.sliderGrabActive;

		colors[ImGuiCol_Button] = user.button;
		colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_Button];
		colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_Button];

		colors[ImGuiCol_Header] = user.header;
		colors[ImGuiCol_HeaderHovered] = colors[ImGuiCol_Header];
		colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_Header];

		colors[ImGuiCol_Tab] = user.tab;
		colors[ImGuiCol_TabHovered] = user.tabHovered;
		colors[ImGuiCol_TabActive] = colors[ImGuiCol_TabHovered];
		colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
		colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

		colors[ImGuiCol_NavHighlight] = ImVec4();

		style.ScaleAllSizes(Renderer::GetResolutionScale());
		ImGui::GetStyle() = style;

		// reload fonts/icons

		MANAGER(IconFont)->LoadSettings();

		MANAGER(IconFont)->ReloadFonts();
		MANAGER(IconFont)->ResizeIcons();

		refreshStyle = false;
	}

	void Styles::RefreshStyle()
	{
		refreshStyle = true;
	}

	ImU32 GetUserStyleColorU32(USER_STYLE a_style)
	{
		return Styles::GetSingleton()->GetColorU32(a_style);
	}

	ImVec4 GetUserStyleColorVec4(USER_STYLE a_style)
	{
		return Styles::GetSingleton()->GetColorVec4(a_style);
	}

	float GetUserStyleVar(USER_STYLE a_style)
	{
		return Styles::GetSingleton()->GetVar(a_style);
	}
}
