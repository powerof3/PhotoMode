#pragma once

#include "ImGui/Util.h"

namespace PhotoMode
{
	namespace Override
	{
		constexpr auto allMods = "$PM_ALL"sv;
		constexpr auto ffForms = "$PM_FF_Forms"sv;

		inline RE::TESIdleForm*           resetRootIdle;
		inline RE::TESImageSpaceModifier* currentImod;
		inline RE::TESWeather*            currentWeather;

		template <class T>
		class Forms
		{
		public:
			void AddForm(const std::string& a_edid, T* a_form)
			{
				if (edidForms.emplace(a_edid, a_form).second) {
					edids.push_back(a_edid);
				}
			}
			void UpdateValidForms()
			{
				if constexpr (std::is_same_v<T, RE::TESIdleForm>) {
					const auto player = RE::PlayerCharacter::GetSingleton();

					edids.clear();
					for (auto& [edid, idle] : edidForms) {
						if (player->CanUseIdle(idle) && idle->CheckConditions(player, nullptr, false)) {
							edids.push_back(edid);
						}
					}
				}
			}
			void ResetIndex()
			{
				if constexpr (std::is_same_v<T, RE::TESWeather>) {
					const auto it = edidForms.find(EditorID::GetEditorID(currentWeather));
					if (it != edidForms.end()) {
						index = static_cast<std::int32_t>(std::distance(edidForms.begin(), it));
					} else {
						index = 0;
					}
				} else {
					index = 0;
				}
			}

			T* GetComboWithFilterResult()
			{
				if (ImGui::ComboWithFilter("##forms", &index, edids)) {
					// avoid losing focus
					ImGui::SetKeyboardFocusHere(-1);
					return edidForms.find(edids[index])->second;
				}
				return nullptr;
			}

		private:
			// members
			StringMap<T*>            edidForms{};
			std::vector<std::string> edids{};
			std::int32_t             index{};
		};

		// modName, forms
		template <class T>
		class FilteredForms
		{
		public:
			FilteredForms(std::string a_name) :
				name(std::move(a_name))
			{}

			void AddForm(const std::string& a_edid, T* a_form)
			{
				std::string modName;
				if (auto file = a_form->GetFile(0)) {
					modName = file->fileName;
				} else {
					modName = ffForms;
				}

				modNameForms[allMods].AddForm(a_edid, a_form);
				modNameForms[modName].AddForm(a_edid, a_form);
			}
			void InitForms()
			{
				if constexpr (!std::is_same_v<T, RE::TESIdleForm>) {
					if (modNameForms.empty()) {
						for (const auto& form : RE::TESDataHandler::GetSingleton()->GetFormArray<T>()) {
							AddForm(EditorID::GetEditorID(form), form);
						}
					}
				}

				// ALL
				// ...mods
				// FF FORMS

				if (modNames.empty()) {
					modNames.reserve(modNameForms.size());
					modNames.emplace_back(TRANSLATE_S(allMods));

					for (const auto& file : RE::TESDataHandler::GetSingleton()->files) {
						if (modNameForms.contains(file->fileName)) {
							modNames.emplace_back(file->fileName);
						}
					}
					if (modNameForms.contains(ffForms)) {
						containsFF = true;
						modNames.emplace_back(TRANSLATE_S(ffForms));
					}
				}

				ResetIndex();
			}
			void ResetIndex()
			{
				curMod = allMods;
				index = 0;
				for (auto& [modName, formData] : modNameForms) {
					formData.ResetIndex();
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
			void Revert()
			{
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
					sky->ResetWeather();
					if (currentWeather) {
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
				ImGuiContext* g = ImGui::GetCurrentContext();

				ImGui::BeginGroup();

				if (!translated) {
					name = TRANSLATE_S(name);
					translated = true;
				}

				ImGui::TextEx(name.c_str(), ImGui::FindRenderedTextEnd(name.c_str()));

				ImGui::PushID(name.c_str());
				ImGui::PushMultiItemsWidths(2, ImGui::GetContentRegionAvail().x);

				if (ImGui::ComboWithFilter("##mods", &index, modNames)) {
					if (index == 0) {
						curMod = allMods;
					} else if (containsFF && index == modNames.size() - 1) {
						curMod = ffForms;
					} else {
						curMod = modNames[index];
					}
					modNameForms[curMod].UpdateValidForms();
					ImGui::SetKeyboardFocusHere();
				}

				ImGui::PopItemWidth();
				ImGui::SameLine(0, g->Style.ItemInnerSpacing.x);

				T* result = modNameForms[curMod].GetComboWithFilterResult();

				ImGui::PopItemWidth();
				ImGui::PopID();

				ImGui::EndGroup();

				return result;
			}

		private:
			// members
			std::string name;
			bool        translated{ false };

			StringMap<Forms<T>>      modNameForms{};
			std::vector<std::string> modNames{};
			std::int32_t             index{};
			bool                     containsFF{ false };

			std::string curMod{ allMods };
		};

		inline FilteredForms<RE::TESEffectShader>       effectShaders{ "$PM_EffectShaders" };
		inline FilteredForms<RE::BGSReferenceEffect>    effectVFX{ "$PM_VisualEffects" };
		inline FilteredForms<RE::TESWeather>            weathers{ "$PM_Weathers" };
		inline FilteredForms<RE::TESIdleForm>           idles{ "$PM_Idles" };
		inline FilteredForms<RE::TESImageSpaceModifier> imods{ "$PM_ImageSpaceModifiers" };

		void InitOverrides();

		void InstallHooks();
	}

	namespace MFG
	{
		inline constexpr std::array expressions{
			"$PM_NONE",
			"$PM_DialogueAnger",
			"$PM_DialogueFear",
			"$PM_DialogueHappy",
			"$PM_DialogueSad",
			"$PM_DialogueSurprise",
			"$PM_DialoguePuzzled",
			"$PM_DialogueDisgusted",
			"$PM_MoodNeutral",
			"$PM_MoodAnger",
			"$PM_MoodFear",
			"$PM_MoodHappy",
			"$PM_MoodSad",
			"$PM_MoodSurprise",
			"$PM_MoodPuzzled",
			"$PM_MoodDisgusted",
			"$PM_CombatAnger",
			"$PM_CombatShout"
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

		inline constexpr std::array gridTypes{
			"$PM_NONE",
			"$PM_Grid_Thirds",
			"$PM_Grid_Diagonal",
			"$PM_Grid_Triangle",
			"$PM_Grid_GoldenRatio",
			//"$PM_Grid_GoldenSpiral",
			"$PM_Grid_Grid"
		};

		inline Type gridType{ kDisabled };

		void Draw();
	}
}
