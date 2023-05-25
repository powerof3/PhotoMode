#pragma once

#include "ImGui/Util.h"

namespace PhotoMode
{
	namespace Override
	{
		inline RE::TESIdleForm*           resetRootIdle;
		inline RE::TESImageSpaceModifier* currentImod;

		// ugly but avoids polymorphism
		template <class T>
		struct Forms
		{
			Forms(std::string a_name) :
				name(std::move(a_name))
			{}

			void GetValidForms()
			{
				if constexpr (std::is_same_v<T, RE::TESIdleForm>) {
					const auto player = RE::PlayerCharacter::GetSingleton();
					edidFormMap.clear();
					for (auto& [edid, idle] : edidFormMap) {
						if (player->CanUseIdle(idle) && idle->CheckConditions(player, nullptr, true)) {
							editorIDs.push_back(edid);
						}
					}
				} else {
					if (edidFormMap.empty()) {
						for (const auto& form : RE::TESDataHandler::GetSingleton()->GetFormArray<T>()) {
							auto edid = EditorID::GetEditorID(form);
							if (edidFormMap.emplace(edid, form).second) {
								editorIDs.emplace_back(edid);
							}
						}
					}
					if constexpr (std::is_same_v<T, RE::TESWeather>) {
						const auto it = edidFormMap.find(EditorID::GetEditorID(RE::Sky::GetSingleton()->currentWeather));
						index = it != edidFormMap.end() ?
						            static_cast<std::int32_t>(std::distance(edidFormMap.begin(), it)) :
						            0;
					}
				}
			}

			void Apply(T* a_form)
			{
				const auto player = RE::PlayerCharacter::GetSingleton();

				if constexpr (std::is_same_v<T, RE::TESEffectShader>) {
					player->ApplyEffectShader(a_form);
				} else if constexpr (std::is_same_v<T, RE::BGSReferenceEffect>) {
					if (const auto effectShader = a_form->data.effectShader) {
						player->ApplyEffectShader(effectShader, -1, nullptr, a_form->data.flags.any(RE::BGSReferenceEffect::Flag::kFaceTarget), a_form->data.flags.any(RE::BGSReferenceEffect::Flag::kAttachToCamera));
					}
					if (const auto artObject = a_form->data.artObject) {
						player->ApplyArtObject(artObject, -1, nullptr, a_form->data.flags.any(RE::BGSReferenceEffect::Flag::kFaceTarget), a_form->data.flags.any(RE::BGSReferenceEffect::Flag::kAttachToCamera));
					}
				} else if constexpr (std::is_same_v<T, RE::TESIdleForm>) {
					if (const auto currentProcess = player->currentProcess) {
						currentProcess->PlayIdle(player, a_form, nullptr);
					}
				} else if constexpr (std::is_same_v<T, RE::TESWeather>) {
					RE::Sky::GetSingleton()->ForceWeather(a_form, true);
				} else if constexpr (std::is_same_v<T, RE::TESImageSpaceModifier>) {
					if (currentImod) {
						RE::ImageSpaceModifierInstanceForm::Stop(currentImod);
					}
					RE::ImageSpaceModifierInstanceForm::Trigger(a_form, 1.0, nullptr);
					currentImod = a_form;
				}
			}
			void Revert(bool a_resetIndex = true)
			{
				if (a_resetIndex) {
					if constexpr (std::is_same_v<T, RE::TESWeather>) {
						const auto it = edidFormMap.find(EditorID::GetEditorID(RE::Sky::GetSingleton()->currentWeather));
						index = it != edidFormMap.end() ?
						            static_cast<std::int32_t>(std::distance(edidFormMap.begin(), it)) :
						            0;
					} else {
						index = 0;
					}
				}

				const auto player = RE::PlayerCharacter::GetSingleton();
				if constexpr (std::is_same_v<T, RE::TESEffectShader> || std::is_same_v<T, RE::BGSReferenceEffect>) {
					if (const auto processLists = RE::ProcessLists::GetSingleton()) {
						const auto handle = player->CreateRefHandle();
						processLists->ForEachMagicTempEffect([&](RE::BSTempEffect& a_effect) {
							if (const auto referenceEffect = a_effect.As<RE::ReferenceEffect>()) {
								if (referenceEffect->target.get() == handle.get()) {
									referenceEffect->finished = true;
								}
							}
							return RE::BSContainer::ForEachResult::kContinue;
						});
					}
				} else if constexpr (std::is_same_v<T, RE::TESIdleForm>) {
					if (const auto currentProcess = player->currentProcess) {
						currentProcess->StopCurrentIdle(player, true);
						currentProcess->PlayIdle(player, resetRootIdle, nullptr);
					}
				} else if constexpr (std::is_same_v<T, RE::TESWeather>) {
                    const auto sky = RE::Sky::GetSingleton();

				    sky->ReleaseWeatherOverride();
					if (const auto currentWeather = sky->currentWeather) {
						sky->ResetWeather();
						sky->ForceWeather(currentWeather, false);
					}
				} else if constexpr (std::is_same_v<T, RE::TESImageSpaceModifier>) {
					if (currentImod) {
						RE::ImageSpaceModifierInstanceForm::Stop(currentImod);
						currentImod = nullptr;
					}
				}
			}

			T* GetFormResultFromCombo()
			{
				if (ImGui::ComboWithFilter(ImGui::LabelPrefix(name).c_str(), &index, editorIDs)) {
					return edidFormMap.find(editorIDs[index])->second;
				}
				return nullptr;
			}

			// members
			std::string                                             name;
			std::vector<std::string>                                editorIDs{};
			ankerl::unordered_dense::segmented_map<std::string, T*> edidFormMap{};
			std::int32_t                                            index{};
		};

		inline Forms<RE::TESEffectShader>       effectShaders{ "Effect Shaders" };
		inline Forms<RE::BGSReferenceEffect>    effectVFX{ "Visual Effects" };
		inline Forms<RE::TESWeather>            weathers{ "Weathers" };
		inline Forms<RE::TESIdleForm>           idles{ "Idles" };
		inline Forms<RE::TESImageSpaceModifier> imods{ "ImageSpace Modifiers" };

		void GetValidOverrides();

		void InstallHooks();
	}

	namespace MFG
	{
		inline constexpr std::array expressions{
			"NONE",
			"DIALOGUE ANGER",
			"DIALOGUE FEAR",
			"DIALOGUE HAPPY",
			"DIALOGUE SAD",
			"DIALOGUE SURPRISE",
			"DIALOGUE PUZZLED",
			"DIALOGUE DISGUSTED",
			"MOOD NEUTRAL",
			"MOOD ANGER",
			"MOOD FEAR",
			"MOOD HAPPY",
			"MOOD SAD",
			"MOOD SURPRISE",
			"MOOD PUZZLED",
			"MOOD DISGUSTED",
			"COMBAT ANGER",
			"COMBAT SHOUT"
		};
		inline constexpr std::array phonemes{
			"Aah",
			"BigAah",
			"BMP",
			"ChJSh",
			"DST",
			"Eee",
			"Eh",
			"FV",
			"I",
			"K",
			"N",
			"Oh",
			"OohQ",
			"R",
			"Th",
			"W"
		};
		inline constexpr std::array modifiers{
			"BlinkLeft",
			"BlinkRight",
			"BrowDownLeft",
			"BrowDownRight",
			"BrowInLeft",
			"BrowInRight",
			"BrowUpLeft",
			"BrowUpRight",
			"LookDown",
			"LookLeft",
			"LookRight",
			"LookUp",
			"SquintLeft",
			"SquintRight",
			"HeadPitch",
			"HeadRoll",
			"HeadYaw"
		};

		struct Expression
		{
			void ApplyExpression(RE::Actor* a_actor) const;

			std::int32_t modifier{ 0 };  // 0 is NONE
			float        strength{ 1.0f };
		};

		struct Modifier
		{
			void ApplyPhenome(std::uint32_t idx, RE::Actor* a_actor) const;
			void ApplyModifier(std::uint32_t idx, RE::Actor* a_actor) const;

			std::int32_t strength{ 0 };
		};

		inline Expression expressionData;
		inline Modifier   phonemeData[phonemes.size()];
		inline Modifier   modifierData[modifiers.size()];

		void RevertAllModifiers();
	}

	namespace Grid
	{
		enum Type
		{
			kDisabled,
			kRuleOfThirds,
			kDiagonal,
			kTriangle,
			kGoldenRatio,
			//kGoldenSpiral,
			kGrid
		};

		inline constexpr std::array gridEnum{
			"NONE",
			"THIRDS",
			"DIAGONAL",
			"TRIANGLE",
			"GOLDEN RATIO",
			//"GOLDEN SPIRAL",
			"GRID"
		};

		inline Type gridType{ kDisabled };

		void Draw();
	}
}
