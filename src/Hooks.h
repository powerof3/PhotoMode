#pragma once

namespace PhotoMode
{
	inline StringMap<RE::TESIdleForm*> cachedIdles;

	void InstallHooks();
}

namespace Screenshot
{
	void InstallHooks();
}

namespace LoadScreen
{
	void InstallHooks();
}

namespace Hooks
{
	void Install();
}
