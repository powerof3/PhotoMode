#include "Styles.h"

#include "IconsFonts.h"
#include "Renderer.h"
#include "Settings.h"

namespace ImGui
{
	void Styles::ConvertVec4StylesToU32()
	{
		frameBG_WidgetU32 = ColorConvertFloat4ToU32(frameBG_Widget);
		frameBG_WidgetActiveU32 = ColorConvertFloat4ToU32(frameBG_WidgetActive);
		gridLinesU32 = ColorConvertFloat4ToU32(gridLines);
		sliderBorderU32 = ColorConvertFloat4ToU32(sliderBorder);
		sliderBorderActiveU32 = ColorConvertFloat4ToU32(sliderBorderActive);
		iconDisabledU32 = ColorConvertFloat4ToU32(iconDisabled);
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
			return iconDisabled;
		case USER_STYLE::kComboBoxTextBox:
			return comboBoxTextBox;
		case USER_STYLE::kComboBoxText:
			return comboBoxText;
		default:
			return ImVec4();
		}
	}

	float Styles::GetVar(USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kButtons:
			return buttonScale;
		case USER_STYLE::kCheckbox:
			return checkboxScale;
		case USER_STYLE::kStepper:
			return stepperScale;
		case USER_STYLE::kSeparatorThickness:
			return separatorThickness;
		case USER_STYLE::kGridLines:
			return gridThickness;
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
		auto get_value = [&]<typename T>(T& a_value, const char* a_section, const char* a_key) {
			a_value = ToStyle<T>(a_ini.GetValue(a_section, a_key, ToString(a_value).c_str()));
			a_ini.SetValue(a_section, a_key, ToString(a_value).c_str());
		};

		get_value(iconDisabled, "Icon", "rDisabledColor");
		get_value(buttonScale, "Icon", "fButtonScale");
		get_value(checkboxScale, "Icon", "fCheckboxScale");
		get_value(stepperScale, "Icon", "fStepperScale");

		get_value(background, "Window", "rBackgroundColor");
		get_value(border, "Window", "rBorderColor");
		get_value(borderSize, "Window", "fBorderSize");

		get_value(comboBoxTextBox, "ComboBox", "rTextBoxColor");
		get_value(frameBG, "ComboBox", "rListBoxColor");
		get_value(comboBoxText, "ComboBox", "rTextColor");
		get_value(button, "ComboBox", "rArrowButtonColor");

		get_value(sliderGrab, "Slider", "rColor");
		get_value(sliderGrabActive, "Slider", "rActiveColor");
		get_value(sliderBorder, "Slider", "rBorderColor");
		get_value(sliderBorderActive, "Slider", "rBorderActiveColor");

		get_value(text, "Text", "rColor");
		get_value(textDisabled, "Text", "rDisabledColor");

		get_value(frameBG_Widget, "Widget", "rBackgroundColor");
		get_value(frameBG_WidgetActive, "Widget", "rBackgroundActiveColor");
		get_value(header, "Widget", "rHighlightColor");
		get_value(gridLines, "Widget", "rGridColor");
		get_value(gridThickness, "Widget", "fGridThickness");
		get_value(separator, "Widget", "rSeparatorColor");
		get_value(separatorThickness, "Widget", "fSeparatorThickness");
		get_value(tab, "Widget", "rTabColor");
		get_value(tabHovered, "Widget", "rTabActiveColor");

		ConvertVec4StylesToU32();
	}

	void Styles::OnStyleRefresh()
	{
		if (!refreshStyle) {
			return;
		}

		LoadStyles();

		auto& style = GetStyle();
		auto& colors = style.Colors;

		style.WindowBorderSize = borderSize;
		style.TabRounding = 0.0f;

		colors[ImGuiCol_WindowBg] = background;
		colors[ImGuiCol_ChildBg] = background;

		colors[ImGuiCol_Border] = border;
		colors[ImGuiCol_Separator] = separator;

		colors[ImGuiCol_Text] = text;
		colors[ImGuiCol_TextDisabled] = textDisabled;

		colors[ImGuiCol_FrameBg] = frameBG;
		colors[ImGuiCol_FrameBgHovered] = colors[ImGuiCol_FrameBg];
		colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_FrameBg];

		colors[ImGuiCol_SliderGrab] = sliderGrab;
		colors[ImGuiCol_SliderGrabActive] = sliderGrabActive;

		colors[ImGuiCol_Button] = button;
		colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_Button];
		colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_Button];

		colors[ImGuiCol_Header] = header;
		colors[ImGuiCol_HeaderHovered] = colors[ImGuiCol_Header];
		colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_Header];

		colors[ImGuiCol_Tab] = tab;
		colors[ImGuiCol_TabHovered] = tabHovered;
		colors[ImGuiCol_TabActive] = colors[ImGuiCol_TabHovered];
		colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
		colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

		colors[ImGuiCol_NavHighlight] = ImVec4();

		style.ScaleAllSizes(Renderer::GetResolutionScale());

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
