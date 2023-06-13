#include "Hotkeys.h"
#include "Input.h"
#include "Manager.h"

namespace PhotoMode::Hotkeys
{
	void Manager::LoadHotKeys(const CSimpleIniA& a_ini)
	{
		openPhotoModeCombo.LoadKeys(a_ini);

		reset.LoadKeys(a_ini, "iReset");
		takePhoto.LoadKeys(a_ini, "iTakePhoto");
		toggleUI.LoadKeys(a_ini, "iToggleUI");
		nextTab.LoadKeys(a_ini, "iNextTab");
		previousTab.LoadKeys(a_ini, "iPreviousTab");
	}

    const std::set<std::uint32_t>& Manager::TogglePhotoModeKeys() const
    {
		return openPhotoModeCombo.GetKeys();
	}

    void Manager::TogglePhotoMode(RE::InputEvent* const* a_event)
	{
		openPhotoModeCombo.ProcessKeyPress(a_event);
	}

    void Manager::Key::LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting)
	{
		keyboard = a_ini.GetLongValue("Controls", fmt::format("{}Key", a_setting).c_str(), keyboard);
		gamePad = a_ini.GetLongValue("Controls", fmt::format("{}GamePad", a_setting).c_str(), gamePad);
	}

	std::uint32_t Manager::Key::GetKey() const
	{
		if (Input::inputType == Input::TYPE::kKeyboard) {
			return keyboard;
		}
		return gamePad;
	}

	std::uint32_t Manager::Key::Keyboard() const
	{
		return keyboard;
	}

	std::uint32_t Manager::Key::GamePad() const
	{
		return gamePad;
	}

	void Manager::KeyCombo::KeyComboImpl::LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting)
	{
		primary = a_ini.GetLongValue("Controls", a_setting.data(), primary);
		modifier = a_ini.GetLongValue("Controls", fmt::format("{}Modifier", a_setting).c_str(), modifier);

		keys.clear();
		if (primary != -1) {
			keys.insert(primary);
		}
		if (modifier != -1) {
			keys.insert(modifier);
		}
	}

	void Manager::KeyCombo::LoadKeys(const CSimpleIniA& a_ini)
	{
		keyboard.LoadKeys(a_ini, "iOpenPhotoModeKey");
		gamePad.LoadKeys(a_ini, "iOpenPhotoModeGamePad");
	}

    const std::set<std::uint32_t>& Manager::KeyCombo::GetKeys() const
	{
		if (Input::inputType == Input::TYPE::kKeyboard) {
			return keyboard.keys;
		}
		return gamePad.keys;
	}

    bool Manager::KeyCombo::ProcessKeyPress(RE::InputEvent* const* a_event)
	{
		std::set<std::uint32_t> pressed;

		for (auto event = *a_event; event; event = event->next) {
			const auto button = event->AsButtonEvent();
			if (!button || !button->HasIDCode()) {
				continue;
			}

			auto key = button->GetIDCode();

			switch (button->GetDevice()) {
			case RE::INPUT_DEVICE::kGamepad:
				key = SKSE::InputMap::GamepadMaskToKeycode(key);
				break;
			default:
				break;
			}

			if (button->IsPressed()) {
				pressed.insert(key);
			}
		}

		if (pressed == keyboard.keys || pressed == gamePad.keys) {
			if (!triggered) {
				triggered = true;
				MANAGER(PhotoMode)->ToggleActive();
			}
		} else {
			triggered = false;
		}

		return triggered;
	}

	std::uint32_t Manager::ResetKey() const
	{
		return reset.GetKey();
	}

	std::uint32_t Manager::TakePhotoKey() const
	{
		return takePhoto.GetKey();
	}

	std::uint32_t Manager::ToggleUIKey() const
	{
		return toggleUI.GetKey();
	}

	std::uint32_t Manager::NextTabKey() const
	{
		return nextTab.GetKey();
	}

	std::uint32_t Manager::PreviousTabKey() const
	{
		return previousTab.GetKey();
	}
}
