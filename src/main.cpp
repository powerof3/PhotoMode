#include "Console.h"
#include "ENB/ENB.h"
#include "Hooks.h"
#include "ImGui/Renderer.h"
#include "Input.h"
#include "Papyrus.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/LoadScreen.h"
#include "Screenshots/Manager.h"
#include "Settings.h"
#include "Translation.h"

void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			logger::info("{:*^30}", "POST LOAD");

			Hooks::Install();

			/*ENB::handle = static_cast<ENB_API::ENBSDKALT1001*>(RequestENBAPI(ENB_API::SDKVersion::V1001));
			if (ENB::handle) {
				ENB::handle->SetCallbackFunction([](ENBCallbackType a_calltype) {
					switch (a_calltype) {
					case ENBCallbackType::ENBCallback_BeginFrame:
						MANAGER(PhotoMode)->UpdateENBParams();
						break;
					case ENBCallbackType::ENBCallback_EndFrame:
						MANAGER(PhotoMode)->RevertENBParams();
						break;
					default:
						break;
					}
				});
				logger::info("Obtained ENB API");
			} else {
				logger::info("Unable to acquire ENB API");
			}*/
		}
		break;
	case SKSE::MessagingInterface::kInputLoaded:
		{
			logger::info("{:*^30}", "INPUT LOADED");

			MANAGER(Input)->Register();
			MANAGER(PhotoMode)->Register();
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			logger::info("{:*^30}", "DATA LOADED");

			MANAGER(Translation)->BuildTranslationMap();

			MANAGER(LoadScreen)->InitLoadScreenObjects();
			MANAGER(Screenshot)->LoadScreenshots();
			MANAGER(PhotoMode)->OnDataLoad();

			Console::Install();
		}
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("PhotoMode");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "PhotoMode";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);

	InitializeLog();

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

	SKSE::AllocTrampoline(98);

	Settings::GetSingleton()->Load(FileType::kDisplayTweaks, [](auto& ini) {
		ImGui::Renderer::LoadSettings(ini);  // display tweaks scaling
	});
	Settings::GetSingleton()->LoadMCMSettings();

	ImGui::Renderer::Install();

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", OnInit);

	SKSE::GetPapyrusInterface()->Register(Papyrus::Register);

	return true;
}
