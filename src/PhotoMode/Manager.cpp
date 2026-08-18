#include "Manager.h"

#include "Gallery/Manager.h"
#include "Hotkeys.h"
#include "IGCSBridge/Bridge.h"  // IGCSDOF lifecycle + per-frame camera feed
#include "ImGui/IconsFonts.h"
#include "ImGui/Styles.h"
#include "ImGui/Widgets.h"
#include "Screenshots/Manager.h"
#include "Shared.h"

#include "Input.h"

namespace PhotoMode
{
	void Manager::Register()
	{
		RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(this);
		logger::info("Registered for menu open/close event");
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		freeCameraSpeed = static_cast<float>(a_ini.GetDoubleValue("Settings", "fFreeCameraTranslationSpeed", freeCameraSpeed));
		freezeTimeOnStart = a_ini.GetBoolValue("Settings", "bFreezeTimeOnStart", freezeTimeOnStart);
	}

	bool Manager::CanShowMenu()
	{
		if (!Shared::CanShowMenu()) {
			return false;
		}

		if (RE::MenuControls::GetSingleton()->InBeastForm() || RE::VATS::GetSingleton()->mode == RE::VATS::VATS_MODE::kKillCam) {
			return false;
		}

		if (MANAGER(Gallery)->IsActive()) {
			return false;
		}

		return true;
	}

	bool Manager::ShouldBlockInput() const
	{
		return blockInputToPhotoMode || MANAGER(Gallery)->IsActive();
	}

	bool Manager::IsActive() const
	{
		return activated;
	}

	bool Manager::IsHidden() const
	{
		return hiddenUI;
	}

	void Manager::ToggleUI()
	{
		hiddenUI = !hiddenUI;

		const auto UI = RE::UI::GetSingleton();
		UI->ShowMenus(!UI->IsShowingMenus());
		RE::PlaySound("UIMenuFocus");

		if (!hiddenUI) {
			restoreLastFocusID = true;
		}
	}

	void Manager::Activate()
	{
		RE::PlaySound("UIMenuOK");

		cameraTab.GetOriginalState();
		timeTab.GetOriginalState();

		const auto player = RE::PlayerCharacter::GetSingleton();
		characterTab.emplace(player->GetFormID(), Character(player));
		cachedCharacter.reset(player);

		filterTab.GetOriginalState();

		const auto pcCamera = RE::PlayerCamera::GetSingleton();
		originalcameraState = pcCamera->currentState ? pcCamera->currentState->id : RE::CameraState::kThirdPerson;

		menusAlreadyHidden = !RE::UI::GetSingleton()->IsShowingMenus();
		if (menusAlreadyHidden) {
			hiddenUI = true;
		}

		// disable saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.set(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		// toggle freecam
		if (originalcameraState != RE::CameraState::kFree) {
			pcCamera->ToggleFreeCameraMode(false);
			//RE::ControlMap::GetSingleton()->PushInputContext(RE::ControlMap::InputContextID::kTFCMode);
		}

		// disable controls
		TogglePlayerControls(false);

		// apply mcm settings
		FreeCamera::translateSpeed = freeCameraSpeed;
		if (freezeTimeOnStart) {
			RE::Main::GetSingleton()->freezeTime = true;
		}

		// load default screenshot keys
		// keybindings can change?
		MANAGER(Input)->LoadDefaultKeys();

		activated = true;
		// IGCSDOF: expose the native Photo Mode camera only while Photo Mode is active.
		IGCSBridge::Bridge::GetSingleton()->OnPhotoModeActivated();
		if (activeGlobal) {
			activeGlobal->value = 1.0f;
		}
	}

	void Manager::TogglePlayerControls(bool a_enable)
	{
		RE::ControlMap::GetSingleton()->ToggleControls(controlFlags, a_enable, true);

		if (const auto pcControls = RE::PlayerControls::GetSingleton()) {
			pcControls->readyWeaponHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->sneakHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->autoMoveHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->shoutHandler->SetInputEventHandlingEnabled(a_enable);
			pcControls->attackBlockHandler->SetInputEventHandlingEnabled(a_enable);
		}
	}

	bool Manager::OnFrameUpdate()
	{
		if (!CanShowMenu()) {
			Deactivate();
			return false;
		}

		// disable controls
		if (ImGui::GetIO().WantTextInput) {
			if (!allowTextInput) {
				allowTextInput = true;
				RE::ControlMap::GetSingleton()->AllowTextInput(true);
			}
		} else if (allowTextInput) {
			allowTextInput = false;
			RE::ControlMap::GetSingleton()->AllowTextInput(false);
		}
		TogglePlayerControls(false);

		timeTab.OnFrameUpdate();
		// IGCSDOF: publish the live camera packet and reconnect if the addon appears late.
		IGCSBridge::Bridge::GetSingleton()->OnFrameUpdate();

		return true;
	}

	bool Manager::HasOverlay() const
	{
		return overlaysTab.HasOverlay();
	}

	void Manager::Deactivate()
	{
		// IGCSDOF: end any active screenshot session and clear the shared camera packet.
		IGCSBridge::Bridge::GetSingleton()->OnPhotoModeDeactivated();
		Revert(true);

		//reset characters
		characterTab.clear();
		cachedCharacter = nullptr;

		// reset camera
		if (originalcameraState != RE::CameraState::kFree) {
			RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
			//RE::ControlMap::GetSingleton()->PopInputContext(RE::ControlMap::InputContextID::kTFCMode);
		}

		// reset controls
		allowTextInput = false;
		RE::ControlMap::GetSingleton()->AllowTextInput(false);
		TogglePlayerControls(true);

		// allow saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.reset(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		// reset variables
		ImGui::ClearImGuiState();

		hiddenUI = false;

		noItemsFocused = false;
		restoreLastFocusID = false;
		lastFocusedID = 0;

		updateKeyboardFocus = false;

		MANAGER(Input)->ToggleCursor(false);
		MANAGER(Input)->ResetInputDevices();

		activated = false;
		if (activeGlobal) {
			activeGlobal->value = 0.0f;
		}

		RE::PlaySound("UIMenuCancel");
	}

	void Manager::ToggleActive()
	{
		if (!IsActive()) {
			if (CanShowMenu() && !ShouldBlockInput()) {
				Activate();
			}
		} else {
			if (!ImGui::GetIO().WantTextInput && !ShouldBlockInput()) {
				Deactivate();
			}
		}
	}

	void Manager::Revert(bool a_deactivate)
	{
		const std::int32_t tabIndex = a_deactivate ? -1 : currentTab;

		// Camera
		if (tabIndex == -1 || tabIndex == kCamera) {
			cameraTab.RevertState(a_deactivate);
			if (!a_deactivate) {
				FreeCamera::translateSpeed = freeCameraSpeed;
			}
			revertENB = true;
		}
		// Time/Weather
		if (tabIndex == -1 || tabIndex == kTime) {
			timeTab.RevertState();
		}

		// Character
		if (tabIndex == kCharacter) {
			if (cachedCharacter) {
				characterTab[cachedCharacter->GetFormID()].RevertState();
			}
		} else if (tabIndex == -1) {
			std::ranges::for_each(characterTab, [](auto& data) {
				data.second.RevertState();
			});
		}

		// Filters
		if (tabIndex == -1 || tabIndex == kFilters) {
			filterTab.RevertState(tabIndex == -1);
		}
		// Overlays
		if (tabIndex == -1 || tabIndex == kOverlays) {
			overlaysTab.RevertOverlays();
		}

		if (a_deactivate) {
			// reset UI
			if ((!menusAlreadyHidden || hiddenUI) && !RE::UI::GetSingleton()->IsShowingMenus()) {
				RE::UI::GetSingleton()->ShowMenus(true);
			}
			resetWindow = true;
			resetPlayerTabs = true;
		} else {
			RE::PlaySound("UIMenuOK");

			const auto notification = std::format("{}", resetAll ? "$PM_ResetNotifAll"_T : TRANSLATE(tabResetNotifs[currentTab]));
			RE::SendHUDMessage::ShowHUDMessage(notification.c_str());

			if (resetAll) {
				resetAll = false;
			}
		}
	}

	void Manager::QuitOnEscape()
	{
		if (IsHidden() || noItemsFocused) {
			Deactivate();
		}
	}

	bool Manager::GetResetAll() const
	{
		return resetAll;
	}

	void Manager::DoResetAll()
	{
		resetAll = true;
	}

	void Manager::NavigateTab(bool a_left)
	{
		constexpr auto tabsSizeInt32 = static_cast<uint32_t>(tabs.size());
		if (a_left) {
			currentTab = (currentTab - static_cast<uint32_t>(1) + tabsSizeInt32) % tabsSizeInt32;
		} else {
			currentTab = (currentTab + static_cast<uint32_t>(1)) % tabsSizeInt32;
		}
		UpdateKeyboardFocus();
		RE::PlaySound("UIJournalTabsSD");
	}

	void Manager::UpdateKeyboardFocus()
	{
		updateKeyboardFocus = true;
	}

	float Manager::GetViewRoll(const float a_fallback) const
	{
		return IsActive() ? cameraTab.GetViewRoll() : a_fallback;
	}

	float Manager::GetViewRoll() const
	{
		return cameraTab.GetViewRoll();
	}

	void Manager::SetViewRoll(float a_value)
	{
		cameraTab.SetViewRoll(a_value);
	}

	void Manager::OnDataLoad()
	{
		overlaysTab.LoadOverlays();

		activeGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("PhotoMode_IsActive");
		resetRootIdle = RE::TESForm::LookupByEditorID<RE::TESIdleForm>("ResetRoot");
	}

	std::pair<ImGui::Texture*, float> Manager::GetOverlay() const
	{
		return overlaysTab.GetCurrentOverlay();
	}

	bool Manager::IsCursorHoveringOverWindow() const
	{
		return isCursorHoveringOverWindow;
	}

	void Manager::Draw()
	{
		ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
		ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
		{
			if (!IsHidden()) {
				overlaysTab.DrawOverlays();
				CameraGrid::Draw();

				DrawBar();
				DrawControls();
			}
		}
		ImGui::End();
	}

	void Manager::DrawOverlays()
	{
		ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
		ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

		ImGui::Begin("##MainOverlay", nullptr, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
		{
			// render hierachy
			overlaysTab.DrawOverlays();
		}
		ImGui::End();
	}

	void Manager::DrawControls()
	{
		const static auto center = ImGui::GetNativeViewportCenter();
		const static auto size = ImGui::GetNativeViewportSize();

		const static auto third_width = size.x / 3;
		const static auto third_height = size.y / 3;

		constexpr auto windowFlags = ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDecoration;

		ImGui::SetNextWindowPos(ImVec2(center.x + third_width, center.y + third_height * 0.8f), ImGuiCond_Always, ImVec2(0.5, 0.5));
		ImGui::SetNextWindowSize(ImVec2(size.x / 3.25f, size.y / 3.125f));

		bool navigateWithMouse = MANAGER(Input)->CanNavigateWithMouse();

		ImGui::Begin("$PM_Title_Menu"_T, nullptr, windowFlags);
		{
			ImGui::ExtendWindowPastBorder();

			if (resetWindow) {
				currentTab = kCamera;
			}

			// console already covers menu
			if (blockInputToPhotoMode) {
				ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, ImGui::GetStyle().Alpha);
			}

			ImGui::BeginDisabled(blockInputToPhotoMode);
			{
				// Q [Tab Tab Tab Tab Tab] E
				ImGui::BeginGroup();
				{
					const auto buttonSize = ImGui::ButtonIcon(MANAGER(Hotkeys)->PreviousTabKey());
					ImGui::SameLine();

					const float tabWidth = (ImGui::GetContentRegionAvail().x - (buttonSize.x + ImGui::GetStyle().ItemSpacing.x * tabs.size())) / tabs.size();

					ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4());

					for (std::int32_t i = 0; i < tabs.size(); ++i) {
						bool activeTab = (currentTab == i) || hoveredTabs[i] == true;
						if (!activeTab) {
							ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
						} else {
							ImGui::PushFont(nullptr, MANAGER(IconFont)->GetLargeIconSize());
						}
						ImGui::Button(tabIcons[i], ImVec2(tabWidth, ImGui::GetFrameHeightWithSpacing()));
						if (ImGui::IsItemClicked() && currentTab != i) {
							currentTab = i;
						}
						hoveredTabs[i] = ImGui::IsItemHovered(ImGuiHoveredFlags_NoNavOverride);
						if (!activeTab) {
							ImGui::PopStyleColor();
						} else {
							ImGui::PopFont();
						}
						ImGui::SameLine();
					}
					ImGui::PopStyleColor(3);
					ImGui::PopItemFlag();

					ImGui::SameLine();
					ImGui::ButtonIcon(MANAGER(Hotkeys)->NextTabKey());
				}
				ImGui::EndGroup();

				//		CAMERA
				// ----------------
				ImGui::CenteredText(currentTab != TAB_TYPE::kCharacter ? TRANSLATE(tabs[currentTab]) : characterTab[cachedCharacter->GetFormID()].GetName());
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));

				// content
				ImGui::SetNextWindowBgAlpha(0.0f);  // child bg color is added ontop of window
				ImGui::BeginChild("##PhotoModeChild", ImVec2(0, 0), ImGuiChildFlags_None, windowFlags);
				{
					ImGui::Spacing();

					if (restoreLastFocusID) {
						navigateWithMouse ? ImGui::SetHoveredID(lastHoveredID) : ImGui::SetFocusID(lastFocusedID, ImGui::GetCurrentWindow());

						restoreLastFocusID = false;
					} else if (updateKeyboardFocus) {
						if (currentTab == TAB_TYPE::kCharacter) {
							resetPlayerTabs = true;
						}
						navigateWithMouse ? ImGui::SetItemDefaultFocus() : ImGui::SetKeyboardFocusHere();
						updateKeyboardFocus = false;
					}

					switch (currentTab) {
					case TAB_TYPE::kCamera:
						{
							if (resetWindow) {
								navigateWithMouse ? ImGui::SetItemDefaultFocus() : ImGui::SetKeyboardFocusHere();
								resetWindow = false;
							}
							cameraTab.Draw();
						}
						break;
					case TAB_TYPE::kTime:
						timeTab.Draw();
						break;
					case TAB_TYPE::kCharacter:
						{
							const auto& consoleRef = RE::Console::GetSelectedRef();
							if (!consoleRef || !consoleRef->Is(RE::FormType::ActorCharacter) || consoleRef->IsDisabled() || consoleRef->IsDeleted() || !consoleRef->Is3DLoaded()) {
								prevCachedCharacter = cachedCharacter;
								cachedCharacter.reset(RE::PlayerCharacter::GetSingleton());
							} else {
								prevCachedCharacter = cachedCharacter;
								cachedCharacter = consoleRef;
							}

							if (cachedCharacter != prevCachedCharacter) {
								if (prevCachedCharacter) {
									if (const auto it = characterTab.find(prevCachedCharacter->GetFormID()); it != characterTab.end()) {
										it->second.SaveFormComboStates();
									}
								}

								if (auto it = characterTab.find(cachedCharacter->GetFormID()); it == characterTab.end()) {
									characterTab.emplace(cachedCharacter->GetFormID(), Character(cachedCharacter->As<RE::Actor>()));
								} else {
									it->second.RestoreFormComboStates();
								}

								resetPlayerTabs = true;
							}

							characterTab[cachedCharacter->GetFormID()].Draw(resetPlayerTabs, navigateWithMouse);

							if (resetPlayerTabs) {
								resetPlayerTabs = false;
							}
						}
						break;
					case TAB_TYPE::kFilters:
						filterTab.Draw();
						break;
					case TAB_TYPE::kOverlays:
						overlaysTab.Draw();
						break;
					default:
						break;
					}

					noItemsFocused = navigateWithMouse ?
					                     !ImGui::IsAnyItemActive() :
					                     (!ImGui::IsAnyItemFocused() || !ImGui::IsWindowFocused());
					lastFocusedID = ImGui::GetFocusID();
					lastHoveredID = ImGui::GetHoveredID();
				}
				ImGui::EndChild();
			}
			ImGui::EndDisabled();

			if (blockInputToPhotoMode) {
				ImGui::PopStyleVar();
			}

			if (navigateWithMouse) {
				UpdateMouseHoveringOverWindow();
			}
		}
		ImGui::End();
	}

	void Manager::DrawBar() const
	{
		const static auto center = ImGui::GetNativeViewportCenter();
		const static auto size = ImGui::GetNativeViewportSize();
		const static auto offsetY = size.y / 25.0f;

		ImGui::SetNextWindowPos(ImVec2(center.x, size.y - offsetY), ImGuiCond_Always, ImVec2(0.5, 0.5));

		ImGui::Begin("##Bar", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);  // same offset as control window
		{
			ImGui::ExtendWindowPastBorder();

			const static auto takePhotoLabel = "$PM_TAKEPHOTO"_T;
			const static auto toggleMenusLabel = "$PM_TOGGLEMENUS"_T;
			const auto        resetLabel = GetResetAll() ? "$PM_RESET_ALL"_T : "$PM_RESET"_T;
			const static auto freezeTimeLabel = "$PM_FREEZETIME"_T;
			const static auto panCameraLabel = "$PM_PAN_CAMERA"_T;

			const ImGui::ButtonBarItem items[] = {
				{ MANAGER(Hotkeys)->PanCameraIcon(), panCameraLabel, MANAGER(Input)->CanNavigateWithMouse() },
				{ MANAGER(Hotkeys)->TakePhotoIcon(), takePhotoLabel },
				{ MANAGER(Hotkeys)->ToggleMenusIcon(), toggleMenusLabel },
				{ MANAGER(Hotkeys)->FreezeTimeIcon(), freezeTimeLabel },
				{ MANAGER(Hotkeys)->ResetIcon(), resetLabel },
			};

			ImGui::ButtonBar(items);
		}
		ImGui::End();
	}

	void Manager::UpdateMouseHoveringOverWindow()
	{
		constexpr float buffer = 50.0f;
		auto            mousePos = ImGui::GetMousePos();
		auto            winPos = ImGui::GetWindowPos();
		auto            winSize = ImGui::GetWindowSize();

		isCursorHoveringOverWindow =
			mousePos.x >= winPos.x - buffer &&
			mousePos.x <= winPos.x + winSize.x + buffer &&
			mousePos.y >= winPos.y - buffer &&
			mousePos.y <= winPos.y + winSize.y + buffer;
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn) {
			return EventResult::kContinue;
		}

		if (a_evn->menuName == RE::Console::MENU_NAME) {
			blockInputToPhotoMode = a_evn->opening;
			if (a_evn->opening) {
				if (IsActive() && IsHidden()) {
					ToggleUI();
				}
			} else if (IsActive() && MANAGER(Input)->DoNavigateWithMouse()) {
				Input::Manager::ToggleCursor(true);
			}
		}

		return EventResult::kContinue;
	}
}

