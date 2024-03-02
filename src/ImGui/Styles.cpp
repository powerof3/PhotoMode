#include "Styles.h"

#include "IconsFonts.h"
#include "Renderer.h"
#include "Settings.h"

namespace ImGui
{
	void Styles::RestoreVanillaStyle()
	{
		bgAlpha = 0.68f;
		disabledAlpha = 0.30f;
		windowBorderSize = 3.5f;
		separatorThickness = 3.5f;
		gridThickness = 2.5f;
		iconScale = 0.5f;

		background = ImVec4(0.0f, 0.0f, 0.0f, bgAlpha);
		border = ImVec4(0.396f, 0.404f, 0.412f, bgAlpha);
		separator = ImVec4(0.396f, 0.404f, 0.412f, bgAlpha);

		text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		textDisabled = ImVec4(1.0f, 1.0f, 1.0f, disabledAlpha);
		textComboBox = ImVec4(1.0f, 1.0f, 1.0f, 0.8f);

		frameBG = ImVec4(0.2f, 0.2f, 0.2f, bgAlpha);
		frameBG_Widget = ImVec4(1.0f, 1.0f, 1.0f, 0.06275f);
		frameBG_WidgetActive = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
		frameBG_ComboBox = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

		sliderGrab = ImVec4(1.0f, 1.0f, 1.0f, 0.245f);
		sliderGrabActive = ImVec4(1.0f, 1.0f, 1.0f, 0.531f);
		button = ImVec4(0.0f, 0.0f, 0.0f, bgAlpha);
		header = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
		tab = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		tabHovered = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);

		gridLines = ImVec4(1.0f, 1.0f, 1.0f, 0.3333f);
		sliderBorder = ImVec4(1.0f, 1.0f, 1.0f, 0.2431f);
		sliderBorderActive = ImVec4(1.0f, 1.0f, 1.0f, 0.8f);

		ConvertVec4StylesToU32();
	}

	void Styles::ConvertVec4StylesToU32()
	{
		frameBG_WidgetU32 = ColorConvertFloat4ToU32(frameBG_Widget);
		frameBG_WidgetActiveU32 = ColorConvertFloat4ToU32(frameBG_WidgetActive);
		gridLinesU32 = ColorConvertFloat4ToU32(gridLines);
		sliderBorderU32 = ColorConvertFloat4ToU32(sliderBorder);
		sliderBorderActiveU32 = ColorConvertFloat4ToU32(sliderBorderActive);
	}

	ImU32 Styles::GetColorU32(USER_STYLE a_style) const
	{
		switch (a_style) {
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
		case USER_STYLE::kText_ComboBox:
			return textComboBox;
		case USER_STYLE::kFrameBG_ComboBox:
			return frameBG_ComboBox;
		default:
			return ImVec4();
		}
	}

	float Styles::GetVar(USER_STYLE a_style) const
	{
		switch (a_style) {
		case USER_STYLE::kIconScale:
			return iconScale;
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

		get_value(iconScale, "Icon", "fScale");

		get_value(background, "Window", "rBackground");
		get_value(border, "Window", "rBorder");
		get_value(windowBorderSize, "Window", "fBorderSize");

		get_value(frameBG, "Frame", "rBackground");
		get_value(frameBG_Widget, "Frame", "rBackgroundWidget");
		get_value(frameBG_WidgetActive, "Frame", "rBackgroundWidgetActive");
		get_value(frameBG_ComboBox, "Frame", "rBackgroundComboBox");

		get_value(gridLines, "Grid", "rGrid");
		get_value(gridThickness, "Grid", "fThickness");

		get_value(separator, "Separator", "rSeparator");
		get_value(separatorThickness, "Separator", "fThickness");

		get_value(sliderGrab, "Slider", "rSlider");
		get_value(sliderGrabActive, "Slider", "rSliderActive");
		get_value(sliderBorder, "Slider", "rBorder");
		get_value(sliderBorderActive, "Slider", "rBorderActive");

		get_value(tab, "Tab", "rTab");
		get_value(tabHovered, "Tab", "rTabActive");

		get_value(text, "Text", "rText");
		get_value(textDisabled, "Text", "rTextDisabled");
		get_value(textComboBox, "Text", "rTextComboBox");

		get_value(button, "Widget", "rButton");
		get_value(header, "Widget", "rHeader");

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

		style.WindowBorderSize = windowBorderSize;
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
