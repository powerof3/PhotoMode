#include "Overrides.h"

namespace PhotoMode
{
	namespace Override	
	{
        void GetValidOverrides()
		{
			effectShaders.GetValidForms();
			effectVFX.GetValidForms();
			weathers.GetValidForms();
			idles.GetValidForms();
			imods.GetValidForms();
		}

        void RevertOverrides(bool& a_vfx, bool& a_weather, bool& a_idle)
        {
            if (a_vfx) {
				effectShaders.Revert();
				effectVFX.Revert();
				a_vfx = false;
			}
			if (a_weather) {
				weathers.Revert();
				a_weather = false;
			}
			if (a_idle) {
				idles.Revert();
				a_idle = false;
			}
			imods.Revert();
        }

		// TESDataHandler idle array is not populated
		struct SetFormEditorID
		{
			static bool thunk(RE::TESIdleForm* a_this, const char* a_str)
			{
				if (!string::is_empty(a_str)) {
					idles.forms.emplace(a_str, a_this);
				}
				return func(a_this, a_str);
			}
			static inline REL::Relocation<decltype(thunk)> func;
			static inline constexpr std::size_t            size{ 0x33 };
		};

		void InstallHooks()
		{
			stl::write_vfunc<RE::TESIdleForm, SetFormEditorID>();
		}
	}

	namespace MFG
	{
		void Expression::ApplyExpression(RE::Actor* a_actor) const
		{
			if (const auto faceData = a_actor->GetFaceGenAnimationData()) {
				faceData->exprOverride = false;
				faceData->SetExpressionOverride(modifier, strength);
				faceData->exprOverride = true;
			}
		}

        void Modifier::ApplyModifier(std::uint32_t idx, RE::Actor* a_actor) const
		{
			if (const auto faceData = a_actor->GetFaceGenAnimationData()) {
				RE::BSSpinLockGuard locker(faceData->lock);
				faceData->modifierKeyFrame.SetValue(idx, static_cast<float>(strength / 100.0f));
			}
		}

        void Modifier::ApplyPhenome(std::uint32_t idx, RE::Actor* a_actor) const
		{
			if (const auto faceData = a_actor->GetFaceGenAnimationData()) {
				RE::BSSpinLockGuard locker(faceData->lock);
				faceData->phenomeKeyFrame.SetValue(idx, static_cast<float>(strength / 100.0f));
			}
		}

        void RevertAllModifiers()
		{
			if (const auto faceData = RE::PlayerCharacter::GetSingleton()->GetFaceGenAnimationData()) {
				faceData->ClearExpressionOverride();
				faceData->Reset(0.0f, true, true, true, false);
				RE::BSFaceGenManager::GetSingleton()->isReset = true;
			}

		    expressionData.modifier = 0;
			expressionData.strength = 0;

		    for (std::uint32_t i = 0; i < phonemes.size(); i++) {
				phonemeData[i].strength = 0;
			}
			for (std::uint32_t i = 0; i < modifiers.size(); i++) {
				modifierData[i].strength = 0;
			}
		}
    }
}
