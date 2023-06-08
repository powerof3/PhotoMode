#pragma once

#include "ImGui/FormComboBox.h"

namespace PhotoMode
{
	inline ImGui::FormComboBoxFiltered<RE::TESImageSpaceModifier> imods{ "$PM_ImageSpaceModifiers" };

	class Filters
	{
	public:
		void GetOriginalState();
		void RevertState(bool a_fullReset);

		void Draw();

	private:
		// members
		RE::ImageSpaceBaseData imageSpaceData{};

		RE::TESImageSpaceModifier* currentImod{ nullptr };
		bool                       imodPlayed{ false };
	};
}
