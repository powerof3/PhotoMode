#pragma once

#include "ImGui/FormComboBox.h"

namespace PhotoMode
{
	class Filters
	{
	public:
		void GetOriginalState();
		void RevertState(bool a_fullReset);

		void Draw();

	private:
		// members
		RE::ImageSpaceBaseData                                 imageSpaceData{};
		RE::TESImageSpaceModifier*                             currentImod{ nullptr };
		bool                                                   imodPlayed{ false };
		ImGui::FormComboBoxFiltered<RE::TESImageSpaceModifier> imods{ "$PM_ImageSpaceModifiers" };
	};
}
