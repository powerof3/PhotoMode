#include "MenuIntegration.h"

namespace MenuIntegration
{
	void Manager::Register()
	{
		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(this);

		if (GetModuleHandle(L"TweenMenuOverhaul") != nullptr) {
			SKSE::GetModCallbackEventSource()->AddEventSink(this);
			logger::info("Registered for mod callback event");
		}
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		photoMode.openFromPause = a_ini.GetBoolValue("Settings", "bOpenFromPauseMenu", photoMode.openFromPause);
		photoGallery.openFromPause = a_ini.GetBoolValue("Settings", "bOpenGalleryFromPauseMenu", photoGallery.openFromPause);
	}

	void Manager::UpdateListVisuals(RE::GFxMovieView* a_view, RE::GFxValue& a_listObj, std::uint32_t a_numItems)
	{
		RE::GFxValue maxShownV;
		if (!a_listObj.GetMember("iMaxItemsShown", &maxShownV) || !maxShownV.IsNumber()) {
			return;
		}
		const auto curClips = static_cast<std::uint32_t>(maxShownV.GetNumber());
		if (curClips == 0 || a_numItems <= curClips) {
			return;
		}
		for (std::uint32_t i = curClips; i < a_numItems; ++i) {
			RE::GFxValue src;
			if (!a_listObj.GetMember(std::format("Entry{}", i - 1).c_str(), &src)) {
				break;
			}
			const auto         newName = std::format("Entry{}", i);
			const RE::GFxValue dupArgs[2] = { RE::GFxValue(newName.c_str()), RE::GFxValue(static_cast<double>(20000 + i)) };
			src.Invoke("duplicateMovieClip", nullptr, dupArgs, 2);

			RE::GFxValue clip;
			if (a_listObj.GetMember(newName.c_str(), &clip)) {
				clip.SetMember("clipIndex", RE::GFxValue(static_cast<double>(i)));
				RE::GFxValue fnPress, fnRoll;
				a_view->CreateFunction(&fnPress, &entryPressHandler);
				a_view->CreateFunction(&fnRoll, &entryRollOverHandler);
				clip.SetMember("onPress", fnPress);
				clip.SetMember("onRollOver", fnRoll);
			}
		}
		a_listObj.SetMember("iMaxItemsShown", RE::GFxValue(static_cast<double>(a_numItems)));
	}

	void Manager::SetupJournalMenu()
	{
		const auto menu = RE::UI::GetSingleton()->GetMenu<RE::JournalMenu>();
		const auto view = menu ? menu->systemTab.view : nullptr;

		RE::GFxValue page;
		if (!view || !view->GetVariable(&page, "_root.QuestJournalFader.Menu_mc.SystemFader.Page_mc")) {
			return;
		}

		if (skyUI6Installed = page.HasMember("UpdateIndices"); skyUI6Installed) {
			if (photoMode.openFromPause) {
				photoMode.pressHandler.InjectJournalEntry(view.get(), page);
			}
			if (photoGallery.openFromPause) {
				photoGallery.pressHandler.InjectJournalEntry(view.get(), page);
			}
			return;
		}

		if (!photoMode.openFromPause) {
			return;
		}

		photoModeInjected = false;

		// in case someone packed the files into a BSA
		static bool dearDiaryExists = RE::BSResourceNiBinaryStream(R"(interface\deardiary_dm\config.txt)").good() || RE::BSResourceNiBinaryStream(R"(interface\deardiary\config.txt)").good();

		// Dear Diary SetShowMod function is broken af, need to do it manually
		if (dearDiaryExists) {
			RE::GFxValue categoryList;
			if (page.GetMember("CategoryList", &categoryList)) {
				RE::GFxValue entryList;
				if (categoryList.GetMember("entryList", &entryList)) {
					std::vector<std::string> elements;

					entryList.VisitMembers([&](const char*, const RE::GFxValue& a_value) {
						RE::GFxValue textVal;
						a_value.GetMember("text", &textVal);
						elements.push_back(textVal.GetString());
					});

					RE::GFxValue showModMenu;
					if (page.GetMember("_showModMenu", &showModMenu) && !showModMenu.GetBool()) {
						page.SetMember("_showModMenu", true);
					} else {
						std::erase(elements, "$MOD MANAGER");
					}

					auto index = std::ranges::contains(elements, "$QUICKSAVE") ? 3 : 2;
					elements.insert(elements.begin() + index, "$PM_Title_Menu");

					entryList.ClearElements();
					for (auto& element : elements) {
						RE::GFxValue entry;
						view->CreateObject(&entry);
						entry.SetMember("text", element.c_str());
						entryList.PushBack(entry);
					}

					categoryList.Invoke("InvalidateData");

					photoModeInjected = true;
				}
			}

		} else {
			RE::GFxValue showModMenu;
			if (page.GetMember("_showModMenu", &showModMenu) && !showModMenu.GetBool()) {
				std::array<RE::GFxValue, 1> args;
				args[0] = true;
				if (!page.Invoke("SetShowMod", nullptr, args.data(), args.size())) {
					return;
				}
			}

			RE::GFxValue categoryList;
			if (page.GetMember("CategoryList", &categoryList)) {
				RE::GFxValue entryList;
				if (categoryList.GetMember("entryList", &entryList)) {
					std::optional<std::uint32_t> modMenuIndex = std::nullopt;

					std::uint32_t index = 0;
					std::string   text;
					entryList.VisitMembers([&](const char*, const RE::GFxValue& a_value) {
						RE::GFxValue textVal;
						a_value.GetMember("text", &textVal);
						text = textVal.GetString();
						if (text == "$MOD MANAGER") {
							modMenuIndex = index;
						}
						index++;
					});

					if (modMenuIndex) {
						RE::GFxValue entry;
						view->CreateObject(&entry);
						entry.SetMember("text", "$PM_Title_Menu");

						entryList.SetElement(*modMenuIndex, entry);
						categoryList.Invoke("InvalidateData");

						photoModeInjected = true;
						return;
					}
				}
			}
		}
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		if (a_evn->menuName == RE::JournalMenu::MENU_NAME) {
			if (a_evn->opening) {
				if (photoMode.openFromPause || photoGallery.openFromPause) {
					SetupJournalMenu();
				}
			}
		} else if (a_evn->menuName == RE::ModManagerMenu::MENU_NAME) {
			if (a_evn->opening) {
				if (RE::UI::GetSingleton()->IsMenuOpen(RE::JournalMenu::MENU_NAME) && !skyUI6Installed && photoModeInjected) {
					const auto msgQueue = RE::UIMessageQueue::GetSingleton();

					msgQueue->AddMessage(RE::ModManagerMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
					msgQueue->AddMessage(RE::JournalMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

					MANAGER(PhotoMode)->Activate();
				}
			}
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(const SKSE::ModCallbackEvent* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		photoMode.SetTweenMenuOpen(a_evn->eventName);
		photoGallery.SetTweenMenuOpen(a_evn->eventName);

		return EventResult::kContinue;
	}
}
