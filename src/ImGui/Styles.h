#pragma once

namespace ImGui
{
	enum class USER_STYLE
	{
		kIconDisabled,
		kButtons,
		kCheckbox,
		kStepper,
		kSeparatorThickness,
		kGridLines,
		kSliderBorder,
		kSliderBorderActive,
		kFrameBG_Widget,
		kFrameBG_WidgetActive,
		kComboBoxTextBox,
		kComboBoxText
	};

	class Styles : public ISingleton<Styles>
	{
	public:
		ImU32  GetColorU32(USER_STYLE a_style) const;
		ImVec4 GetColorVec4(USER_STYLE a_style) const;
		float  GetVar(USER_STYLE a_style) const;

		void LoadStyles();
		void LoadStyles(CSimpleIniA& a_ini);

		void OnStyleRefresh();
		void RefreshStyle();

	private:
		void ConvertVec4StylesToU32();

		template <class T>
		T ToStyle(const std::string& a_str);
		template <class T>
		std::string ToString(const T& a_style);

		// members
		// unused, helpers
		float bgAlpha{ 0.68f };
		float disabledAlpha{ 0.30f };

		float buttonScale{ 0.5f };
		float checkboxScale{ 0.5f };
		float stepperScale{ 0.5f };

		ImVec4 iconDisabled{ 1.0f, 1.0f, 1.0f, disabledAlpha };

		ImVec4 background{ 0.0f, 0.0f, 0.0f, bgAlpha };

		ImVec4 border{ 0.396f, 0.404f, 0.412f, bgAlpha };
		float  borderSize{ 3.5f };

		ImVec4 text{ 1.0f, 1.0f, 1.0f, 1.0f };
		ImVec4 textDisabled{ 1.0f, 1.0f, 1.0f, disabledAlpha };

		ImVec4 comboBoxText{ 1.0f, 1.0f, 1.0f, 0.8f };
		ImVec4 comboBoxTextBox{ 0.0f, 0.0f, 0.0f, 1.0f };
		ImVec4 button{ 0.0f, 0.0f, 0.0f, bgAlpha };  // arrow button

		ImVec4 frameBG{ 0.2f, 0.2f, 0.2f, bgAlpha };
		ImVec4 frameBG_Widget{ 1.0f, 1.0f, 1.0f, 0.06275f };
		ImVec4 frameBG_WidgetActive{ 1.0f, 1.0f, 1.0f, 0.2f };

		ImVec4 sliderGrab{ 1.0f, 1.0f, 1.0f, 0.245f };
		ImVec4 sliderGrabActive{ 1.0f, 1.0f, 1.0f, 0.531f };

		ImVec4 header{ 1.0f, 1.0f, 1.0f, 0.1f };  // select highlight
		ImVec4 tab{ 0.0f, 0.0f, 0.0f, 0.0f };
		ImVec4 tabHovered{ 0.2f, 0.2f, 0.2f, 1.0f };

		ImVec4 gridLines{ 1.0f, 1.0f, 1.0f, 0.3333f };
		float  gridThickness{ 2.5f };

		ImVec4 separator{ 0.396f, 0.404f, 0.412f, bgAlpha };
		float  separatorThickness{ 3.5f };

		ImVec4 sliderBorder{ 1.0f, 1.0f, 1.0f, 0.2431f };
		ImVec4 sliderBorderActive{ 1.0f, 1.0f, 1.0f, 0.8f };

		// for values that require ImU32 colours
		// can't read directly, repeated (ARGB -> RGBA -> ARGB) conversion is broken
		ImU32 frameBG_WidgetU32;
		ImU32 frameBG_WidgetActiveU32;
		ImU32 gridLinesU32;
		ImU32 sliderBorderU32;
		ImU32 sliderBorderActiveU32;
		ImU32 iconDisabledU32;

		bool refreshStyle{ false };
	};

	ImU32  GetUserStyleColorU32(USER_STYLE a_style);
	ImVec4 GetUserStyleColorVec4(USER_STYLE a_style);
	float  GetUserStyleVar(USER_STYLE a_style);

	template <class T>
	inline T Styles::ToStyle(const std::string& a_str)
	{
		if constexpr (std::is_same_v<ImVec4, T>) {
			static srell::regex pattern("([0-9]+),([0-9]+),([0-9]+),([0-9]+)");
			srell::smatch       matches;
			if (srell::regex_match(a_str, matches, pattern)) {
				auto red = std::stoi(matches[1]);
				auto green = std::stoi(matches[2]);
				auto blue = std::stoi(matches[3]);
				auto alpha = std::stoi(matches[4]);
				return { red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f };
			}
			return T{};
		} else {
			return string::to_num<T>(a_str);
		}
	}

	template <class T>
	inline std::string Styles::ToString(const T& a_style)
	{
		if constexpr (std::is_same_v<ImVec4, T>) {
			return std::format("{},{},{},{}", std::round(255.0f * a_style.x), std::round(255.0f * a_style.y), std::round(255.0f * a_style.z), std::round(255.0f * a_style.w));
		} else {
			return std::format("{:.2f}", a_style);
		}
	}
}
