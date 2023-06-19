#include "Time.h"

#include "ImGui/Util.h"
#include "Translation.h"

namespace PhotoMode
{
	void Time::OriginalState::Get()
	{
		freezeTime = RE::Main::GetSingleton()->freezeTime;
		globalTimeMult = RE::BSTimer::GetCurrentGlobalTimeMult();

		const auto calendar = RE::Calendar::GetSingleton();
		timescale = calendar->GetTimescale();
		gameHour = calendar->gameHour->value;
	}

	void Time::OriginalState::Revert() const
	{
		RE::Main::GetSingleton()->freezeTime = freezeTime;
		RE::BSTimer::GetCurrentGlobalTimeMult() = globalTimeMult;

		const auto calendar = RE::Calendar::GetSingleton();
		calendar->timeScale->value = timescale;
		// shouldn't revert time?
		// calendar->gameHour->value = gameHour;
	}

	void Time::GetOriginalState()
	{
		originalState.Get();
		weathers.InitForms();
	}

	void Time::RevertState()
	{
		originalState.Revert();

		// revert timescale mult
		currentTimescaleMult = 1.0f;

		// revert weather
		weathers.ResetIndex();
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
		ImGui::OnOffToggle("$PM_FreezeTime"_T, &RE::Main::GetSingleton()->freezeTime, "$PM_YES"_T, "$PM_NO"_T);
		ImGui::Slider("$PM_GlobalTimeMult"_T, &RE::BSTimer::GetCurrentGlobalTimeMult(), 0.0f, 2.0f);

		ImGui::Dummy({ 0, 5 });

		auto& gameHour = RE::Calendar::GetSingleton()->gameHour->value;
		ImGui::Slider("$PM_GameHour"_T, &gameHour, 0.0f, 23.99f, std::format("{:%I:%M %p}", std::chrono::duration<float, std::ratio<3600>>(gameHour)).c_str());

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
