#include "Console.h"
#include "Hooks.h"
#include "ImGui/Renderer.h"
#include "Input.h"
#include "MenuIntegration.h"
#include "Papyrus.h"
#include "PhotoMode/Favorites.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/LoadScreen.h"
#include "Screenshots/Manager.h"
#include "Settings.h"
#include "Translation.h"
#include "UIAPI/Manager.h"

void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			REX::INFO("{:*^30}", "POST LOAD");

			Hooks::Install();
		}
		break;
	case SKSE::MessagingInterface::kInputLoaded:
		{
			REX::INFO("{:*^30}", "INPUT LOADED");

			MANAGER(Input)->Register();
			MANAGER(MenuIntegration)->Register();
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			REX::INFO("{:*^30}", "DATA LOADED");

			MANAGER(Translation)->BuildTranslationMap();

			MANAGER(LoadScreen)->InitLoadScreenObjects();
			MANAGER(Screenshot)->LoadScreenshots();

			MANAGER(PhotoMode)->OnDataLoad();
			MANAGER(Gallery)->OnDataLoad();

			MANAGER(Favorites)->LoadFavorites();

			Console::Install();
		}
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_SUPPORT_AE
SKSE_PLUGIN_VERSION = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(REL::Version{ Version::MAJOR, Version::MINOR, Version::PATCH });
	v.PluginName("PhotoMode");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });

	if constexpr (SKSE::RUNTIME_SSE_LATEST < Runtime::MIN_ADDRESS_LIBRARY_V5) {
		v.MinimumRequiredXSEVersion(REL::Version{ 2, 2, 5 });
	} else {
		v.MinimumRequiredXSEVersion(REL::Version{ 2, 3, 0 });
	}

	return v;
}();
#else
SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "PhotoMode";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		REX::CRITICAL("Loaded in editor, marking as incompatible");
		return false;
	}

	if (const auto ver = a_skse->RuntimeVersion(); ver < SKSE::RUNTIME_SSE_1_5_39) {
		REX::CRITICAL("Unsupported runtime version {}", ver);
		return false;
	}

	return true;
}
#endif

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, SKSE::InitInfo{
						   .log = true,
						   .logName = Version::PROJECT.data(),
						   .trampoline = true,
						   .trampolineSize = 150,
					   });

	auto runtimeVersion = a_skse->RuntimeVersion();

	REX::INFO("Game version : {}", runtimeVersion);

#ifdef SKYRIM_SUPPORT_AE
	if constexpr (SKSE::RUNTIME_SSE_LATEST < Runtime::MIN_ADDRESS_LIBRARY_V5) {
		if (runtimeVersion >= Runtime::MIN_ADDRESS_LIBRARY_V5) {
			REX::FAIL(
				"You are using a newer version of Skyrim than this version of {0} supports.\n"
				"Install the correct version of {0} for your game version.\n"
				"Runtime: {1}\n"
				"Supported: 1.6.1170 (Steam) / 1.6.1179 (GOG)",
				Version::PROJECT, runtimeVersion);
		}
	}
#endif

	Settings::GetSingleton()->Load(FileType::kDisplayTweaks, [](auto& ini) {
		ImGui::Renderer::LoadSettings(ini);  // display tweaks scaling
	});
	Settings::GetSingleton()->LoadMCMSettings();

	ImGui::Renderer::Install();

	SKSE::GetMessagingInterface()->RegisterListener("SKSE", OnInit);
	SKSE::GetPapyrusInterface()->Register(Papyrus::Register);

	return true;
}

extern "C" DLLEXPORT std::uint8_t IsPhotoModeActive()
{
	return MANAGER(PhotoMode)->IsActive();
}

extern "C" DLLEXPORT std::uint32_t IsPhotoGalleryActive()
{
	return MANAGER(Gallery)->IsActive();
}

extern "C" DLLEXPORT PhotoModeAPI::IPhotoMode* RequestPhotoModeUIAPI(PhotoModeAPI::InterfaceVersion a_version)
{
	return a_version == PhotoModeAPI::InterfaceVersion::kV1 ? MANAGER(UIAPI) : nullptr;
}
