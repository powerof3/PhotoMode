#pragma once

namespace IconFont
{
	struct IconData;
}

namespace PhotoMode::Hotkeys
{
	class Manager : public ISingleton<Manager>
	{
	public:
		void LoadHotKeys(const CSimpleIniA& a_ini);

		void TogglePhotoMode(RE::InputEvent* const* a_event);

		std::uint32_t        ResetKey() const;
		std::uint32_t        TakePhotoKey() const;
		std::uint32_t        ToggleMenusKey() const;
		std::uint32_t        NextTabKey() const;
		std::uint32_t        PreviousTabKey() const;
		static std::uint32_t EscapeKey();

		const IconFont::IconData* ResetIcon() const;
		const IconFont::IconData* TakePhotoIcon() const;
		const IconFont::IconData* ToggleMenusIcon() const;
		const IconFont::IconData* NextTabIcon() const;
		const IconFont::IconData* PreviousTabIcon() const;

		std::set<const IconFont::IconData*> TogglePhotoModeIcons() const;

	private:
		struct Key
		{
			void          LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting);
			std::uint32_t GetKey() const;

			std::uint32_t Keyboard() const;
			std::uint32_t GamePad() const;

		private:
			std::uint32_t keyboard{ 0 };
			std::uint32_t gamePad{ 0 };
		};

		struct KeyCombo
		{
			void LoadKeys(const CSimpleIniA& a_ini);

			bool                    IsInvalid() const;
			std::set<std::uint32_t> GetKeys() const;

			bool ProcessKeyPress(RE::InputEvent* const* a_event, std::function<void()> a_callback);

		private:
			struct KeyComboImpl
			{
				void LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting);

				std::int32_t primary{ -1 };
				std::int32_t modifier{ -1 };

				std::set<std::uint32_t> keys{};
			};

			KeyComboImpl keyboard;
			KeyComboImpl gamePad;

			bool triggered{ false };
		} togglePhotoMode;

		Key nextTab;
		Key previousTab;
		Key takePhoto;
		Key toggleMenus;
		Key reset;
	};
}
namespace Hotkeys = PhotoMode::Hotkeys;
