#pragma once

namespace Cache
{
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

namespace DOF = Cache::DOF;
namespace FreeCamera = Cache::FreeCamera;
