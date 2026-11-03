#include "Manager.h"

#include "Gallery/Manager.h"
#include "Input.h"
#include "MenuIntegration.h"
#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"

namespace UIAPI
{
	bool Manager::IsReplaced(std::uint32_t a_flag) const
	{
		return frontend && (frontend->GetReplaceFlags() & a_flag) != 0;
	}

	void Manager::OnActivate() const
	{
		if (frontend) {
			frontend->OnActivate();
		}
	}

	void Manager::OnDeactivate()
	{
		{
			std::scoped_lock lock(queueLock);
			queuedValues.clear();
			queuedPresses.clear();
			queuedFavorites.clear();
			queuedTasks.clear();
		}

		if (frontend) {
			frontend->OnDeactivate();
		}
	}

	bool Manager::CheckBox(const char* a_label, bool* a_toggle)
	{
		const auto id = ImGui::GetID(a_label);

		bool changed = false;
		if (const auto value = PopValue(id)) {
			*a_toggle = *value != 0.0f;
			changed = true;
		}

		auto& entry = Add(id, WidgetType::kCheckBox, a_label, {}, kNone);
		entry.value = *a_toggle ? 1.0f : 0.0f;
		entry.max = 1.0f;

		return changed;
	}

	bool Manager::Button(const char* a_label)
	{
		const auto id = ImGui::GetID(a_label);

		Add(id, WidgetType::kButton, a_label, {}, kNone);

		return PopPress(id);
	}

	bool Manager::Combo(const char* a_label, int* a_currentItem, const std::vector<std::string>& a_items, const StringSet* a_favorites, std::string* a_favToggled)
	{
		const auto id = ImGui::GetID(a_label);
		const auto count = static_cast<int>(a_items.size());

		bool changed = false;
		if (const auto value = PopValue(id); value && static_cast<int>(*value) >= 0 && static_cast<int>(*value) < count) {
			*a_currentItem = static_cast<int>(*value);
			changed = true;
		}

		if (a_favorites && a_favToggled) {
			std::scoped_lock lock(queueLock);
			if (const auto it = queuedFavorites.find(id); it != queuedFavorites.end()) {
				if (it->second < a_items.size()) {
					*a_favToggled = a_items[it->second];
				}
				queuedFavorites.erase(it);
			}
		}

		const bool validItem = *a_currentItem >= 0 && *a_currentItem < count;

		auto& entry = Add(id, WidgetType::kCombo, a_label, validItem ? a_items[*a_currentItem] : std::string_view{}, a_favorites ? kHasFavorites : kNone);
		entry.value = static_cast<float>(*a_currentItem);
		entry.min = -1.0f;
		entry.max = static_cast<float>(count - 1);
		entry.items = &a_items;
		entry.favorites = a_favorites;

		return changed;
	}

	bool Manager::TabItem(const char* a_label, ImGuiTabItemFlags a_flags)
	{
		const auto id = ImGui::GetID(a_label);

		if (const auto value = PopValue(id); value && *value != 0.0f) {
			a_flags |= ImGuiTabItemFlags_SetSelected;
		}

		// real tab item, so EndTabItem/EndTabBar stay balanced
		const bool selected = ImGui::BeginTabItem(a_label, nullptr, a_flags);
		containers.insert_or_assign(GImGui->LastItemData.ID, id);

		auto& entry = Add(id, WidgetType::kSubTab, a_label, {}, kNone);
		entry.value = selected ? 1.0f : 0.0f;
		entry.max = 1.0f;

		return selected;
	}

	bool Manager::TreeNode(const char* a_label)
	{
		const auto id = ImGui::GetID(a_label);

		if (const auto value = PopValue(id)) {
			ImGui::SetNextItemOpen(*value != 0.0f);
		}

		// real tree node, so TreePop stays balanced
		const bool open = ImGui::TreeNode(a_label);
		containers.insert_or_assign(id, id);

		auto& entry = Add(id, WidgetType::kTreeNode, a_label, {}, kNone);
		entry.value = open ? 1.0f : 0.0f;
		entry.max = 1.0f;

		return open;
	}

	void Manager::Header(const char* a_label)
	{
		Add(ImGui::GetID(a_label), WidgetType::kHeader, a_label, {}, kNone);
	}

	void Manager::Text(const char* a_label)
	{
		Add(ImGui::GetID(a_label), WidgetType::kText, a_label, {}, kNone);
	}

	bool Manager::RegisterFrontend(IFrontend* a_frontend)
	{
		if (frontend || !a_frontend) {
			return false;
		}
		frontend = a_frontend;
		REX::INFO("UI frontend registered (flags: {:X})", frontend->GetReplaceFlags());
		return true;
	}

	void Manager::UnregisterFrontend(IFrontend* a_frontend)
	{
		if (frontend == a_frontend) {
			frontend = nullptr;
			REX::INFO("UI frontend unregistered");
		}
	}

	void Manager::SetValue(WidgetID a_id, float a_value)
	{
		std::scoped_lock lock(queueLock);
		queuedValues.insert_or_assign(a_id, a_value);
	}

	void Manager::Press(WidgetID a_id)
	{
		std::scoped_lock lock(queueLock);
		queuedPresses.insert(a_id);
	}

	void Manager::ToggleFavorite(WidgetID a_combo, std::uint32_t a_index)
	{
		std::scoped_lock lock(queueLock);
		queuedFavorites.insert_or_assign(a_combo, a_index);
	}

	void Manager::SelectTab(std::uint32_t a_tab)
	{
		Queue([a_tab] {
			const auto photoMode = MANAGER(PhotoMode);
			if (a_tab < PhotoMode::Manager::tabs.size() && static_cast<std::int32_t>(a_tab) != photoMode->currentTab) {
				photoMode->currentTab = static_cast<std::int32_t>(a_tab);
				photoMode->UpdateKeyboardFocus();
				RE::PlaySound("UIJournalTabsSD");
			}
		});
	}

	void Manager::DoAction(Action a_action)
	{
		Queue([a_action] {
			const auto photoMode = MANAGER(PhotoMode);
			switch (a_action) {
			case Action::kTakePhoto:
				MANAGER(Input)->QueueScreenshot(true);
				break;
			case Action::kToggleMenus:
				photoMode->ToggleUI();
				break;
			case Action::kFreezeTime:
				RE::Main::GetSingleton()->freezeTime = !RE::Main::GetSingleton()->freezeTime;
				break;
			case Action::kReset:
				photoMode->Revert(false);
				break;
			case Action::kResetAll:
				photoMode->DoResetAll();
				photoMode->Revert(false);
				break;
			case Action::kNextTab:
				photoMode->NavigateTab(false);
				break;
			case Action::kPreviousTab:
				photoMode->NavigateTab(true);
				break;
			case Action::kClose:
				SKSE::GetTaskInterface()->AddTask([]() { MANAGER(PhotoMode)->Deactivate(); });
				break;
			default:
				break;
			}
		});
	}

	void Manager::GetOptions(WidgetID a_combo, IOptionSink& a_sink) const
	{
		const auto it = std::ranges::find(entries, a_combo, &Entry::id);
		if (it == entries.end() || !it->items) {
			return;
		}

		for (std::uint32_t i = 0; i < it->items->size(); i++) {
			const auto& item = (*it->items)[i];
			a_sink.Add(i, item.c_str(), it->favorites && it->favorites->contains(item));
		}
	}

	std::uint32_t Manager::GetActionKey(Action a_action) const
	{
		const auto hotKeys = MANAGER(PhotoMode::Hotkeys);

		switch (a_action) {
		case Action::kTakePhoto:
			return hotKeys->TakePhotoKey();
		case Action::kToggleMenus:
			return hotKeys->ToggleMenusKey();
		case Action::kFreezeTime:
			return hotKeys->FreezeTimeKey();
		case Action::kReset:
		case Action::kResetAll:
			return hotKeys->ResetKey();
		case Action::kNextTab:
			return hotKeys->NextTabKey();
		case Action::kPreviousTab:
			return hotKeys->PreviousTabKey();
		case Action::kClose:
			return PhotoMode::Hotkeys::Manager::EscapeKey();
		case Action::kPanCamera:
			return MANAGER(Input)->CanNavigateWithMouse() ? hotKeys->PanCameraKey() : 0;
		default:
			return 0;
		}
	}

	void Manager::BeginRecord()
	{
		RunQueued();

		if (IsReplaced(kWantTextInput)) {
			ImGui::GetCurrentContext()->PlatformImeData.WantTextInput = true;  // same as an active ImGui::InputText, EndFrame() turns it into io.WantTextInput
		}

		entries.clear();
		containers.clear();

		// not 0: Begin() skips the items of windows with alpha 0, which hides native containers like BeginTabBar
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, FLT_MIN);
		recording = true;
	}

	void Manager::EndRecord()
	{
		recording = false;
		ImGui::PopStyleVar();

		if (!frontend) {
			return;
		}

		const auto photoMode = MANAGER(PhotoMode);

		if (tabNames.empty()) {
			for (const auto& tab : PhotoMode::Manager::tabs) {
				tabNames.emplace_back(TRANSLATE(tab));
			}
			for (const auto& tabName : tabNames) {
				tabs.push_back(tabName.c_str());
			}
		}

		const auto currentTab = static_cast<std::uint32_t>(photoMode->currentTab);
		if (currentTab == PhotoMode::Manager::kCharacter && photoMode->cachedCharacter) {
			title = photoMode->characterTab[photoMode->cachedCharacter->GetFormID()].GetName();
		} else {
			title = tabNames[currentTab];
		}

		// all enum options in one array, so pointers stay valid once it's filled
		options.clear();
		for (const auto& entry : entries) {
			for (const auto& option : entry.options) {
				options.push_back(option.c_str());
			}
		}

		widgets.clear();
		std::size_t optionOffset = 0;
		for (const auto& entry : entries) {
			widgets.push_back({
				.id = entry.id,
				.parent = entry.parent,
				.type = entry.type,
				.indent = entry.indent,
				.flags = entry.flags,
				.label = entry.label.c_str(),
				.text = entry.text.c_str(),
				.value = entry.value,
				.min = entry.min,
				.max = entry.max,
				.step = entry.step,
				.options = entry.options.empty() ? nullptr : options.data() + optionOffset,
				.optionCount = static_cast<std::uint32_t>(entry.options.size()),
			});
			optionOffset += entry.options.size();
		}

		const FrameInfo frame{
			.tabs = tabs.data(),
			.tabCount = static_cast<std::uint32_t>(tabs.size()),
			.currentTab = currentTab,
			.title = title.c_str(),
			.widgets = widgets.data(),
			.widgetCount = static_cast<std::uint32_t>(widgets.size()),
			.inputDevice = static_cast<InputDevice>(MANAGER(Input)->GetInputDevice()),
			.hidden = photoMode->IsHidden(),
			.resetAll = photoMode->GetResetAll(),
			.blocked = MANAGER(MenuIntegration)->GetConsoleOpen()
		};

		frontend->OnFrame(frame);
	}

	Manager::Entry& Manager::Add(WidgetID a_id, WidgetType a_type, std::string_view a_label, std::string_view a_text, std::uint32_t a_flags)
	{
		const auto window = ImGui::GetCurrentWindow();

		auto& entry = entries.emplace_back();
		entry.id = a_id;
		entry.type = a_type;
		entry.flags = a_flags;

		// strip ImGui id suffix ("Label##id")
		const auto idPos = a_label.find("##");
		entry.label = a_label.substr(0, idPos);
		entry.text = a_text;

		if (GImGui->CurrentItemFlags & ImGuiItemFlags_Disabled) {
			entry.flags |= kDisabled;
		}
		// recorded widgets take no space, so consume SameLine() here like ItemSize() would
		if (window->DC.IsSameLine) {
			entry.flags |= kSameLine;
			window->DC.IsSameLine = false;
		}
		if (const auto spacing = ImGui::GetStyle().IndentSpacing; spacing > 0.0f) {
			entry.indent = static_cast<std::uint8_t>(window->DC.Indent.x / spacing);
		}

		// innermost enclosing sub tab / tree node
		for (auto i = window->IDStack.Size - 1; i >= 0; i--) {
			if (const auto container = containers.find(window->IDStack[i]); container != containers.end() && container->second != a_id) {
				entry.parent = container->second;
				break;
			}
		}

		return entry;
	}

	std::optional<float> Manager::PopValue(WidgetID a_id)
	{
		std::scoped_lock lock(queueLock);
		if (const auto it = queuedValues.find(a_id); it != queuedValues.end()) {
			const auto value = it->second;
			queuedValues.erase(it);
			return value;
		}
		return std::nullopt;
	}

	bool Manager::PopPress(WidgetID a_id)
	{
		std::scoped_lock lock(queueLock);
		return queuedPresses.erase(a_id) > 0;
	}

	void Manager::Queue(std::function<void()> a_task)
	{
		std::scoped_lock lock(queueLock);
		queuedTasks.push_back(std::move(a_task));
	}

	void Manager::RunQueued()
	{
		std::vector<std::function<void()>> tasks;
		{
			std::scoped_lock lock(queueLock);
			tasks = std::exchange(queuedTasks, {});
		}
		for (const auto& task : tasks) {
			task();
		}
	}

	void Manager::SelectPhoto(std::int32_t a_index, bool a_enlarge)
	{
		Queue([a_index, a_enlarge] {
			const auto gallery = MANAGER(Gallery);
			if (a_index < 0 || a_index >= static_cast<std::int32_t>(gallery->photos.size()) || gallery->showDeleteMsg) {
				return;
			}
			if (a_enlarge) {
				gallery->EnlargePhoto(a_index);
			} else {
				gallery->selectedPhotoIndex = a_index;
			}
		});
	}

	void Manager::ScrollGallery(std::int32_t a_firstRow)
	{
		Queue([a_firstRow] {
			MANAGER(Gallery)->firstVisibleRow = a_firstRow;
		});
	}

	void Manager::GalleryDoAction(GalleryAction a_action)
	{
		Queue([a_action] {
			const auto gallery = MANAGER(Gallery);
			switch (a_action) {
			case GalleryAction::kEnlarge:
				gallery->ToggleEnlarge();
				break;
			case GalleryAction::kToggleLoadScreen:
				gallery->ToggleLoadScreenForSelected();
				break;
			case GalleryAction::kDelete:
				gallery->RequestDelete();
				break;
			case GalleryAction::kBack:
				gallery->GoBack();
				break;
			case GalleryAction::kToggleUI:
				gallery->ToggleUI();
				break;
			case GalleryAction::kPrevious:
				gallery->Navigate(-1, 0);
				break;
			case GalleryAction::kNext:
				gallery->Navigate(+1, 0);
				break;
			default:
				break;
			}
		});
	}

	std::uint32_t Manager::GetGalleryActionKey(GalleryAction a_action) const
	{
		const auto hotKeys = MANAGER(PhotoMode::Hotkeys);

		switch (a_action) {
		case GalleryAction::kEnlarge:
			return hotKeys->GalleryEnlargeKey();
		case GalleryAction::kToggleLoadScreen:
			return hotKeys->GalleryLoadScreenKey();
		case GalleryAction::kDelete:
			return hotKeys->GalleryDeleteKey();
		case GalleryAction::kBack:
			return PhotoMode::Hotkeys::Manager::EscapeKey();
		case GalleryAction::kToggleUI:
			return hotKeys->ToggleMenusKey();
		default:
			return 0;
		}
	}

	void Manager::OnGalleryActivate() const
	{
		if (frontend) {
			frontend->OnGalleryActivate();
		}
	}

	void Manager::OnGalleryDeactivate()
	{
		{
			std::scoped_lock lock(queueLock);
			queuedTasks.clear();
		}
		galleryPhotos.clear();
		galleryStrings.clear();

		if (frontend) {
			frontend->OnGalleryDeactivate();
		}
	}

	void Manager::OnGalleryInput(RE::InputEvent* const* a_events) const
	{
		if (IsReplaced(kReplaceGallery)) {
			frontend->OnGalleryInput(a_events);
		}
	}

	void Manager::UpdateGallery()
	{
		RunQueued();

		const auto gallery = MANAGER(Gallery);
		if (!gallery->IsActive()) {
			return;
		}
		gallery->ClampIndices();
		gallery->UpdateVisibleRows();

		if (!frontend) {
			return;
		}

		// the visible rows and the enlarged photo, reserved so the pointers stay valid
		const auto columns = Gallery::Manager::columns;
		const auto count = static_cast<std::int32_t>(gallery->photos.size());
		const auto first = std::min(gallery->firstVisibleRow * columns, count);
		const auto last = std::min(first + Gallery::Manager::visibleRows * columns, count);
		const auto enlarged = gallery->enlargedPhotoIndex;
		const bool hasEnlarged = enlarged >= 0 && enlarged < count;
		const auto photoCount = static_cast<std::size_t>(last - first) + (hasEnlarged ? 1 : 0);

		galleryPhotos.clear();
		galleryStrings.clear();
		galleryPhotos.reserve(photoCount);
		galleryStrings.reserve(photoCount * 3);

		const auto add_photo = [&](const Gallery::PhotoPtr& a_photo) {
			auto loadScreen = LoadScreenState::kNone;
			if (a_photo->hasLoadScreenDDS && a_photo->index != -1) {
				loadScreen = MANAGER(Screenshot)->IsImageExcludedFromLoadScreen(a_photo->index) ? LoadScreenState::kExcluded : LoadScreenState::kShown;
			}

			const auto& name = galleryStrings.emplace_back(a_photo->name);
			const auto& path = galleryStrings.emplace_back(stl::uft16_to_uft8(a_photo->pngPath.wstring()));
			const auto& thumbnail = galleryStrings.emplace_back(a_photo->GetThumbnail() ? stl::uft16_to_uft8(a_photo->thumbnailPath.wstring()) : std::string{});

			galleryPhotos.push_back({
				.name = name.c_str(),
				.path = path.c_str(),
				.thumbnail = thumbnail.empty() ? nullptr : thumbnail.c_str(),
				.aspectRatio = a_photo->aspectRatio,
				.loadScreen = loadScreen,
			});
		};

		for (auto i = first; i < last; i++) {
			add_photo(gallery->photos[i]);
		}
		if (hasEnlarged) {
			add_photo(gallery->photos[enlarged]);
		}

		const GalleryInfo info{
			.photos = galleryPhotos.data(),
			.photoCount = static_cast<std::uint32_t>(last - first),
			.totalPhotos = static_cast<std::uint32_t>(count),
			.enlargedPhoto = hasEnlarged ? &galleryPhotos.back() : nullptr,
			.selected = gallery->selectedPhotoIndex,
			.enlarged = enlarged,
			.columns = static_cast<std::uint32_t>(columns),
			.visibleRows = static_cast<std::uint32_t>(Gallery::Manager::visibleRows),
			.firstVisibleRow = gallery->firstVisibleRow,
			.gridAspectRatio = gallery->gridAspectRatio,
			.inputDevice = static_cast<InputDevice>(MANAGER(Input)->GetInputDevice()),
			.confirmDelete = gallery->showDeleteMsg,
			.hideUI = gallery->hideEnlargedUI
		};

		frontend->OnGalleryFrame(info);
	}
}
