#pragma once

#include "ImGui/FormComboBox.h"

namespace PhotoMode
{
	inline RE::TESIdleForm* resetRootIdle{ nullptr };

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
			"$PM_Aah",
			"$PM_BigAah",
			"$PM_BMP",
			"$PM_ChJSh",
			"$PM_DST",
			"$PM_Eee",
			"$PM_Eh",
			"$PM_FV",
			"$PM_I",
			"$PM_K",
			"$PM_N",
			"$PM_Oh",
			"$PM_OohQ",
			"$PM_R",
			"$PM_Th",
			"$PM_W"
		};
		inline constexpr std::array modifiers{
			"$PM_BlinkLeft",
			"$PM_BlinkRight",
			"$PM_BrowDownLeft",
			"$PM_BrowDownRight",
			"$PM_BrowInLeft",
			"$PM_BrowInRight",
			"$PM_BrowUpLeft",
			"$PM_BrowUpRight",
			"$PM_LookDown",
			"$PM_LookLeft",
			"$PM_LookRight",
			"$PM_LookUp",
			"$PM_SquintLeft",
			"$PM_SquintRight",
			"$PM_HeadPitch",
			"$PM_HeadRoll",
			"$PM_HeadYaw"
		};

		class Data
		{
		public:
			void Revert(RE::Actor* a_actor);

			// members
			struct Expression
			{
				void ApplyExpression(RE::Actor* a_actor) const;

				std::int32_t modifier{ 0 };  // 0 is NONE
				std::int32_t strength{ 0 };
			};
			struct Modifier
			{
				void ApplyPhenome(std::uint32_t idx, RE::Actor* a_actor) const;
				void ApplyModifier(std::uint32_t idx, RE::Actor* a_actor) const;

				std::int32_t strength{ 0 };
			};

			Expression expressionData;
			Modifier   phonemeData[phonemes.size()];
			Modifier   modifierData[modifiers.size()];
		};
	}

	class Character
	{
	public:
		Character() = default;
		Character(RE::Actor* a_actor);

		void GetOriginalState();
		void RevertState();

		const std::string& GetName();

		void Draw(bool a_resetTabs);

	private:
		struct State
		{
			void Get(const RE::Actor* a_actor);

			// members
			bool         visible{ true };
			float        rotZ{};
			RE::NiPoint3 pos{};
		};

		void RevertIdle() const;

		// members
		RE::Actor* character{ nullptr };
		std::string characterName{};

		// names should ideally be pulled from a shared map with different indices for characters but this will do
		ImGui::FormComboBoxFiltered<RE::TESEffectShader>    effectShaders{ "$PM_EffectShaders" };
		ImGui::FormComboBoxFiltered<RE::TESIdleForm>        idles{ "$PM_Idles" };
		ImGui::FormComboBoxFiltered<RE::BGSReferenceEffect> effectVFX{ "$PM_VisualEffects" };

		MFG::Data mfgData{};

		State originalState{};
		State currentState{};

		bool rotationChanged{ false };
		bool positionChanged{ false };

		bool effectsPlayed{ false };
		bool vfxPlayed{ false };
		bool idlePlayed{ false };
	};

	void InstallHook();
}
