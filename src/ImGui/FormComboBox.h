#pragma once

#include "Hooks.h"
#include "ImGui/Widgets.h"

namespace ImGui
{
	constexpr auto allMods = "$PM_ALL"sv;
	constexpr auto ffForms = "$PM_FF_Forms"sv;

	template <class T>
	class FormComboBox
	{
	public:
		void AddForm(const std::string& a_edid, T* a_form)
		{
			if (edidForms.emplace(a_edid, a_form).second) {
				edids.push_back(a_edid);
			}
		}
		void UpdateValidForms(RE::Actor* a_actor = nullptr)
		{
			if (valid) {
				return;
			}

			SetValid(true);

			if constexpr (std::is_same_v<T, RE::TESIdleForm>) {
				if (!a_actor) {
					a_actor = RE::PlayerCharacter::GetSingleton();
				}
				edids.clear();
				for (auto& [edid, idle] : edidForms) {
					if (a_actor->CanUseIdle(idle) && idle->CheckConditions(a_actor, nullptr, false)) {
						edids.push_back(edid);
					}
				}
			}
		}
		void ResetIndex()
		{
			index = 0;

			if constexpr (std::is_same_v<T, RE::TESWeather>) {
				if (auto currentWeather = RE::Sky::GetSingleton()->currentWeather) {
					if (const auto it = edidForms.find(editorID::get_editorID(currentWeather)); it != edidForms.end()) {
						index = static_cast<std::int32_t>(std::distance(edidForms.begin(), it));
					}
				}
			}
		}
		void SetValid(bool a_valid)
		{
			valid = a_valid;
		}

		T* GetComboWithFilterResult(RE::Actor* a_actor = nullptr)
		{
			UpdateValidForms(a_actor);
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
		bool                     valid{ false };
	};

	// modName, forms
	template <class T>
	class FormComboBoxFiltered
	{
	public:
		FormComboBoxFiltered(std::string a_name) :
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
			if (modNameForms.empty()) {
				if constexpr (!std::is_same_v<T, RE::TESIdleForm>) {
					for (const auto& form : RE::TESDataHandler::GetSingleton()->GetFormArray<T>()) {
						if (form) {
							AddForm(editorID::get_editorID(form), form);
						}
					}
				} else {
					for (auto& [edid, form] : PhotoMode::cachedIdles) {
						AddForm(edid, form);
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

			Reset();
		}
		void Reset()
		{
			curMod = allMods;
			index = 0;
			for (auto& [modName, formData] : modNameForms) {
				formData.ResetIndex();
				formData.SetValid(false);
			}
		}

		void GetFormResultFromCombo(std::function<void(T*)> a_func, RE::Actor* a_actor = nullptr)
		{
			T* formResult;

			if (!translated) {
				name = TRANSLATE_S(name);
				translated = true;
			}

			ImGui::BeginGroup();
			{
				ImGui::SeparatorText(name.c_str());

				ImGui::PushStyleColor(ImGuiCol_NavCursor, GetUserStyleColorVec4(USER_STYLE::kComboBoxText));
				ImGui::PushStyleColor(ImGuiCol_Header, GetColorU32(ImGuiCol_TextDisabled));

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
				}

				ImGui::PopItemWidth();
				ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

				formResult = modNameForms[curMod].GetComboWithFilterResult(a_actor);

				ImGui::PopItemWidth();
				ImGui::PopID();
				ImGui::PopStyleColor(2);
			}
			ImGui::EndGroup();

			if (formResult) {
				a_func(formResult);
			}
		}

	private:
		// members
		std::string name;
		bool        translated{ false };

		StringMap<FormComboBox<T>> modNameForms{};
		std::vector<std::string>   modNames{};
		std::int32_t               index{};
		bool                       containsFF{ false };

		std::string curMod{ allMods };
	};
}
