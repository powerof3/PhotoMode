#pragma once

#include "ImGui/FormComboBox.h"

namespace PhotoMode
{
	class Time
	{
	public:
		void GetOriginalState();
		void RevertState();

		void OnFrameUpdate() const;
		void Draw();

	private:
		struct OriginalState
		{
			void Get();
			void Revert() const;

			// members
			bool  freezeTime{ false };
			float globalTimeMult{ 1.0f };
			float gameHour{};
			float timescale{};
		};

		// members
		OriginalState originalState{};

		float currentGlobalTimeMult{};
		float currentTimescaleMult{};

		ImGui::FormComboBoxFiltered<RE::TESWeather> weathers{ "$PM_Weathers" };
		RE::TESWeather*                             originalWeather{ nullptr };
		bool                                        weatherForced{ false };
	};
}
