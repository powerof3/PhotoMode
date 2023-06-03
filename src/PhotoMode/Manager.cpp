#include "Manager.h"

#include "ImGui/Icons.h"
#include "ImGui/Util.h"
#include "Input.h"
#include "Overrides.h"
#include "Screenshots/Manager.h"
#include "Settings.h"

namespace PhotoMode
{
	void Manager::LoadSettings(CSimpleIniA& a_ini)
	{
		std::string kPattern{ IO.keyboard.GetPattern() };
		ini::get_value(a_ini, kPattern, "PhotoMode", "HotKey", ";Toggle photomode\n\n;Default is N\n;DXScanCodes : https://www.creationkit.com/index.php?title=Input_Script");
		IO.keyboard.SetPattern(kPattern);

		std::string gPattern{ IO.gamePad.GetPattern() };
		ini::get_value(a_ini, gPattern, "PhotoMode", "GamepadKey", ";Default is LShoulder+RShoulder");
		IO.gamePad.SetPattern(gPattern);
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
		GetOriginalState();

		RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
		RE::ControlMap::GetSingleton()->ToggleControls(controlFlags, false);

		activated = true;
	}

	void Manager::Deactivate()
	{
		Revert(true);

		Settings::GetSingleton()->SaveSettings();

		if (const auto camera = RE::PlayerCamera::GetSingleton()) {
			switch (originalCameraState) {
			case RE::CameraState::kFirstPerson:
				camera->ForceFirstPerson();
				break;
			default:
				camera->ForceThirdPerson();
				break;
			}
		}

		// reset controls
		const auto controlMap = RE::ControlMap::GetSingleton();
		controlMap->ToggleControls(controlFlags, true);
		controlMap->ignoreKeyboardMouse = false;

		// allow saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.reset(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);

		activated = false;
	}

	void Manager::ToggleActive(const KeyCombination*)
	{
		if (const auto mgr = GetSingleton(); !mgr->IsActive()) {
			if (GetValid() && !RE::UI::GetSingleton()->IsMenuOpen(RE::Console::MENU_NAME)) {
				mgr->Activate();
			}
		} else {
			if (!ImGui::GetIO().WantTextInput) {
				mgr->Deactivate();
			}
		}
	}

	void Manager::GetOriginalState()
	{
		originalState.GetState();

		Override::InitOverrides();

		// this isn't updated per frame
		currentState.player.visible = originalState.player.visible;
		if (RE::PlayerCamera::GetSingleton()->IsInFirstPerson()) {
			originalCameraState = RE::CameraState::kFirstPerson;
		} else {  // assume
			originalCameraState = RE::CameraState::kThirdPerson;
		}

		menusAlreadyHidden = !RE::UI::GetSingleton()->IsShowingMenus();

		// init imagespace
		const auto IMGS = RE::ImageSpaceManager::GetSingleton();
		if (IMGS->currentBaseData) {
			imageSpaceData = *IMGS->currentBaseData;
		}
		imageSpaceData.tint.amount = 0.0f;
		imageSpaceData.tint.color = { 1.0f, 1.0f, 1.0f };
		IMGS->overrideBaseData = &imageSpaceData;

		//disable saving
		RE::PlayerCharacter::GetSingleton()->byCharGenFlag.set(RE::PlayerCharacter::ByCharGenFlag::kDisableSaving);
	}

	void Manager::Revert(bool a_deactivate)
	{
		if (a_deactivate) {
			RevertTab(-1);
			// reset UI
			if (!menusAlreadyHidden && !RE::UI::GetSingleton()->IsShowingMenus()) {
				RE::UI::GetSingleton()->ShowMenus(true);
			}
			tabIndex = 0;
			doResetWindow = true;
		} else {
			RevertTab(resetAll ? -1 : tabIndex);

			const auto notification = fmt::format("{}", resetAll ? "$PM_ResetNotifAll"_T : TRANSLATE(tabEnumNotif[tabIndex]));
			RE::DebugNotification(notification.c_str());

			if (resetAll) {
				DoResetAll(false);
			}
		}
	}

	void Manager::RevertTab(std::int32_t a_tabIndex)
	{
		const bool resetAllTabs = a_tabIndex == -1;

		// Camera
		if (resetAllTabs || a_tabIndex == 0) {
			originalState.camera.set();
			// revert grid
			Grid::gridType = Grid::kDisabled;
			// revert DOF
			if (const auto& effect = RE::ImageSpaceManager::GetSingleton()->effects[RE::ImageSpaceManager::ImageSpaceEffectEnum::DepthOfField]) {
				static_cast<RE::ImageSpaceEffectDepthOfField*>(effect)->enabled = true;
			}
			// revert view roll
			currentState.camera.viewRoll = 0.0f;
		}
		// Time/Weather
		if (resetAllTabs || a_tabIndex == 1) {
			originalState.time.set();
			timescaleMult = 1.0f;
			// revert weather
			Override::weathers.ResetIndex();
			if (weatherForced) {
				Override::weathers.Revert();
				weatherForced = false;
			}
		}
		// Player
		if (resetAllTabs || a_tabIndex == 2) {
			originalState.player.set();
			// revert current values
			currentState.player.pos = RE::NiPoint3();
			if (!currentState.player.visible) {
				if (const auto root = RE::PlayerCharacter::GetSingleton()->Get3D()) {
					root->CullGeometry(false);
				}
				currentState.player.visible = true;
			}
			// reset expressions
			MFG::RevertAllModifiers();
			// revert idles
			Override::idles.ResetIndex();
			if (idlePlayed) {
				Override::idles.Revert();
				idlePlayed = false;
			}
			// revert effects
			Override::effectShaders.ResetIndex();
			if (effectsPlayed) {
				Override::effectShaders.Revert();
				effectsPlayed = false;
			}
			Override::effectVFX.ResetIndex();
			if (vfxPlayed) {
				Override::effectVFX.Revert();
				vfxPlayed = false;
			}
		}
		// Filters
		if (resetAllTabs || a_tabIndex == 3) {
			// reset imagespace
			const auto IMGS = RE::ImageSpaceManager::GetSingleton();
			if (resetAllTabs) {
				IMGS->overrideBaseData = nullptr;
			} else if (IMGS->overrideBaseData) {
				if (IMGS->currentBaseData) {
					imageSpaceData = *IMGS->currentBaseData;
				}
				imageSpaceData.tint.amount = 0.0f;
				imageSpaceData.tint.color = { 1.0f, 1.0f, 1.0f };
				IMGS->overrideBaseData = &imageSpaceData;
			}
			// reset imod
			Override::imods.ResetIndex();
			if (imodPlayed) {
				Override::imods.Revert();
				imodPlayed = false;
			}
		}
		// Screenshots
		if (a_tabIndex == 4) {
			Screenshot::Manager::GetSingleton()->Revert();
		}
	}

	float Manager::GetResetHoldDuration() const
	{
		return resetAllHoldDuration;
	}

	bool Manager::GetResetAll() const
	{
		return resetAll;
	}

	void Manager::DoResetAll(bool a_enable)
	{
		resetAll = a_enable;
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

	void Manager::CheckActive(RE::InputEvent* const* a_event)
	{
		IO.keyboard.Process(a_event);
		if (RE::BSInputDeviceManager::GetSingleton()->IsGamepadEnabled()) {
			IO.gamePad.Process(a_event);
		}
	}

	float Manager::GetViewRoll(const float a_fallback) const
	{
		return IsActive() ? currentState.camera.viewRoll : a_fallback;
	}

	void Manager::Draw()
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

		DrawControls();
		DrawBar();
		Grid::Draw();

		ImGui::End();
	}

	void Manager::OnFrameUpdate() const
	{
		RE::ControlMap::GetSingleton()->ignoreKeyboardMouse = ImGui::GetIO().WantTextInput;
		if (weatherForced) {
			RE::Sky::GetSingleton()->lastWeatherUpdate = RE::Calendar::GetSingleton()->gameHour->value;
		}
	}

	void Manager::DrawControls()
	{
		const auto viewport = ImGui::GetMainViewport();

		const static auto center = viewport->GetCenter();
		const static auto third_width = viewport->Size.x / 3;
		const static auto third_height = viewport->Size.y / 3;

		ImGui::SetNextWindowPos(ImVec2(center.x + third_width, center.y + third_height), ImGuiCond_Always, ImVec2(0.5, 0.5));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x / 3.5f, viewport->Size.y / 3.5f), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.66f);

		ImGui::Begin("$PM_Title"_T, nullptr, ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoCollapse);

		if (ImGui::BeginTabBar("PhotoMode##TopBar", ImGuiTabBarFlags_FittingPolicyScroll)) {
			if (doResetWindow) {
				ImGui::SetKeyboardFocusHere();
				doResetWindow = false;
			}

			if (ImGui::OpenTabOnHover("$PM_Camera"_T)) {
				tabIndex = 0;

				ImGui::EnumSlider("$PM_Grid"_T, &Grid::gridType, Grid::gridTypes);

				ImGui::Slider("$PM_FieldOfView"_T, &RE::PlayerCamera::GetSingleton()->worldFOV, 20.0f, 120.0f);
				ImGui::Slider("$PM_ViewRoll"_T, &currentState.camera.viewRoll, -2.0f, 2.0f);
				ImGui::Slider("$PM_TranslateSpeed"_T,
					&Cache::translateSpeedValue,  // fFreeCameraTranslationSpeed:Camera
					0.1f, 50.0f);

				if (const auto& effect = RE::ImageSpaceManager::GetSingleton()->effects[RE::ImageSpaceManager::ImageSpaceEffectEnum::DepthOfField]) {
					if (ImGui::TreeNode("$PM_DepthOfField"_T)) {
						const auto dofEffect = static_cast<RE::ImageSpaceEffectDepthOfField*>(effect);

						ImGui::OnOffToggle("$PM_DOF_Enabled"_T, &dofEffect->enabled, "$PM_YES"_T, "$PM_NO"_T);

						ImGui::BeginDisabled(!dofEffect->enabled);
						ImGui::Slider("$PM_DOF_Strength"_T, &Cache::DOF::blurMultiplier, 0.0f, 1.0f);
						ImGui::Slider("$PM_DOF_NearDistance"_T, &Cache::DOF::nearDist, 0.0f, 1000.0f);
						ImGui::Slider("$PM_DOF_NearRange"_T, &Cache::DOF::nearRange, 0.0f, 1000.0f);
						ImGui::Slider("$PM_DOF_FarDistance"_T, &Cache::DOF::farDist, 0.0f, 100000.0f);
						ImGui::Slider("$PM_DOF_FarRange"_T, &Cache::DOF::farRange, 0.0f, 100000.0f);
						ImGui::EndDisabled();

						ImGui::TreePop();
					}
				}

				ImGui::EndTabItem();
			}

			if (ImGui::OpenTabOnHover("$PM_TimeWeather"_T)) {
				tabIndex = 1;

				ImGui::OnOffToggle("$PM_FreezeTime"_T, &RE::Main::GetSingleton()->freezeTime, "$PM_YES"_T, "$PM_NO"_T);
				ImGui::Slider("$PM_GlobalTimeMult"_T, &RE::BSTimer::GetCurrentGlobalTimeMult(), 0.0f, 2.0f);

				ImGui::Dummy({ 0, 5 });

				auto& gameHour = RE::Calendar::GetSingleton()->gameHour->value;
				ImGui::Slider("$PM_GameHour"_T, &gameHour, 0.0f, 23.99f, std::format("{:%I:%M %p}", std::chrono::duration<float, std::ratio<3600>>(gameHour)).c_str());

				if (ImGui::DragOnHover("$PM_TimeScaleMult"_T, &timescaleMult, 10, 1.0f, 1000.0f, "%.fX")) {
					RE::Calendar::GetSingleton()->timeScale->value = originalState.time.timescale * timescaleMult;
				}

				ImGui::Dummy({ 0, 15 });

				if (const auto weather = Override::weathers.GetFormResultFromCombo()) {
					Override::weathers.Apply(weather);
					weatherForced = true;
				}

				ImGui::EndTabItem();
			}

			if (ImGui::OpenTabOnHover("$PM_Player"_T)) {
				tabIndex = 2;

				static auto player = RE::PlayerCharacter::GetSingleton();
				auto&       playerState = currentState.player;

				if (ImGui::OnOffToggle("$PM_ShowPlayer"_T, &playerState.visible, "$PM_YES"_T, "$PM_NO"_T)) {
					player->Get3D()->CullGeometry(!playerState.visible);
				}

				ImGui::BeginDisabled(!playerState.visible);
				if (ImGui::BeginTabBar("Player#TopBar", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
					// ugly af, improve later
					if (ImGui::OpenTabOnHover("$PM_Expressions"_T)) {
						using namespace MFG;

						if (ImGui::TreeNode("$PM_ExpressionsNode"_T)) {
							if (ImGui::EnumSlider("$PM_Expression"_T, &expressionData.modifier, expressions)) {
								expressionData.ApplyExpression(player);
							}
							ImGui::TreePop();
						}

						if (ImGui::TreeNode("$PM_Phoneme"_T)) {
							for (std::uint32_t i = 0; i < phonemes.size(); i++) {
								if (ImGui::DragOnHover(phonemes[i], &phonemeData[i].strength)) {
									phonemeData[i].ApplyPhenome(i, player);
								}
							}
							ImGui::TreePop();
						}

						if (ImGui::TreeNode("$PM_Modifier"_T)) {
							for (std::uint32_t i = 0; i < modifiers.size(); i++) {
								if (ImGui::DragOnHover(modifiers[i], &modifierData[i].strength)) {
									modifierData[i].ApplyModifier(i, player);
								}
							}
							ImGui::TreePop();
						}
						ImGui::EndTabItem();
					}

					if (ImGui::OpenTabOnHover("$PM_Poses"_T)) {
						if (const auto idle = Override::idles.GetFormResultFromCombo()) {
							if (idlePlayed) {
								Override::idles.Revert();
							}
							Override::idles.Apply(idle);
							idlePlayed = true;
						}
						ImGui::EndTabItem();
					}

					if (ImGui::OpenTabOnHover("$PM_Effects"_T)) {
						if (const auto effectShader = Override::effectShaders.GetFormResultFromCombo()) {
							Override::effectShaders.Apply(effectShader);
							effectsPlayed = true;
						}
						if (const auto vfx = Override::effectVFX.GetFormResultFromCombo()) {
							Override::effectVFX.Apply(vfx);
							vfxPlayed = true;
						}
						ImGui::EndTabItem();
					}

					if (ImGui::OpenTabOnHover("$PM_Transforms"_T)) {
						playerState.rotZ = RE::rad_to_deg(player->GetAngleZ());
						if (ImGui::Slider("$PM_Rotation"_T, &playerState.rotZ, 0.0f, 360.0f)) {
							player->SetRotationZ(RE::deg_to_rad(playerState.rotZ));
						}

						bool update = ImGui::Slider("$PM_PositionLeftRight"_T, &playerState.pos.x, -100.0f, 100.0f);
						update |= ImGui::Slider("$PM_PositionNearFar"_T, &playerState.pos.y, -100.0f, 100.0f);
						// update |= ImGui::Slider("Elevation", &playerState.pos.z, -100.0f, 100.0f);

						if (update) {
							player->SetPosition({ originalState.player.pos + playerState.pos }, true);
						}

						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::EndDisabled();
				ImGui::EndTabItem();
			}

			if (ImGui::OpenTabOnHover("$PM_Filters"_T)) {
				tabIndex = 3;

				if (const auto imageSpace = Override::imods.GetFormResultFromCombo()) {
					Override::imods.Apply(imageSpace);
					imodPlayed = true;
				}

				ImGui::Dummy({ 0, 15 });

				if (const auto& overrideData = RE::ImageSpaceManager::GetSingleton()->overrideBaseData) {
					ImGui::Slider("$PM_Brightness"_T, &overrideData->cinematic.brightness, 0.0f, 3.0f);
					ImGui::Slider("$PM_Saturation"_T, &overrideData->cinematic.saturation, 0.0f, 3.0f);
					ImGui::Slider("$PM_Contrast"_T, &overrideData->cinematic.contrast, 0.0f, 3.0f);

					if (ImGui::TreeNode("$PM_Tint"_T)) {
						ImGui::Slider("$PM_TintAlpha"_T, &overrideData->tint.amount, 0.0f, 1.0f);
						ImGui::Slider("$PM_TintRed"_T, &overrideData->tint.color.red, 0.0f, 1.0f);
						ImGui::Slider("$PM_TintBlue"_T, &overrideData->tint.color.blue, 0.0f, 1.0f);
						ImGui::Slider("$PM_TintGreen"_T, &overrideData->tint.color.green, 0.0f, 1.0f);

						ImGui::TreePop();
					}
				} else {
					RE::ImageSpaceManager::GetSingleton()->overrideBaseData = &imageSpaceData;
				}

				ImGui::EndTabItem();
			}

			if (ImGui::OpenTabOnHover("$PM_Screenshots"_T)) {
				tabIndex = 4;

				Screenshot::Manager::GetSingleton()->Draw();
				ImGui::EndTabItem();
			}

			ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
			ImGui::TabItemButton("<", ImGuiTabItemFlags_Leading);
			ImGui::TabItemButton(">", ImGuiTabItemFlags_Trailing);
			ImGui::PopItemFlag();

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	void Manager::DrawBar() const
	{
		const auto viewport = ImGui::GetMainViewport();

		const static auto center = viewport->GetCenter();
		const static auto offset = viewport->Size.y / 20;

		ImGui::SetNextWindowPos(ImVec2(center.x, (center.y + viewport->Size.y / 2) - offset), ImGuiCond_Always, ImVec2(0.5, 0.25));
		ImGui::SetNextWindowBgAlpha(0.66f);

		ImGui::BeginChild("##Bar", ImVec2(viewport->Size.x / 4.0f, offset * 0.75f), false, ImGuiWindowFlags_NoBringToFrontOnFocus);
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
			float             width = 0.0f;

			width += takePhotoIcon->width;
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(takePhotoLabel).x;
			width += style.ItemSpacing.x;

			width += toggleUIIcon->width;
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(toggleUILabel).x;
			width += style.ItemSpacing.x;

			width += resetIcon->width;
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(resetLabel).x;
			width += style.ItemSpacing.x;

			for (const auto& icon : exitIcons) {
				width += icon->width;
			}
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(exitLabel).x;

			// align at center
			ImGui::AlignForWidth(width);

			// draw
			ImGui::ButtonIconWithLabel(takePhotoLabel, takePhotoIcon);
			ImGui::SameLine();

			ImGui::ButtonIconWithLabel(toggleUILabel, toggleUIIcon);
			ImGui::SameLine();

			ImGui::ButtonIconWithLabel(resetLabel, resetIcon);
			ImGui::SameLine();

			ImGui::ButtonIconWithLabel(exitLabel, exitIcons);
		}
		ImGui::EndChild();
	}

	struct FromEulerAnglesZXY
	{
		static void thunk(RE::NiMatrix3* a_matrix, float a_z, float a_x, float a_y)
		{
			return func(a_matrix, a_z, a_x, Manager::GetSingleton()->GetViewRoll(a_y));
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHook()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(49814, 50744), 0x1B };  // FreeCamera::GetRotation
		stl::write_thunk_call<FromEulerAnglesZXY>(target.address());

		Override::InstallHooks();
	}
}
