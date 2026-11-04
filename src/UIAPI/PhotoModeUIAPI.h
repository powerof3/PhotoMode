#pragma once

// lets another plugin replace the ImGui panel, button bar and gallery with its own frontend:
// the tab code still runs, its widget calls are recorded and edits come back by widget id.
// pointers are only valid during the callback, without a frontend nothing changes

#include <cstdint>

namespace RE
{
	class InputEvent;
}

namespace PhotoModeAPI
{
	enum class InterfaceVersion : std::uint32_t
	{
		kV1
	};

	// extern "C" __declspec(dllexport) IPhotoMode* RequestPhotoModeUIAPI(InterfaceVersion);
	// nullptr if the version isn't supported
	inline constexpr auto exportName = "RequestPhotoModeUIAPI";

	using WidgetID = std::uint32_t;  // ImGuiID, stable for the session

	enum class WidgetType : std::uint8_t
	{
		kSlider,    // value/min/max
		kDrag,      // DragOnHover, step = drag speed
		kCheckBox,  // value 0/1
		kEnum,      // value = index into options
		kButton,
		kCombo,     // ComboWithFilter, value = index, items via GetOptions()
		kSubTab,    // value 1 = selected
		kTreeNode,  // value 1 = open
		kHeader,
		kText
	};

	enum WidgetFlag : std::uint32_t
	{
		kNone = 0,
		kDisabled = 1 << 0,
		kInteger = 1 << 1,
		kNoInput = 1 << 2,       // no text entry
		kHasFavorites = 1 << 3,  // kCombo supports ToggleFavorite()
		kSameLine = 1 << 4       // same row as the previous widget
	};

	struct Widget
	{
		WidgetID           id;
		WidgetID           parent;  // enclosing kSubTab/kTreeNode, 0 = tab root
		WidgetType         type;
		std::uint8_t       indent;
		std::uint32_t      flags;
		const char*        label;  // translated
		const char*        text;   // formatted value / enum entry / combo preview
		float              value;
		float              min;
		float              max;
		float              step;
		const char* const* options;  // kEnum only
		std::uint32_t      optionCount;
	};

	enum class Action : std::uint32_t
	{
		kTakePhoto,
		kToggleMenus,
		kFreezeTime,
		kReset,
		kResetAll,
		kNextTab,
		kPreviousTab,
		kClose,
		kPanCamera  // hint only, no key while the mouse can't pan
	};

	enum class InputDevice : std::uint8_t
	{
		kNone,
		kKeyboard,
		kMouse,
		kGamepadDirectX,
		kGamepadOrbis
	};

	struct FrameInfo
	{
		const char* const* tabs;
		std::uint32_t      tabCount;
		std::uint32_t      currentTab;
		const char*        title;  // tab name, character name on the player tab
		const Widget*      widgets;
		std::uint32_t      widgetCount;
		InputDevice        inputDevice;
		bool               hidden;    // ToggleMenus
		bool               resetAll;  // reset key held
		bool               blocked;   // console open
	};

	enum ReplaceFlag : std::uint32_t
	{
		kReplaceControls = 1 << 0,  // tab panel, cursor/camera/hotkeys stay with PhotoMode
		kReplaceButtonBar = 1 << 1,
		kReplaceGallery = 1 << 2,  // hotkeys and thumbnail streaming stay with PhotoMode
		kWantTextInput = 1 << 3    // while typing: pauses hotkeys and game controls
	};

	enum class GalleryAction : std::uint32_t
	{
		kEnlarge,  // also confirms a delete
		kToggleLoadScreen,
		kDelete,  // asks for confirmation first
		kBack,    // Escape
		kToggleUI,
		kPrevious,
		kNext
	};

	enum class LoadScreenState : std::uint8_t
	{
		kNone,  // no loadscreen image
		kShown,
		kExcluded
	};

	struct GalleryPhoto
	{
		const char*     name;
		const char*     path;       // PNG, UTF-8
		const char*     thumbnail;  // nullptr until streamed
		float           aspectRatio;
		LoadScreenState loadScreen;
	};

	struct GalleryInfo
	{
		const GalleryPhoto* photos;      // visible rows only, the first is photo firstVisibleRow * columns
		std::uint32_t       photoCount;  // of photos
		std::uint32_t       totalPhotos;
		const GalleryPhoto* enlargedPhoto;  // nullptr in the grid
		std::int32_t        selected;       // -1 = none
		std::int32_t        enlarged;       // -1 = grid
		std::uint32_t       columns;
		std::uint32_t       visibleRows;
		std::int32_t        firstVisibleRow;
		float               gridAspectRatio;
		InputDevice         inputDevice;
		bool                confirmDelete;
		bool                hideUI;
	};

	struct IOptionSink
	{
		virtual void Add(std::uint32_t a_index, const char* a_text, bool a_favorite) = 0;

	protected:
		~IOptionSink() = default;
	};

	// implemented by the frontend, called on the render thread
	struct IFrontend
	{
		virtual std::uint32_t GetReplaceFlags() const = 0;
		virtual bool          IsCursorOverUI() const = 0;  // blocks camera tilt/pan
		virtual void          OnActivate() = 0;
		virtual void          OnDeactivate() = 0;
		virtual void          OnFrame(const FrameInfo& a_frame) = 0;

		// kReplaceGallery
		virtual void OnGalleryActivate() = 0;
		virtual void OnGalleryDeactivate() = 0;
		virtual void OnGalleryFrame(const GalleryInfo& a_gallery) = 0;
		virtual void OnGalleryInput(RE::InputEvent* const* a_events) = 0;  // other sinks get none while the gallery is open

	protected:
		~IFrontend() = default;
	};

	// implemented by PhotoMode
	struct IPhotoMode
	{
		virtual bool RegisterFrontend(IFrontend* a_frontend) = 0;  // false if one is already registered
		virtual void UnregisterFrontend(IFrontend* a_frontend) = 0;

		// queued, applied in the next pass
		virtual void SetValue(WidgetID a_id, float a_value) = 0;
		virtual void Press(WidgetID a_id) = 0;
		virtual void ToggleFavorite(WidgetID a_combo, std::uint32_t a_index) = 0;
		virtual void SelectTab(std::uint32_t a_tab) = 0;
		virtual void DoAction(Action a_action) = 0;

		// only valid inside IFrontend::OnFrame
		virtual void GetOptions(WidgetID a_combo, IOptionSink& a_sink) const = 0;

		// SKSE key code for the current input device
		virtual std::uint32_t GetActionKey(Action a_action) const = 0;

		// "$PM_..." key
		virtual const char* Translate(const char* a_key) const = 0;

		// gallery, queued
		virtual void          SelectPhoto(std::int32_t a_index, bool a_enlarge) = 0;
		virtual void          ScrollGallery(std::int32_t a_firstRow) = 0;
		virtual void          GalleryDoAction(GalleryAction a_action) = 0;
		virtual std::uint32_t GetGalleryActionKey(GalleryAction a_action) const = 0;

	protected:
		~IPhotoMode() = default;
	};
}
