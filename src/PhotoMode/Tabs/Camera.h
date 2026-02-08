#pragma once

#include "CameraPositions.h"

namespace PhotoMode
{
	class Camera
	{
	public:
		void GetOriginalState();
		void RevertState(bool a_deactivate);

		[[nodiscard]] float GetViewRoll() const { return currentViewRoll; }
		void                SetViewRoll(float a_roll) { currentViewRoll = a_roll; }

		void Draw();

	private:
		struct OriginalState
		{
			void Get();
			void Revert(bool a_deactivate) const;

			// members
			float fov{};
			float translateSpeed{};

			struct
			{
				float blurMultiplier;
				float nearDist;
				float nearRange;
				float farDist;
				float farRange;
			} vanillaDOF;
		};

		// members
		OriginalState originalState{};

		bool revertENB{ false };

		float currentViewRoll{};
		float currentViewRollDegrees{};

		// Camera position management
		CameraPositions cameraPositions{};
	};

	namespace CameraGrid
	{
		enum GridType : std::uint8_t
		{
			kDisabled,
			kRuleOfThirds,
			kDiagonal,
			kTriangle,
			kGoldenRatio,
			//kGoldenSpiral,
			kGrid
		};

		static constexpr std::array gridTypes{
			"$PM_NONE",
			"$PM_Grid_Thirds",
			"$PM_Grid_Diagonal",
			"$PM_Grid_Triangle",
			"$PM_Grid_GoldenRatio",
			/*"$PM_Grid_GoldenSpiral"*/
			"$PM_Grid_Grid"
		};

		void Draw();

		// members
		inline GridType gridType{ kDisabled };
	}
}
