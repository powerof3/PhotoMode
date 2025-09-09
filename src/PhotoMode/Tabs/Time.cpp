#include "Time.h"

#include "ImGui/Widgets.h"

namespace PhotoMode
{
	void Time::OriginalState::Get()
	{
		freezeTime = RE::Main::GetSingleton()->freezeTime;
		globalTimeMult = RE::BSTimer::QGlobalTimeMultiplier();

		const auto calendar = RE::Calendar::GetSingleton();
		timescale = calendar->GetTimescale();
		gameHour = calendar->gameHour->value;
	}

	void Time::OriginalState::Revert() const
	{
		RE::Main::GetSingleton()->freezeTime = freezeTime;
		RE::BSTimer::GetSingleton()->SetGlobalTimeMultiplier(globalTimeMult, true);

		const auto calendar = RE::Calendar::GetSingleton();
		calendar->timeScale->value = timescale;
		calendar->gameHour->value = gameHour;
	}

	void Time::GetOriginalState()
	{
		originalState.Get();
		weathers.InitForms();
	}

	void Time::RevertState()
	{
		originalState.Revert();

		// is this needed when value is updated every frame?
		currentTimescaleMult = 1.0f;
		currentGlobalTimeMult = 1.0f;

		// revert weather
		weathers.Reset();
		if (weatherForced) {
			const auto sky = RE::Sky::GetSingleton();
			sky->ReleaseWeatherOverride();
			sky->ResetWeather();
			if (originalWeather) {
				sky->ForceWeather(originalWeather, false);
			}
			weatherForced = false;
		}
	}

	void Time::OnFrameUpdate() const
	{
		if (weatherForced) {
			RE::Sky::GetSingleton()->lastWeatherUpdate = RE::Calendar::GetSingleton()->gameHour->value;
		}
	}

	void Time::Draw()
	{
		ImGui::CheckBox("$PM_FreezeTime"_T, &RE::Main::GetSingleton()->freezeTime);

		currentGlobalTimeMult = RE::BSTimer::QGlobalTimeMultiplier();
		if (ImGui::Slider("$PM_GlobalTimeMult"_T, &currentGlobalTimeMult, 0.01f, 2.0f)) {
			RE::BSTimer::GetSingleton()->SetGlobalTimeMultiplier(currentGlobalTimeMult, true);
		}

		ImGui::Dummy({ 0, 5 });

		auto& gameHour = RE::Calendar::GetSingleton()->gameHour->value;
		ImGui::Slider("$PM_GameHour"_T, &gameHour, 0.0f, 23.99f, std::format("{:%I:%M %p}", std::chrono::duration<float, std::ratio<3600>>(gameHour)).c_str(), ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput);

		if (ImGui::DragOnHover("$PM_TimeScaleMult"_T, &currentTimescaleMult, 10, 1.0f, 1000.0f, "%.fX")) {
			RE::Calendar::GetSingleton()->timeScale->value = originalState.timescale * currentTimescaleMult;
		} 

		ImGui::BeginDisabled(RE::Sky::GetSingleton()->mode == RE::Sky::Mode::kInterior);
		{
			weathers.GetFormResultFromCombo([&](const auto& weather) {
				RE::Sky::GetSingleton()->ForceWeather(weather, true);
				weatherForced = true;
			});
		}
		ImGui::EndDisabled();
	}
}
