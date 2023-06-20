#pragma once

namespace PhotoMode
{
	class Camera
	{
	public:
		void GetOriginalState();
		void RevertState(bool a_deactivate);

		[[nodiscard]] float GetViewRoll() const { return currentViewRoll; }

		void Draw();

		void UpdateENBParams();
		void RevertENBParams();

	private:
		// very, very basic support
		struct ENBDOF
		{
			enum TYPE : std::uint32_t
			{
				kNone,
				kEnable = 1 << 0,
				kAll
			};

			void Get();
			void SetParameter(std::uint32_t a_type) const;

			// members
			bool enabled;
		};

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

			ENBDOF enbDOF{};
		};

		// members
		OriginalState originalState{};

		ENBDOF curDOF{};
		ENBDOF lastDOF{};
		bool   revertENB{ false };

		float currentViewRoll{};
	};

	namespace CameraGrid
	{
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
