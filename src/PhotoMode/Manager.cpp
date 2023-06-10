#include "Manager.h"

#include "ImGui/IconsFonts.h"
#include "ImGui/Util.h"
#include "Input.h"
#include "Screenshots/Manager.h"
#include "Settings.h"

namespace PhotoMode
{
	void Manager::Register()
	{
		RE::UI::GetSingleton()->AddEventSink(GetSingleton());

		logger::info("Registered for menu open/close event");
	}

	void Manager::LoadSettings(CSimpleIniA& a_ini)
	{
		std::string kPattern{ IO.keyboard.GetPattern() };
		ini::get_value(a_ini, kPattern, "PhotoMode", "HotKey", ";Toggle photomode\n\n;Default is N\n;DXScanCodes : https://www.creationkit.com/index.php?title=Input_Script");
		IO.keyboard.SetPattern(kPattern);

		std::string gPattern{ IO.gamePad.GetPattern() };
		ini::get_value(a_ini, gPattern, "PhotoMode", "GamepadKey", ";Default is LShoulder+RShoulder");
		IO.gamePad.SetPattern(gPattern);
	}

	void Manager::SaveSettings(CSimpleIniA& a_ini) const
	{
		a_ini.SetValue("PhotoMode", "Key", IO.keyboard.GetPattern().data(), ";Toggle photomode\n\n;Default is N\n;DXScanCodes : https://www.creationkit.com/index.php?title=Input_Script");
		a_ini.SetValue("PhotoMode", "GamepadKey", IO.gamePad.GetPattern().data(), ";Default is LShoulder+RShoulder");
	}

	bool Manager::GetValid()
	{
		static constexpr std::array badMenus{
			RE::MainMenu::MENU_NAME,
			RE::MistMenu::MENU_NAME,
			RE::JournalMenu::MENU_NAME,
			RE::InventoryMenu::MENU_NAME,
			RE::MagicMenu::MENU_NAME,
			RE::MapMenu::MENU_NAME,
			RE::BookMenu::MENU_NAME,
			RE::LockpickingMenu::MENU_NAME,
			RE::StatsMenu::MENU_NAME,
			RE::ContainerMenu::MENU_NAME,
			RE::DialogueMenu::MENU_NAME,
			RE::CraftingMenu::MENU_NAME,
			RE::TweenMenu::MENU_NAME,
			RE::SleepWaitMenu::MENU_NAME,
			RE::RaceSexMenu::MENU_NAME,
			"CustomMenu"sv
		};

		if (const auto player = RE::PlayerCharacter::GetSingleton(); !player || !player->Get3D() && !player->Get3D(true)) {
			return false;
		}

		if (const auto UI = RE::UI::GetSingleton(); !UI || std::ranges::any_of(badMenus, [&](const auto& menuName) { return UI->IsMenuOpen(menuName); })) {
			return false;
		}

		return true;
	}

	bool Manager::IsActive() const
	{
		return activated;
	}

	void Manager::Activate()
	{
		cameraTab.GetOriginalState();
		timeTab.GetOriginalState();
		playerTab.GetOriginalState();
		filterTab.GetOriginalState();

		const auto pcCamera = RE::PlayerCamera::GetSingleton();
		if (pcCamera->IsInFirstPerson()) {
			cameraState = RE::CameraState::kFirstPerson;
		} else if (pcCamera->IsInFreeCameraMode()) {
			cameraState = RE::CameraState::kFree;
		} else {
			cameraState = RE::CameraState::kThirdPerson;
		}

		menusAlreadyHidden = !RE::UI::GetSingleton()->IsShowingMenus();

		//disable saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.set(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		// toggle freecam
		if (cameraState != RE::CameraState::kFree) {
			RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
		}

		activated = true;
	}

	void Manager::Revert(bool a_deactivate)
	{
		const std::int32_t tabIndex = a_deactivate ? -1 : currentTab;

		// Camera
		if (tabIndex == -1 || tabIndex == kCamera) {
			cameraTab.RevertState();
		}
		// Time/Weather
		if (tabIndex == -1 || tabIndex == kTime) {
			timeTab.RevertState();
		}
		// Player
		if (tabIndex == -1 || tabIndex == kPlayer) {
			playerTab.RevertState();
		}
		// Filters
		if (tabIndex == -1 || tabIndex == kFilters) {
			filterTab.RevertState(tabIndex == -1);
		}
		// Settings
		if (tabIndex == kSettings) {
			// tbd
		}

		if (a_deactivate) {
			// reset UI
			if (!menusAlreadyHidden && !RE::UI::GetSingleton()->IsShowingMenus()) {
				RE::UI::GetSingleton()->ShowMenus(true);
			}
			resetWindow = true;
		} else {
			RE::PlaySound("UIMenuOK");

			const auto notification = fmt::format("{}", resetAll ? "$PM_ResetNotifAll"_T : TRANSLATE(tabResetNotifs[currentTab]));
			RE::DebugNotification(notification.c_str());

			if (resetAll) {
				resetAll = false;
			}
		}
	}

	void Manager::Deactivate()
	{
		Revert(true);

		// reset controls
		const auto controlMap = RE::ControlMap::GetSingleton();
		controlMap->ToggleControls(controlFlags, true);
		controlMap->ignoreKeyboardMouse = false;

		// allow saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.reset(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		// reset camera
		switch (cameraState) {
		case RE::CameraState::kFirstPerson:
			RE::PlayerCamera::GetSingleton()->ForceFirstPerson();
			break;
		case RE::CameraState::kThirdPerson:
			RE::PlayerCamera::GetSingleton()->ForceThirdPerson();
			break;
		default:
			break;
		}

		activated = false;
	}

	void Manager::ToggleActive()
	{
		if (!IsActive()) {
			if (GetValid() && !RE::UI::GetSingleton()->IsMenuOpen(RE::Console::MENU_NAME)) {
				Activate();
			}
		} else {
			if (!ImGui::GetIO().WantTextInput) {
				Deactivate();
			}
		}
	}

	void Manager::ToggleActive_Input(const KeyCombination*)
	{
		GetSingleton()->ToggleActive();
	}

	float Manager::GetResetHoldDuration()
	{
		return 0.5f;
	}

	bool Manager::GetResetAll() const
	{
		return resetAll;
	}

	void Manager::DoResetAll()
	{
		resetAll = true;
	}

	void Manager::SetInputType(Input::TYPE a_inputType)
	{
		inputType = a_inputType;
	}

	std::uint32_t Manager::ResetKey() const
	{
		switch (inputType) {
		case Input::TYPE::kKeyboard:
			return KEY::kR;
		case Input::TYPE::kGamepadDirectX:
			return GAMEPAD_DIRECTX::kY;
		case Input::TYPE::kGamepadOrbis:
			return GAMEPAD_ORBIS::kPS3_Y;
		default:
			return 0;
		}
	}

	std::uint32_t Manager::TakePhotoKey() const
	{
		switch (inputType) {
		case Input::TYPE::kKeyboard:
			return KEY::kPrintScreen;
		case Input::TYPE::kGamepadDirectX:
			return GAMEPAD_DIRECTX::kBack;
		case Input::TYPE::kGamepadOrbis:
			return GAMEPAD_ORBIS::kPS3_Back;
		default:
			return 0;
		}
	}

	std::uint32_t Manager::ToggleUIKey() const
	{
		switch (inputType) {
		case Input::TYPE::kKeyboard:
			return KEY::kT;
		case Input::TYPE::kGamepadDirectX:
			return GAMEPAD_DIRECTX::kX;
		case Input::TYPE::kGamepadOrbis:
			return GAMEPAD_ORBIS::kPS3_X;
		default:
			return 0;
		}
	}

	std::uint32_t Manager::RightTabKey() const
	{
		switch (inputType) {
		case Input::TYPE::kKeyboard:
			return KEY::kE;
		case Input::TYPE::kGamepadDirectX:
			return GAMEPAD_DIRECTX::kRightTrigger;
		case Input::TYPE::kGamepadOrbis:
			return GAMEPAD_ORBIS::kPS3_RT;
		default:
			return 0;
		}
	}

	std::uint32_t Manager::LeftTabKey() const
	{
		switch (inputType) {
		case Input::TYPE::kKeyboard:
			return KEY::kQ;
		case Input::TYPE::kGamepadDirectX:
			return GAMEPAD_DIRECTX::kLeftTrigger;
		case Input::TYPE::kGamepadOrbis:
			return GAMEPAD_ORBIS::kPS3_LT;
		default:
			return 0;
		}
	}

	void Manager::NavigateTab(bool a_left)
	{
		if (a_left) {
			currentTab = (currentTab - 1 + tabs.size()) % tabs.size();
		} else {
			currentTab = (currentTab + 1) % tabs.size();
		}
		updateKeyboardFocus = true;
	}

	void Manager::CheckActive(RE::InputEvent* const* a_event)
	{
		IO.keyboard.Process(a_event);
		if (RE::BSInputDeviceManager::GetSingleton()->IsGamepadEnabled()) {
			IO.gamePad.Process(a_event);
		}
	}

	float Manager::GetViewRoll(const float a_fallback) const
	{
		return IsActive() ? cameraTab.GetViewRoll() : a_fallback;
	}

	void Manager::Draw()
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);
		{
			DrawControls();
			DrawBar();

			cameraTab.DrawGrid();
		}
		ImGui::End();
	}

	bool Manager::OnFrameUpdate()
	{
		if (!GetValid()) {
			Deactivate();
			return false;
		}

		// disable controls
		const auto controlMap = RE::ControlMap::GetSingleton();
		controlMap->ignoreKeyboardMouse = ImGui::GetIO().WantTextInput;
		controlMap->ToggleControls(controlFlags, false);

		timeTab.OnFrameUpdate();

		return true;
	}

	void Manager::DrawControls()
	{
		const auto viewport = ImGui::GetMainViewport();
		const auto io = ImGui::GetIO();
		const auto styling = ImGui::GetStyle();

		const static auto center = viewport->GetCenter();

		const static auto third_width = viewport->Size.x / 3;
		const static auto third_height = viewport->Size.y / 3;

		ImGui::SetNextWindowPos(ImVec2(center.x + third_width, center.y + third_height * 0.8f), ImGuiCond_Always, ImVec2(0.5, 0.5));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x / 3.25f, viewport->Size.y / 3.125f));

		constexpr auto windowFlags = ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoDecoration;

		ImGui::Begin("$PM_Title_Menu"_T, nullptr, windowFlags);
		{
			if (resetWindow) {
				currentTab = kCamera;
			}

			static auto iconMgr = Icon::Manager::GetSingleton();

			// Q [Tab Tab Tab Tab Tab] E
			ImGui::BeginGroup();
			{
				const auto buttonSize = ImGui::ButtonIcon(inputType, LeftTabKey());
				ImGui::SameLine();

				const float tabWidth = (ImGui::GetContentRegionAvail().x - (buttonSize.x + styling.ItemSpacing.x * tabs.size())) / tabs.size();

				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				for (std::int32_t i = 0; i < tabs.size(); ++i) {
					if (currentTab != i) {
						ImGui::BeginDisabled(true);
					} else {
						ImGui::PushFont(iconMgr->GetBigIconFont());
					}
					ImGui::Button(tabIcons[i], ImVec2(tabWidth, ImGui::GetFrameHeightWithSpacing()));
					if (currentTab != i) {
						ImGui::EndDisabled();
					} else {
						ImGui::PopFont();
					}
					ImGui::SameLine();
				}
				ImGui::PopStyleColor();
				ImGui::PopItemFlag();

				ImGui::SameLine();
				ImGui::ButtonIcon(inputType, RightTabKey());
			}
			ImGui::EndGroup();

			//		CAMERA
			// ----------------
			ImGui::CenteredText(TRANSLATE(tabs[currentTab]));
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 3.0f);

			// content
			ImGui::SetNextWindowBgAlpha(0.0f);  // child bg color is added ontop of window
			ImGui::BeginChild("##PhotoModeChild", ImVec2(0, 0), false, windowFlags);
			{
				ImGui::Spacing();

				if (updateKeyboardFocus) {
					ImGui::SetKeyboardFocusHere();
					RE::PlaySound("UIJournalTabsSD");
					updateKeyboardFocus = false;
				}

				switch (currentTab) {
				case TAB_TYPE::kCamera:
					{
						if (resetWindow) {
							ImGui::SetKeyboardFocusHere();
							resetWindow = false;
						}
						cameraTab.Draw();
					}
					break;
				case TAB_TYPE::kTime:
					timeTab.Draw();
					break;
				case TAB_TYPE::kPlayer:
					playerTab.Draw();
					break;
				case TAB_TYPE::kFilters:
					filterTab.Draw();
					break;
				case TAB_TYPE::kSettings:
					// TBD
					break;
				default:
					break;
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void Manager::DrawBar() const
	{
		const auto viewport = ImGui::GetMainViewport();

		const static auto center = viewport->GetCenter();
		const static auto offset = viewport->Size.y / 20.25f;

		ImGui::SetNextWindowPos(ImVec2(center.x, viewport->Size.y - offset), ImGuiCond_Always, ImVec2(0.5, 0.5));

		ImGui::BeginChild("##Bar", ImVec2(viewport->Size.x / 3.5f, offset), false, ImGuiWindowFlags_NoBringToFrontOnFocus);  // same offset as control window
		{
			const static auto takePhotoLabel = "$PM_TAKEPHOTO"_T;
			const static auto toggleUILabel = "$PM_TOGGLEUI"_T;
			const auto        resetLabel = GetResetAll() ? "$PM_RESET_ALL"_T : "$PM_RESET"_T;
			const static auto exitLabel = "$PM_EXIT"_T;

			const auto& takePhotoIcon = Icon::Manager::GetSingleton()->GetIcon(inputType, TakePhotoKey());
			const auto& toggleUIIcon = Icon::Manager::GetSingleton()->GetIcon(inputType, ToggleUIKey());
			const auto& resetIcon = Icon::Manager::GetSingleton()->GetIcon(inputType, ResetKey());
			const auto& exitIcons = Icon::Manager::GetSingleton()->GetIcons(inputType, IO.GetKeys());

			// calc total elements width
			const ImGuiStyle& style = ImGui::GetStyle();

			float width = 0.0f;

			width += takePhotoIcon->size.x;
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(takePhotoLabel).x;
			width += style.ItemSpacing.x;

			width += toggleUIIcon->size.x;
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(toggleUILabel).x;
			width += style.ItemSpacing.x;

			width += resetIcon->size.x;
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(resetLabel).x;
			width += style.ItemSpacing.x;

			for (const auto& icon : exitIcons) {
				width += icon->size.x;
			}
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(exitLabel).x;

			// align at center
			ImGui::AlignForWidth(width);

			// draw
			ImGui::ButtonIconWithLabel(takePhotoLabel, takePhotoIcon, true);
			ImGui::SameLine();

			ImGui::ButtonIconWithLabel(toggleUILabel, toggleUIIcon, true);
			ImGui::SameLine();

			ImGui::ButtonIconWithLabel(resetLabel, resetIcon, true);
			ImGui::SameLine();

			ImGui::ButtonIconWithLabel(exitLabel, exitIcons, true);
		}
		ImGui::EndChild();
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn || !a_evn->opening) {
			return EventResult::kContinue;
		}

		const auto UI = RE::UI::GetSingleton();

		if (a_evn->menuName == RE::JournalMenu::MENU_NAME) {
			const auto menu = UI->GetMenu<RE::JournalMenu>(RE::JournalMenu::MENU_NAME);

			if (const auto& view = menu ? menu->systemTab.view : RE::GPtr<RE::GFxMovieView>()) {
				RE::GFxValue page;
				if (!view->GetVariable(&page, "_root.QuestJournalFader.Menu_mc.SystemFader.Page_mc")) {
					return EventResult::kContinue;
				}

				std::array<RE::GFxValue, 1> args;
				args[0] = true;
				if (!page.Invoke("SetShowMod", nullptr, args.data(), args.size())) {
					return EventResult::kContinue;
				}

				RE::GFxValue categoryList;
				if (page.GetMember("CategoryList", &categoryList)) {
					RE::GFxValue entryList;
					if (categoryList.GetMember("entryList", &entryList)) {
						RE::GFxValue entry;
						view->CreateObject(&entry);
						entry.SetMember("text", TRANSLATE("$PM_Title_Menu"));

						entryList.SetElement(3, entry);
						categoryList.Invoke("InvalidateData");
					}
				}
			}
		} else if (a_evn->menuName == RE::ModManagerMenu::MENU_NAME) {
			if (UI->IsMenuOpen(RE::JournalMenu::MENU_NAME)) {
				const auto msgQueue = RE::UIMessageQueue::GetSingleton();

				msgQueue->AddMessage(RE::ModManagerMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
				msgQueue->AddMessage(RE::JournalMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);

				RE::PlaySound("UIMenuOK");
				Activate();
			}
		}

		return EventResult::kContinue;
	}

	struct FromEulerAnglesZXY
	{
		static void thunk(RE::NiMatrix3* a_matrix, float a_z, float a_x, float a_y)
		{
			return func(a_matrix, a_z, a_x, Manager::GetSingleton()->GetViewRoll(a_y));
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// TESDataHandler idle array is not populated
	struct SetFormEditorID
	{
		static bool thunk(RE::TESIdleForm* a_this, const char* a_str)
		{
			if (!clib_util::string::is_empty(a_str)) {
				idles.AddForm(a_str, a_this);
			}
			return func(a_this, a_str);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            size{ 0x33 };
	};

	void InstallHooks()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(49814, 50744), 0x1B };  // FreeCamera::GetRotation
		stl::write_thunk_call<FromEulerAnglesZXY>(target.address());

		stl::write_vfunc<RE::TESIdleForm, SetFormEditorID>();
	}
}
