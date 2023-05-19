#pragma once

namespace PhotoMode::Renderer
{
	inline std::atomic initialized{ false };
	inline ImFont*     selectedFont{ nullptr };

	void InstallHook();
}
