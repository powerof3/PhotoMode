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
					if (!resetRootIdle) {
						resetRootIdle = RE::TESForm::LookupByEditorID<RE::TESIdleForm>("ResetRoot");
					}
					const auto player = RE::PlayerCharacter::GetSingleton();
					formEDIDs.clear();
					for (auto& [edid, idle] : forms) {
						if (player->CanUseIdle(idle) && idle->CheckConditions(player, nullptr, false)) {
							formEDIDs.push_back(edid);
						}
					}
				} else {
					if (forms.empty()) {
						for (const auto& form : RE::TESDataHandler::GetSingleton()->GetFormArray<T>()) {
							auto edid = EditorID::GetEditorID(form);
							if (forms.emplace(edid, form).second) {
								formEDIDs.emplace_back(edid);
							}
						}
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
					index = 0;
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
					RE::Sky::GetSingleton()->ResetWeather();
				} else if constexpr (std::is_same_v<T, RE::TESImageSpaceModifier>) {
					if (currentImod) {
						RE::ImageSpaceModifierInstanceForm::Stop(currentImod);
						currentImod = nullptr;
					}
				}
			}

			T* GetFormResultFromCombo()
			{
				if (ImGui::ComboWithFilter(ImGui::LabelPrefix(name).c_str(), &index, formEDIDs)) {
					return forms.find(formEDIDs[index])->second;
				}
				return nullptr;
			}

			// members
			std::string                         name;
			std::vector<std::string>            formEDIDs{};
			std::unordered_map<std::string, T*> forms{};
			std::int32_t                        index{};
		};

		inline Forms<RE::TESEffectShader>       effectShaders{ "Effect Shaders" };
		inline Forms<RE::BGSReferenceEffect>    effectVFX{ "Visual Effects" };
		inline Forms<RE::TESWeather>            weathers{ "Weathers" };
		inline Forms<RE::TESIdleForm>           idles{ "Idles" };
		inline Forms<RE::TESImageSpaceModifier> imods{ "ImageSpace Modifiers" };

		void GetValidOverrides();
		void RevertOverrides(bool& a_vfx, bool& a_weather, bool& a_idle);

		void InstallHooks();
	}

	namespace MFG
	{
		inline constexpr std::array expressions{
			"Dialogue Anger",
			"Dialogue Fear",
			"Dialogue Happy",
			"Dialogue Sad",
			"Dialogue Surprise",
			"Dialogue Puzzled",
			"Dialogue Disgusted",
			"Mood Neutral",
			"Mood Anger",
			"Mood Fear",
			"Mood Happy",
			"Mood Sad",
			"Mood Surprise",
			"Mood Puzzled",
			"Mood Disgusted",
			"Combat Anger",
			"Combat Shout"
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

			std::int32_t modifier{ 0 };
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
}
