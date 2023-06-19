#pragma once

namespace PhotoMode
{
	class Camera
	{
	public:
	    void GetOriginalState();
		void RevertState();

		[[nodiscard]] float GetViewRoll() const { return currentViewRoll; }

		void Draw();
		void DrawGrid() const;

	private:
		enum GridType
		{
			kDisabled,
			kRuleOfThirds,
			kDiagonal,
			kTriangle,
			kGoldenRatio,
			//kGoldenSpiral,
			kGrid
		};

		struct OriginalState
		{
			void Get();
			void Revert() const;

			// members
			float fov{};
			float translateSpeed{};

			float blurMultiplier;
			float nearDist;
			float nearRange;
			float farDist;
			float farRange;
		};

		// members
		OriginalState originalState{};
		float         currentViewRoll{};

		static constexpr std::array gridTypes{ "$PM_NONE", "$PM_Grid_Thirds", "$PM_Grid_Diagonal", "$PM_Grid_Triangle", "$PM_Grid_GoldenRatio" /*,"$PM_Grid_GoldenSpiral"*/, "$PM_Grid_Grid" };
		GridType                    gridType{ kDisabled };
	};
}
