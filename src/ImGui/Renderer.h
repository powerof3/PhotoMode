#pragma once

namespace PhotoMode::Renderer
{
	inline std::atomic initialized{ false };

	void InstallHook();
}
