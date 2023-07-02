#pragma once

namespace ImGui::Renderer
{
	inline std::atomic initialized{ false };

	namespace DisplayTweaks
	{
		inline float resolutionScale{ 1.0f };
		inline bool  borderlessUpscale{ false };
	}

	float GetResolutionScale();

	void LoadSettings(const CSimpleIniA& a_ini);
	void Install();
}
