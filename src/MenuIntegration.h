#pragma once

#include "Gallery/Manager.h"
#include "PhotoMode/Manager.h"

namespace MenuIntegration
{
	// adapted from https://github.com/Fuzzlesz/FUCK/blob/main/src/System/Journal.cpp
	class EntryPressHandler : public RE::GFxFunctionHandler
	{
	public:
		void Call(Params& a_params) override
		{
			if (!a_params.thisPtr) {
				return;
			}
			RE::GFxValue itemIndex, parent;
			if (!a_params.thisPtr->GetMember("itemIndex", &itemIndex) || itemIndex.IsUndefined() || !a_params.thisPtr->GetMember("_parent", &parent)) {
				return;
			}
			RE::GFxValue kbOrMouse{ 0.0 };
			if (a_params.argCount >= 2 && a_params.args) {
				kbOrMouse = a_params.args[1];
			}
			parent.Invoke("onItemPress", nullptr, &kbOrMouse, 1);
		}
	};

	class EntryRollOverHandler : public RE::GFxFunctionHandler
	{
	public:
		void Call(Params& a_params) override
		{
			if (!a_params.thisPtr) {
				return;
			}
			RE::GFxValue itemIndex, parent;
			if (!a_params.thisPtr->GetMember("itemIndex", &itemIndex) || itemIndex.IsUndefined() || !a_params.thisPtr->GetMember("_parent", &parent)) {
				return;
			}
			RE::GFxValue anim, dis;
			const bool   animating = parent.GetMember("listAnimating", &anim) && anim.IsBool() && anim.GetBool();
			const bool   disabled = parent.GetMember("bDisableInput", &dis) && dis.IsBool() && dis.GetBool();
			if (animating || disabled) {
				return;
			}
			const RE::GFxValue args[2] = { itemIndex, RE::GFxValue(0.0) };
			parent.Invoke("doSetSelectedIndex", nullptr, args, 2);
			parent.SetMember("bMouseDrivenNav", RE::GFxValue(true));
		}
	};

	template <class F>
	class MenuPressHandler : public RE::GFxFunctionHandler
	{
	public:
		MenuPressHandler() = default;
		MenuPressHandler(const char* a_menu, const char* a_func, const char* memberHooked) :
			menuName(a_menu),
			buttonPressFunc(a_func),
			hookMemberName(memberHooked)
		{}

		void Call(Params& a_params) override;
		bool InjectJournalEntry(RE::GFxMovieView* a_view, RE::GFxValue& a_page);

		// members
		const char* menuName{};
		const char* buttonPressFunc{};
		const char* hookMemberName{};
	};

	class Manager :
		public REX::Singleton<Manager>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
		public RE::BSTEventSink<SKSE::ModCallbackEvent>
	{
	public:
		void Register();
		void LoadMCMSettings(const CSimpleIniA& a_ini);

		void UpdateListVisuals(RE::GFxMovieView* a_view, RE::GFxValue& a_listObj, std::uint32_t a_numItems);

		bool GetConsoleOpen() const { return consoleOpen; }

	private:
		template <class F>
		struct MenuSettings
		{
			MenuSettings() = default;
			MenuSettings(const char* a_tweenMenuName, const char* a_menu, const char* a_func, const char* memberHooked) :
				pressHandler(a_menu, a_func, memberHooked),
				tweenMenuName(a_tweenMenuName)
			{}

			void TryOpenFromTweenMenu();
			void SetTweenMenuOpen(const RE::BSFixedString& a_callback);

			// members
			MenuPressHandler<F> pressHandler;
			const char*         tweenMenuName;
			bool                openFromPause{ true };
			bool                openFromTween{ false };
		};

		void SetupJournalMenu();

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
		EventResult ProcessEvent(const SKSE::ModCallbackEvent* a_evn, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;

		// members
		EntryPressHandler    entryPressHandler;
		EntryRollOverHandler entryRollOverHandler;

		MenuSettings<PhotoMode::Manager> photoMode{
			"OpenTween_PhotoMode",
			"$PM_Title_Menu",
			"pm_orig_onCategoryButtonPress",
			"pm_hooked"
		};
		MenuSettings<Gallery::Manager> photoGallery{
			"OpenTween_PhotoGallery",
			"$PM_PhotoGallery_Menu",
			"pg_orig_onCategoryButtonPress",
			"pg_hooked"
		};

		bool skyUI6Installed{ false };
		bool photoModeInjected{ false };
		bool consoleOpen{ false };
	};

	template <class F>
	inline void MenuPressHandler<F>::Call(Params& a_params)
	{
		if (a_params.argCount < 1 || !a_params.args || !a_params.thisPtr) {
			return;
		}
		RE::GFxValue entry, textVal;
		if (a_params.args[0].GetMember("entry", &entry) && entry.GetMember("text", &textVal) && textVal.IsString() && std::string_view(textVal.GetString()) == menuName) {
			RE::UIMessageQueue::GetSingleton()->AddMessage(RE::JournalMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			SKSE::GetTaskInterface()->AddTask([]() { MANAGER(F)->Activate(); });
			return;
		}
		a_params.thisPtr->Invoke(buttonPressFunc, nullptr, a_params.args, 1);
	}

	template <class F>
	inline bool MenuPressHandler<F>::InjectJournalEntry(RE::GFxMovieView* a_view, RE::GFxValue& a_page)
	{
		RE::GFxValue listObj, entryList;
		if (!a_page.GetMember("CategoryList", &listObj) || !listObj.IsObject() || !listObj.GetMember("entryList", &entryList) || !entryList.IsArray()) {
			return false;
		}

		const std::uint32_t arraySize = entryList.GetArraySize();
		if (arraySize == 0) {
			return false;
		}

		std::uint32_t quitIdx = arraySize;
		for (std::uint32_t i = 0; i < arraySize; ++i) {
			RE::GFxValue element, textVal;
			if (entryList.GetElement(i, &element) && element.IsObject() && element.GetMember("text", &textVal) && textVal.IsString()) {
				std::string_view text{ textVal.GetString() };
				if (text == menuName) {
					return true;
				}
				if (text == "$QUIT") {
					quitIdx = i;
				}
			}
		}

		RE::GFxValue newEntry;
		a_view->CreateObject(&newEntry);
		newEntry.SetMember("text", menuName);

		if (quitIdx < arraySize) {
			RE::GFxValue last;
			entryList.GetElement(arraySize - 1, &last);
			entryList.PushBack(last);
			for (std::uint32_t i = arraySize - 1; i > quitIdx; --i) {
				RE::GFxValue temp;
				entryList.GetElement(i - 1, &temp);
				entryList.SetElement(i, temp);
			}
			entryList.SetElement(quitIdx, newEntry);
			a_page.Invoke("UpdateIndices", nullptr, nullptr, 0);
		} else {
			entryList.PushBack(newEntry);
		}

		if (!a_page.HasMember(hookMemberName)) {
			RE::GFxValue orig;
			if (a_page.GetMember("onCategoryButtonPress", &orig)) {
				a_page.SetMember(buttonPressFunc, orig);
				RE::GFxValue hook;
				a_view->CreateFunction(&hook, this);
				a_page.SetMember("onCategoryButtonPress", hook);
			}
			a_page.SetMember(hookMemberName, RE::GFxValue(true));
		}

		Manager::GetSingleton()->UpdateListVisuals(a_view, listObj, entryList.GetArraySize());
		listObj.Invoke("InvalidateData", nullptr, nullptr, 0);
		return true;
	}

	template <class F>
	inline void Manager::MenuSettings<F>::TryOpenFromTweenMenu()
	{
		if (openFromTween) {
			openFromTween = false;
			MANAGER(F)->Activate();
		}
	}

	template <class F>
	inline void Manager::MenuSettings<F>::SetTweenMenuOpen(const RE::BSFixedString& a_callback)
	{
		if (a_callback == tweenMenuName) {
			openFromTween = true;
			SKSE::GetTaskInterface()->AddUITask([this]() {
				TryOpenFromTweenMenu();
			});
		}
	}
}
