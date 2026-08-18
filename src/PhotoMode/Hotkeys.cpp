#include "Hotkeys.h"

#include "Gallery/Manager.h"
#include "ImGui/IconsFonts.h"
#include "Input.h"
#include "Manager.h"

namespace PhotoMode::Hotkeys
{
	void Manager::LoadHotKeys(const CSimpleIniA& a_ini)
	{
		togglePhotoMode.LoadKeys(a_ini, "iOpenPhotoMode");
		toggleGallery.LoadKeys(a_ini, "iOpenGallery");

		reset.LoadKeys(a_ini, "iReset");
		takePhoto.LoadKeys(a_ini, "iTakePhoto");
		toggleMenus.LoadKeys(a_ini, "iToggleMenus");
		nextTab.LoadKeys(a_ini, "iNextTab");
		previousTab.LoadKeys(a_ini, "iPreviousTab");
		freezeTime.LoadKeys(a_ini, "iFreezeTime");
		panCamera.LoadKeys(a_ini, "iPanCamera");
		galleryEnlarge.LoadKeys(a_ini, "iGalleryEnlarge");
		galleryDelete.LoadKeys(a_ini, "iGalleryDelete");
		galleryLoadScreen.LoadKeys(a_ini, "iGalleryLoadScreen");
	}

	void Manager::TogglePhotoMode(RE::InputEvent* const* a_event)
	{
		if (togglePhotoMode.IsInvalid()) {
			return;
		}

		togglePhotoMode.ProcessKeyPress(a_event, []() {
			MANAGER(PhotoMode)->ToggleActive();
		});
	}

	void Manager::ToggleGallery(RE::InputEvent* const* a_event)
	{
		if (toggleGallery.IsInvalid()) {
			return;
		}

		toggleGallery.ProcessKeyPress(a_event, []() {
			MANAGER(Gallery)->ToggleActive();
		});
	}

	void Manager::Key::LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting)
	{
		keyboard = a_ini.GetLongValue("Controls", std::format("{}Key", a_setting).c_str(), keyboard);
		gamePad = a_ini.GetLongValue("Controls", std::format("{}GamePad", a_setting).c_str(), gamePad);
	}

	std::uint32_t Manager::Key::GetKey() const
	{
		return MANAGER(Input)->IsInputKBM() ? keyboard : gamePad;
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
		modifier = a_ini.GetLongValue("Controls", std::format("{}Modifier", a_setting).c_str(), modifier);

		keys.clear();
		if (primary != -1) {
			keys.insert(primary);
		}
		if (modifier != -1) {
			keys.insert(modifier);
		}
	}

	void Manager::KeyCombo::LoadKeys(const CSimpleIniA& a_ini, std::string_view a_settingPrefix)
	{
		keyboard.LoadKeys(a_ini, std::format("{}Key", a_settingPrefix));
		gamePad.LoadKeys(a_ini, std::format("{}GamePad", a_settingPrefix));
	}

	bool Manager::KeyCombo::IsInvalid() const
	{
		return keyboard.keys.empty() && gamePad.keys.empty();
	}

	std::set<std::uint32_t> Manager::KeyCombo::GetKeys() const
	{
		auto device = MANAGER(Input)->GetInputDevice();
		return (device == Input::DEVICE::kKeyboard || device == Input::DEVICE::kMouse) ? keyboard.keys : gamePad.keys;
	}

	bool Manager::KeyCombo::ProcessKeyPress(RE::InputEvent* const* a_event, std::function<void()> a_callback)
	{
		std::set<std::uint32_t> pressed;

		for (auto event = *a_event; event; event = event->next) {
			const auto button = event->AsButtonEvent();
			if (!button || !button->HasIDCode()) {
				continue;
			}
			if (button->IsPressed()) {
				auto key = button->GetIDCode();
				switch (button->GetDevice()) {
				case RE::INPUT_DEVICE::kKeyboard:
					break;
				case RE::INPUT_DEVICE::kMouse:
					key += SKSE::InputMap::kMacro_MouseButtonOffset;
					break;
				case RE::INPUT_DEVICE::kGamepad:
					key = SKSE::InputMap::GamepadMaskToKeycode(key);
					break;
				default:
					continue;
				}
				pressed.insert(key);
			}
		}

		if (!pressed.empty() && (pressed == keyboard.keys || pressed == gamePad.keys)) {
			if (!triggered) {
				triggered = true;
				a_callback();
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

	std::uint32_t Manager::ToggleMenusKey() const
	{
		return toggleMenus.GetKey();
	}

	std::uint32_t Manager::NextTabKey() const
	{
		return nextTab.GetKey();
	}

	std::uint32_t Manager::PreviousTabKey() const
	{
		return previousTab.GetKey();
	}

	std::uint32_t Manager::FreezeTimeKey() const
	{
		return freezeTime.GetKey();
	}

	std::uint32_t Manager::PanCameraKey() const
	{
		return panCamera.GetKey();
	}

	std::uint32_t Manager::GalleryEnlargeKey() const
	{
		return galleryEnlarge.GetKey();
	}

	std::uint32_t Manager::GalleryDeleteKey() const
	{
		return galleryDelete.GetKey();
	}

	std::uint32_t Manager::GalleryLoadScreenKey() const
	{
		return galleryLoadScreen.GetKey();
	}

	std::uint32_t Manager::EscapeKey()
	{
		return MANAGER(Input)->IsInputKBM() ? KEY::kEscape : SKSE::InputMap::kGamepadButtonOffset_B;
	}

	const IconFont::IconTexture* Manager::ResetIcon() const
	{
		return MANAGER(IconFont)->GetIcon(reset.GetKey());
	}

	const IconFont::IconTexture* Manager::TakePhotoIcon() const
	{
		return MANAGER(IconFont)->GetIcon(takePhoto.GetKey());
	}

	const IconFont::IconTexture* Manager::ToggleMenusIcon() const
	{
		return MANAGER(IconFont)->GetIcon(toggleMenus.GetKey());
	}

	const IconFont::IconTexture* Manager::NextTabIcon() const
	{
		return MANAGER(IconFont)->GetIcon(nextTab.GetKey());
	}

	const IconFont::IconTexture* Manager::PreviousTabIcon() const
	{
		return MANAGER(IconFont)->GetIcon(previousTab.GetKey());
	}

	const IconFont::IconTexture* Manager::FreezeTimeIcon() const
	{
		return MANAGER(IconFont)->GetIcon(freezeTime.GetKey());
	}

	const IconFont::IconTexture* Manager::GalleryEnlargeIcon() const
	{
		return MANAGER(IconFont)->GetIcon(galleryEnlarge.GetKey());
	}

	const IconFont::IconTexture* Manager::GalleryDeleteIcon() const
	{
		return MANAGER(IconFont)->GetIcon(galleryDelete.GetKey());
	}

	const IconFont::IconTexture* Manager::GalleryLoadScreenIcon() const
	{
		return MANAGER(IconFont)->GetIcon(galleryLoadScreen.GetKey());
	}

	const IconFont::IconTexture* Manager::PanCameraIcon() const
	{
		return MANAGER(IconFont)->GetIcon(panCamera.GetKey());
	}

	const IconFont::IconTexture* Manager::EscapeIcon()
	{
		return MANAGER(IconFont)->GetIcon(EscapeKey());
	}

	std::set<const IconFont::IconTexture*> Manager::TogglePhotoModeIcons() const
	{
		return MANAGER(IconFont)->GetIcons(togglePhotoMode.GetKeys());
	}

	std::set<const IconFont::IconTexture*> Manager::ToggleGalleryIcons() const
	{
		return MANAGER(IconFont)->GetIcons(toggleGallery.GetKeys());
	}
}
