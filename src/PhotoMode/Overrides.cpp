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

	void Grid::Draw()
	{
		if (gridType == kDisabled) {
			return;
		}

		const auto drawList = ImGui::GetBackgroundDrawList();

		const auto pos = ImGui::GetWindowPos();
		const auto size = ImGui::GetWindowSize();

		constexpr auto topLeft = ImVec2(0.0f, 0.0f);
		const auto static topRight = ImVec2(size.x, 0.0f);
		const auto static bottomLeft = ImVec2(0.0f, size.y);
		const auto static bottomRight = ImVec2(size.x, size.y);

		constexpr auto colour = IM_COL32(255, 255, 255, 85);
		constexpr auto thickness = 2.5f;

		switch (gridType) {
		case kRuleOfThirds:
			{
				const static auto third_width = size.x / 3;
				const static auto third_height = size.y / 3;

				// Draw the horizontal lines
				drawList->AddLine(ImVec2(0, third_height), ImVec2(size.x, third_height), colour, thickness);
				drawList->AddLine(ImVec2(0, third_height * 2), ImVec2(size.x, third_height * 2), colour, thickness);

				// Draw the vertical lines
				drawList->AddLine(ImVec2(third_width, 0), ImVec2(third_width, size.y), colour, thickness);
				drawList->AddLine(ImVec2(third_width * 2, 0), ImVec2(third_width * 2, size.y), colour, thickness);
			}
			break;
		case kDiagonal:
			{
				drawList->AddLine(topLeft, ImVec2(size.x * 0.6666f, size.y), colour, thickness);
				drawList->AddLine(topRight, ImVec2(size.x * 0.3333f, size.y), colour, thickness);

				drawList->AddLine(bottomLeft, ImVec2(size.x * 0.6666f, 0.0f), colour, thickness);
				drawList->AddLine(bottomRight, ImVec2(size.x * 0.3333f, 0.0f), colour, thickness);
			}
			break;
		case kTriangle:
			{
				static bool mirrored = false;
				if (ImGui::IsKeyPressed(ImGuiKey_LeftShift)) {
					mirrored = !mirrored;
				}

				// Draw main diagonal line
				drawList->AddLine(
					!mirrored ? topLeft : bottomLeft,
					!mirrored ? bottomRight : topRight,
					colour, thickness);

				drawList->AddLine(
					!mirrored ? bottomLeft : topLeft,
					!mirrored ? ImVec2(size.x * 0.30f, size.y * 0.30f) : ImVec2(size.x * 0.30f, size.y * 0.70f),
					colour, thickness);

				drawList->AddLine(
					!mirrored ? topRight : bottomRight,
					!mirrored ? ImVec2(size.x * 0.70f, size.y * 0.70f) : ImVec2(size.x * 0.70f, size.y * 0.30f),
					colour, thickness);
			}
			break;
		case kGoldenRatio:
			{
				const static auto width40 = size.x * 0.4f;
				const static auto width60 = size.x * 0.6f;

				const static auto height40 = size.y * 0.4f;
				const static auto height60 = size.y * 0.6f;

				// Draw the horizontal lines
				drawList->AddLine(ImVec2(0, height40), ImVec2(size.x, height40), colour, thickness);
				drawList->AddLine(ImVec2(0, height60), ImVec2(size.x, height60), colour, thickness);

				// Draw the vertical lines
				drawList->AddLine(ImVec2(width40, 0), ImVec2(width40, size.y), colour, thickness);
				drawList->AddLine(ImVec2(width60, 0), ImVec2(width60, size.y), colour, thickness);
			}
			break;
		/*case kGoldenSpiral:
			{
			}
			break;*/
		case kGrid:
			{
				constexpr int   GRID_WIDTH = 8;
				constexpr int   GRID_HEIGHT = 6;
				constexpr float GRID_PADDING = 20.0f;

				const float cellWidth = (size.x - 2 * GRID_PADDING) / GRID_WIDTH;
				const float cellHeight = (size.y - 2 * GRID_PADDING) / GRID_HEIGHT;

				const float startX = pos.x + GRID_PADDING - cellWidth;
				const float endX = pos.x + size.x - GRID_PADDING + cellWidth;

				const float startY = pos.y + GRID_PADDING - cellHeight;
				const float endY = pos.y + size.y - GRID_PADDING + cellHeight;

				// Draw vertical lines
				for (int i = 0; i <= GRID_WIDTH + 1; ++i) {
					const float x = startX + i * cellWidth;
					drawList->AddLine(ImVec2(x, startY), ImVec2(x, endY), colour, thickness);
				}

				// Draw horizontal lines
				for (int j = 0; j <= GRID_HEIGHT + 1; ++j) {
					const float y = startY + j * cellHeight;
					drawList->AddLine(ImVec2(startX, y), ImVec2(endX, y), colour, thickness);
				}
			}
			break;
		default:
			break;
		}
	}
}
