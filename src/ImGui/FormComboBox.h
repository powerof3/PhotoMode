#pragma once

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
				const auto it = edidForms.find(EditorID::GetEditorID(RE::Sky::GetSingleton()->currentWeather));
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

		void GetFormResultFromCombo(std::function<void(T*)> a_func)
		{
			T* formResult;

			if (!translated) {
				name = TRANSLATE_S(name);
				translated = true;
			}

			ImGui::BeginGroup();
			{
				ImGui::SeparatorText(name.c_str());

				ImGui::PushStyleColor(ImGuiCol_NavHighlight, IM_COL32(255, 255, 255, 204));
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
					modNameForms[curMod].UpdateValidForms();
					ImGui::SetKeyboardFocusHere();
				}

				ImGui::PopItemWidth();
				ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

				formResult = modNameForms[curMod].GetComboWithFilterResult();

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
