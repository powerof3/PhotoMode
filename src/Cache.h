#pragma once

namespace Cache
{
	using _GetFormEditorID = const char* (*)(std::uint32_t);

	namespace EditorID
	{
		inline std::string GetEditorID(RE::TESForm* a_form)
		{
			if (!a_form) {
				return {};
			}
			switch (a_form->GetFormType()) {
			case RE::FormType::Keyword:
			case RE::FormType::LocationRefType:
			case RE::FormType::Action:
			case RE::FormType::MenuIcon:
			case RE::FormType::Global:
			case RE::FormType::HeadPart:
			case RE::FormType::Race:
			case RE::FormType::Sound:
			case RE::FormType::Script:
			case RE::FormType::Navigation:
			case RE::FormType::Cell:
			case RE::FormType::WorldSpace:
			case RE::FormType::Land:
			case RE::FormType::NavMesh:
			case RE::FormType::Dialogue:
			case RE::FormType::Quest:
			case RE::FormType::Idle:
			case RE::FormType::AnimatedObject:
			case RE::FormType::ImageAdapter:
			case RE::FormType::VoiceType:
			case RE::FormType::Ragdoll:
			case RE::FormType::DefaultObject:
			case RE::FormType::MusicType:
			case RE::FormType::StoryManagerBranchNode:
			case RE::FormType::StoryManagerQuestNode:
			case RE::FormType::StoryManagerEventNode:
			case RE::FormType::SoundRecord:
				return a_form->GetFormEditorID();
			default:
				static auto function = reinterpret_cast<_GetFormEditorID>(GetProcAddress(GetModuleHandle(L"po3_Tweaks"), "GetFormEditorID"));
				if (function) {
					return function(a_form->GetFormID());
				}
				return {};
			}
		}
	}

	namespace FreeCamera
	{
		inline static float& translateSpeed{
			*REL::Relocation<float*>{ RELOCATION_ID(509808, 382522) }
		};
	}

	namespace DOF
	{
		inline static bool& dynamicToggle{
			*REL::Relocation<bool*>{ RELOCATION_ID(517709, 404236) }
		};

		inline static float& nearDist{
			*REL::Relocation<float*>{ RELOCATION_ID(528191, 415136) }
		};
		inline static float& farDist{
			*REL::Relocation<float*>{ RELOCATION_ID(528192, 415137) }
		};
		inline static float& nearRange{
			*REL::Relocation<float*>{ RELOCATION_ID(528193, 415138) }
		};
		inline static float& farRange{
			*REL::Relocation<float*>{ RELOCATION_ID(528194, 415139) }
		};
		inline static float& nearBlur{
			*REL::Relocation<float*>{ RELOCATION_ID(528195, 415140) }
		};
		inline static float& farBlur{
			*REL::Relocation<float*>{ RELOCATION_ID(528196, 415141) }
		};
		inline static float& blurMultiplier{
			*REL::Relocation<float*>{ RELOCATION_ID(528197, 415142) }
		};

		inline static bool& centerWeightToggle{
			*REL::Relocation<bool*>{ RELOCATION_ID(528124, 415069) }
		};
		inline static float& centerWeight{
			*REL::Relocation<float*>{ RELOCATION_ID(528120, 415065) }
		};
		inline static float& maxDepth{
			*REL::Relocation<float*>{ RELOCATION_ID(528121, 415066) }
		};
	}
}
namespace EditorID = Cache::EditorID;
namespace DOF = Cache::DOF;
namespace FreeCamera = Cache::FreeCamera;
