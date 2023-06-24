#include "Player.h"

#include "Translation.h"

namespace PhotoMode
{
	namespace MFG
	{
		void Expression::ApplyExpression(RE::Actor* a_actor) const
		{
			if (const auto faceData = a_actor->GetFaceGenAnimationData()) {
				if (modifier == 0) {
					faceData->ClearExpressionOverride();
					faceData->Reset(0.0f, true, false, false, false);
					RE::BSFaceGenManager::GetSingleton()->isReset = true;
				} else {
					faceData->exprOverride = false;
					faceData->SetExpressionOverride(modifier - 1, strength);
					faceData->exprOverride = true;
				}
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

			for (std::uint32_t i = 0; i < phonemes.size(); i++) {
				phonemeData[i].strength = 0;
			}
			for (std::uint32_t i = 0; i < modifiers.size(); i++) {
				modifierData[i].strength = 0;
			}
		}
	}

	void Player::State::Get()
	{
		const auto playerRef = RE::PlayerCharacter::GetSingleton();

		visible = playerRef->Get3D() ? !playerRef->Get3D()->GetAppCulled() : false;

		rotZ = playerRef->GetAngleZ();
		pos = playerRef->GetPosition();
	}

	void Player::State::Set() const
	{
		const auto playerRef = RE::PlayerCharacter::GetSingleton();

		//culling is handled by manager

		playerRef->SetRotationZ(rotZ);
		playerRef->SetPosition(pos, true);
		playerRef->UpdateActor3DPosition();
	}

	void Player::RevertIdle() const
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (const auto currentProcess = player->currentProcess) {
			currentProcess->StopCurrentIdle(player, true);
			currentProcess->PlayIdle(player, resetRootIdle, nullptr);
		}
	}

	void Player::GetOriginalState()
	{
		originalState.Get();

		effectShaders.InitForms();
		effectVFX.InitForms();

		if (!resetRootIdle) {
			resetRootIdle = RE::TESForm::LookupByEditorID<RE::TESIdleForm>("ResetRoot");
		}

		idles.InitForms();
	}

	void Player::RevertState()
	{
		originalState.Set();

		// revert current values
		currentState.pos = RE::NiPoint3();
		if (!currentState.visible) {
			if (const auto root = RE::PlayerCharacter::GetSingleton()->Get3D()) {
				root->CullGeometry(false);
			}
			currentState.visible = true;
		}

		// reset expressions
		MFG::RevertAllModifiers();

		// revert idles
		idles.ResetIndex();
		if (idlePlayed) {
			RevertIdle();
			idlePlayed = false;
		}

		// revert effects
		effectShaders.ResetIndex();
		effectVFX.ResetIndex();

		if (vfxPlayed || effectsPlayed) {
			const auto player = RE::PlayerCharacter::GetSingleton();
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
			vfxPlayed = false;
			effectsPlayed = false;
		}
	}

	void Player::Draw()
	{
		static auto player = RE::PlayerCharacter::GetSingleton();

		if (ImGui::CheckBox("$PM_ShowPlayer"_T, &currentState.visible)) {
			player->Get3D()->CullGeometry(!currentState.visible);
		}

		ImGui::Spacing();

		ImGui::BeginDisabled(!currentState.visible);
		{
			if (ImGui::BeginTabBarCustom("Player#TopBar", 0)) {
				// ugly af, improve later
				const float width = ImGui::GetContentRegionAvail().x / 4;

			    ImGui::SetNextItemWidth(width);
				if (ImGui::OpenTabOnHover("$PM_Expressions"_T)) {
					using namespace MFG;

					if (ImGui::EnumSlider("$PM_Expression"_T, &expressionData.modifier, expressions)) {
						expressionData.ApplyExpression(player);
					}

					ImGui::Spacing();

					if (ImGui::TreeNode("$PM_Phoneme"_T)) {
						for (std::uint32_t i = 0; i < phonemes.size(); i++) {
							if (ImGui::DragOnHover(phonemes[i], &phonemeData[i].strength)) {
								phonemeData[i].ApplyPhenome(i, player);
							}
						}
						ImGui::TreePop();
					}

					if (ImGui::TreeNode("$PM_Modifier"_T)) {
						for (std::uint32_t i = 0; i < modifiers.size(); i++) {
							if (ImGui::DragOnHover(modifiers[i], &modifierData[i].strength)) {
								modifierData[i].ApplyModifier(i, player);
							}
						}
						ImGui::TreePop();
					}
					ImGui::EndTabItem();
				}

			    ImGui::SetNextItemWidth(width);
				if (ImGui::OpenTabOnHover("$PM_Poses"_T)) {
					idles.GetFormResultFromCombo([&](const auto& a_idle) {
						if (idlePlayed) {
							if (const auto currentProcess = player->currentProcess) {
								currentProcess->StopCurrentIdle(player, true);
								currentProcess->PlayIdle(player, resetRootIdle, nullptr);
							}
						}
						if (const auto currentProcess = player->currentProcess) {
							currentProcess->PlayIdle(player, a_idle, nullptr);
							idlePlayed = true;
						}
					});
					ImGui::EndTabItem();
				}

			    ImGui::SetNextItemWidth(width);
				if (ImGui::OpenTabOnHover("$PM_Effects"_T)) {
					effectShaders.GetFormResultFromCombo([&](const auto& a_effectShader) {
						player->ApplyEffectShader(a_effectShader);
						effectsPlayed = true;
					});
					effectVFX.GetFormResultFromCombo([&](const auto& a_vfx) {
						if (const auto effectShader = a_vfx->data.effectShader) {
							player->ApplyEffectShader(effectShader, -1, nullptr, a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kFaceTarget), a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kAttachToCamera));
						}
						if (const auto artObject = a_vfx->data.artObject) {
							player->ApplyArtObject(artObject, -1, nullptr, a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kFaceTarget), a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kAttachToCamera));
						}
						vfxPlayed = true;
					});
					ImGui::EndTabItem();
				}

			    ImGui::SetNextItemWidth(width);
				if (ImGui::OpenTabOnHover("$PM_Transforms"_T)) {
					currentState.rotZ = RE::rad_to_deg(player->GetAngleZ());
					if (ImGui::Slider("$PM_Rotation"_T, &currentState.rotZ, 0.0f, 360.0f)) {
						player->SetRotationZ(RE::deg_to_rad(currentState.rotZ));
					}

					bool update = ImGui::Slider("$PM_PositionLeftRight"_T, &currentState.pos.x, -100.0f, 100.0f);
					update |= ImGui::Slider("$PM_PositionNearFar"_T, &currentState.pos.y, -100.0f, 100.0f);
					// update |= ImGui::Slider("Elevation", &currentState.pos.z, -100.0f, 100.0f);

					if (update) {
						player->SetPosition({ originalState.pos + currentState.pos }, true);
					}

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::EndDisabled();
	}
}
