#include "Gallery/Manager.h"

#include "Graphics.h"
#include "ImGui/IconsFontAwesome6.h"
#include "ImGui/IconsFonts.h"
#include "ImGui/Styles.h"
#include "ImGui/Util.h"
#include "ImGui/Widgets.h"
#include "Input.h"
#include "MenuIntegration.h"
#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"
#include "Settings.h"
#include "Shared.h"

namespace Gallery
{
	bool Photo::Image::Load(const std::filesystem::path& a_path, float a_scale, const std::filesystem::path& a_thumbnailOut)
	{
		auto expected = TextureState::kQueued;
		if (!state.compare_exchange_strong(expected, TextureState::kLoading, std::memory_order_acq_rel)) {
			return false;
		}

		std::error_code ec;
		if (!a_thumbnailOut.empty() && std::filesystem::exists(a_thumbnailOut, ec)) {
			image = std::make_unique<ImGui::Texture>(*stl::utf8_to_utf16(a_thumbnailOut.string()));
			if (image && image->LoadImpl(1.0f)) {
				state.store(TextureState::kReady, std::memory_order_release);
				return true;
			}
			std::filesystem::remove(a_thumbnailOut, ec);
		}

		image = std::make_unique<ImGui::Texture>(*stl::utf8_to_utf16(a_path.string()));
		const bool loaded = image && image->LoadImpl(a_scale, {}, a_thumbnailOut.empty());

		if (loaded) {
			if (!a_thumbnailOut.empty() && image->image) {
				if (!Texture::SaveToPNG(*image->image, a_thumbnailOut.string(), MANAGER(Screenshot)->GetForceSRGB())) {
					std::filesystem::remove(a_thumbnailOut, ec);
				}
				image->image.reset();
			}
			state.store(TextureState::kReady, std::memory_order_release);
			return true;
		}

		image.reset();
		state.store(TextureState::kFailed, std::memory_order_release);
		return false;
	}

	bool Photo::Image::Queue()
	{
		auto expected = TextureState::kUnloaded;
		return state.compare_exchange_strong(expected, TextureState::kQueued, std::memory_order_acq_rel);
	}

	void Photo::Image::Unload()
	{
		auto expected = TextureState::kQueued;
		if (state.compare_exchange_strong(expected, TextureState::kUnloaded, std::memory_order_acq_rel)) {
			return;
		}
		expected = TextureState::kReady;
		if (state.compare_exchange_strong(expected, TextureState::kUnloaded, std::memory_order_acq_rel)) {
			image.reset();
		}
	}

	Photo::Photo(std::filesystem::path a_pngPath) :
		pngPath(std::move(a_pngPath)),
		name(pngPath.stem().string()),
		index(Shared::GetScreenshotIndex(name))
	{}

	void Photo::ReadHeader()
	{
		thumbnailPath = GetThumbnailPath();

		auto size = ImGui::Texture::GetPNGDimensions(pngPath);
		aspectRatio = size.width > 0 && size.height > 0 ? static_cast<float>(size.width) / static_cast<float>(size.height) : 16.0f / 9.0f;

		std::error_code ec;
		lastWriteTime = std::filesystem::last_write_time(pngPath, ec);

		if (index != -1) {
			auto screenshotMgr = MANAGER(Screenshot);
			hasLoadScreenDDS = screenshotMgr->GetScreenshotWithIndex(index) != nullptr || screenshotMgr->GetPaintingWithIndex(index) != nullptr;
		}
	}

	void Photo::LoadTexture(bool a_fullRes)
	{
		if (a_fullRes) {
			full.Load(pngPath, 1.0f);
		} else {
			thumbnail.Load(pngPath, thumbnailScale, thumbnailPath);
		}
	}

	const std::filesystem::path Photo::GetThumbnailPath() const
	{
		return Shared::GetThumbnailPath(MANAGER(Screenshot)->GetThumbnailDirectory(), pngPath);
	}

	bool Manager::CanShowMenu()
	{
		if (!Shared::CanShowMenu()) {
			return false;
		}
		return !MANAGER(PhotoMode)->IsActive();
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		recyclePhotos = a_ini.GetBoolValue("Gallery", "bSendPhotosToRecycleBin", recyclePhotos);
		unpauseMenu = a_ini.GetBoolValue("Gallery", "bUnpauseMenu", unpauseMenu);
		blurMenu = a_ini.GetBoolValue("Gallery", "bBlurMenu", blurMenu);
	}

	bool Manager::IsActive() const
	{
		return activated;
	}

	void Manager::Activate()
	{
		if (activated) {
			return;
		}

		RE::PlaySound("UIMenuOK");

		CollectPhotos();
		StartStreaming();

		firstVisibleRow = 0;
		lastVisibleRow = 0;
		scrollToSelection = true;
		selectedPhotoIndex = photos.empty() ? -1 : 0;
		enlargedPhotoIndex = -1;
		showDeleteMsg = false;

		MANAGER(Input)->ToggleCursor(true);
		if (!unpauseMenu) {
			RE::Main::GetSingleton()->freezeTime = true;
		}
		if (blurMenu) {
			RE::UIBlurManager::GetSingleton()->IncrementBlurCount();
		}
		RE::SendHUDMessage::PushHUDMode("WorldMapMode");

		activated = true;
		if (activeGlobal) {
			activeGlobal->value = 1.0f;
		}
	}

	void Manager::Deactivate()
	{
		if (!activated) {
			return;
		}

		StopStreaming();
		photos.clear();

		ImGui::ClearImGuiState();
		MANAGER(Input)->ResetInputDevices();

		RE::Main::GetSingleton()->freezeTime = false;
		if (blurMenu) {
			RE::UIBlurManager::GetSingleton()->DecrementBlurCount();
		}
		RE::SendHUDMessage::PopHUDMode("WorldMapMode");

		MANAGER(Input)->ToggleCursor(false);

		enlargedPhotoIndex = -1;
		showDeleteMsg = false;
		activated = false;
		if (activeGlobal) {
			activeGlobal->value = 0.0f;
		}

		RE::PlaySound("UIMenuCancel");
	}

	void Manager::ToggleActive()
	{
		if (activated) {
			Deactivate();
			return;
		}
		if (const auto* controlMap = RE::ControlMap::GetSingleton(); controlMap && controlMap->textEntryCount <= 0 && CanShowMenu()) {
			Activate();
		}
	}

	bool Manager::OnFrameUpdate()
	{
		if (!CanShowMenu()) {
			Deactivate();
			return false;
		}

		return true;
	}

	void Manager::CollectPhotos()
	{
		// Documents/My Games/Skyrim Special Edition/Photos
		const auto& photoDir = MANAGER(Screenshot)->GetPhotoDirectory();
		CollectPhotos(photoDir);

		// vanilla dir (root)
		static std::filesystem::path vanillaDir;
		if (vanillaDir.empty()) {
			std::filesystem::path basePath{ *"sScreenShotBaseName:Display"_ini };
			vanillaDir = basePath.parent_path();
			if (!vanillaDir.is_absolute()) {
				vanillaDir = std::filesystem::current_path() / vanillaDir;
			}
		}
		std::error_code ec;
		if (!std::filesystem::equivalent(vanillaDir, photoDir, ec)) {
			CollectPhotos(vanillaDir);
		}

		std::for_each(std::execution::par, photos.begin(), photos.end(), [](const PhotoPtr& a_photo) {
			a_photo->ReadHeader();
		});

		std::ranges::sort(photos, [](const PhotoPtr& a_lhs, const PhotoPtr& a_rhs) {
			return *a_lhs < *a_rhs;
		});

		CalcGridAspectRatio();
	}

	void Manager::CollectPhotos(const std::filesystem::path& a_folder)
	{
		Shared::ForEachFile(a_folder, ".png"sv, [this](const auto& a_path) {
			photos.emplace_back(std::make_shared<Photo>(a_path));
		});
	}

	void Manager::CalcGridAspectRatio()
	{
		gridAspectRatio = 16.0f / 9.0f;

		if (photos.empty()) {
			return;
		}

		std::map<float, std::int32_t> counts;
		for (const auto& photo : photos) {
			counts[photo->aspectRatio]++;
		}

		const auto mostCommon = std::ranges::max_element(counts, {}, [](const auto& a_pair) { return a_pair.second; });
		gridAspectRatio = mostCommon->first;
	}

	std::int32_t Manager::GetActiveIndex()
	{
		return enlargedPhotoIndex != -1 ? enlargedPhotoIndex : selectedPhotoIndex;
	}

	void Manager::ClampIndices()
	{
		if (photos.empty()) {
			selectedPhotoIndex = -1;
			enlargedPhotoIndex = -1;
		} else {
			selectedPhotoIndex = std::clamp(selectedPhotoIndex, 0, static_cast<std::int32_t>(photos.size()) - 1);
		}
	}

	void Manager::DeleteSelectedPhoto()
	{
		if (selectedPhotoIndex == -1 || selectedPhotoIndex >= static_cast<std::int32_t>(photos.size())) {
			return;
		}

		const auto& photo = photos[selectedPhotoIndex];

		std::error_code ec;

		// png
		if (recyclePhotos) {
			Shared::RecycleFile(photo->pngPath.wstring());
		} else {
			if (!Shared::RemoveFile(photo->pngPath)) {
				logger::warn("Gallery: failed to delete PNG ({})", photo->pngPath.string());
			} else {
				logger::info("Gallery: deleted PNG ({})", photo->pngPath.string());
			}
		}

		// loadscreen images
		MANAGER(Screenshot)->DeleteImagesWithIndex(photo->index, recyclePhotos);

		//thumbnail
		if (recyclePhotos) {
			Shared::RecycleFile(photo->thumbnailPath.wstring());
		} else {
			if (!Shared::RemoveFile(photo->thumbnailPath)) {
				logger::warn("Gallery: failed to delete thumbnail ({})", photo->thumbnailPath.string());
			} else {
				logger::info("Gallery: deleted thumbnail ({})", photo->thumbnailPath.string());
			}
		}

		enlargedPhotoIndex = -1;

		photos.erase(photos.begin() + selectedPhotoIndex);

		ClampIndices();
		scrollToSelection = true;
	}

	void Manager::StartStreaming()
	{
		if (!streamingThreads.empty()) {
			return;
		}

		const auto numThreads = std::max(2u, std::thread::hardware_concurrency() / 4);
		streamingThreads.reserve(numThreads);

		for (std::uint32_t i = 0; i < numThreads; i++) {
			streamingThreads.emplace_back([this](std::stop_token a_token) {
				::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
				while (!a_token.stop_requested()) {
					LoadRequest request;
					{
						std::unique_lock lock(queueLock);
						if (!queueCV.wait(lock, a_token, [this] { return !loadQueue.empty(); })) {
							break;  // stop requested
						}
						request = std::move(loadQueue.front());
						loadQueue.pop_front();
					}

					if (const auto photo = request.photo.lock()) {
						photo->LoadTexture(request.fullRes);
					}
				}
			});
		}
	}

	void Manager::StopStreaming()
	{
		for (auto& thread : streamingThreads) {
			thread.request_stop();
		}
		queueCV.notify_all();
		streamingThreads.clear();

		std::scoped_lock lock(queueLock);
		loadQueue.clear();
	}

	void Manager::QueueLoad(const PhotoPtr& a_photo, bool a_fullRes)
	{
		if (!a_photo->QueueImage(a_fullRes)) {
			return;
		}
		{
			std::scoped_lock lock(queueLock);
			if (a_fullRes) {
				loadQueue.emplace_front(a_photo, a_fullRes);
			} else {
				loadQueue.emplace_back(a_photo, a_fullRes);
			}
		}
		queueCV.notify_one();
	}

	void Manager::PrefetchTextures()
	{
		if (photos.empty()) {
			return;
		}

		const auto count = static_cast<std::int32_t>(photos.size());
		const auto queue_photos = [&](std::int32_t a_firstRow, std::int32_t a_lastRow) {
			const auto first = std::max(a_firstRow, 0) * columns;
			const auto last = std::min((a_lastRow + 1) * columns, count);
			for (auto i = first; i < last; i++) {
				QueueLoad(photos[i], false);
			}
		};

		queue_photos(firstVisibleRow, lastVisibleRow);
		queue_photos(lastVisibleRow + 1, lastVisibleRow + prefetchRows);
		queue_photos(firstVisibleRow - prefetchRows, firstVisibleRow - 1);
	}

	bool Manager::IsEnlargedOrNeighbor(std::int32_t a_index) const
	{
		if (enlargedPhotoIndex == -1) {
			return false;
		}
		if (a_index == enlargedPhotoIndex) {
			return true;
		}

		const auto count = static_cast<std::int32_t>(photos.size());
		if (count < 2) {
			return false;
		}

		return a_index == (enlargedPhotoIndex + 1) % count ||
		       a_index == (enlargedPhotoIndex - 1 + count) % count;
	}

	void Manager::FlushTextures()
	{
		const auto keepFirst = (firstVisibleRow - retainRows) * columns;
		const auto keepLast = (lastVisibleRow + retainRows + 1) * columns;

		for (std::int32_t i = 0; i < static_cast<std::int32_t>(photos.size()); i++) {
			if (i < keepFirst || i >= keepLast) {
				photos[i]->UnloadThumbnail();
			}
			if (!IsEnlargedOrNeighbor(i)) {
				photos[i]->UnloadFullRes();
			}
		}
	}

	void Manager::GoBack()
	{
		if (showDeleteMsg) {
			showDeleteMsg = false;
		} else if (enlargedPhotoIndex != -1) {
			enlargedPhotoIndex = -1;
			scrollToSelection = true;
		} else {
			Deactivate();
		}
	}

	void Manager::ToggleEnlarge()
	{
		if (showDeleteMsg) {
			DeleteSelectedPhoto();
			showDeleteMsg = false;
			return;
		}

		if (selectedPhotoIndex == -1) {
			return;
		}

		if (enlargedPhotoIndex == -1) {
			enlargedPhotoIndex = selectedPhotoIndex;
			RE::Main::GetSingleton()->freezeTime = true;
			hideEnlargedUI = false;
		} else {
			enlargedPhotoIndex = -1;
			if (!unpauseMenu) {
				RE::Main::GetSingleton()->freezeTime = false;
			}
			hideEnlargedUI = true;
		}
	}

	void Manager::ToggleUI()
	{
		if (enlargedPhotoIndex != -1 && !showDeleteMsg) {
			hideEnlargedUI = !hideEnlargedUI;
		}
	}

	void Manager::StepEnlarged(std::int32_t a_direction)
	{
		if (photos.empty() || enlargedPhotoIndex == -1) {
			return;
		}

		const auto count = static_cast<std::int32_t>(photos.size());
		enlargedPhotoIndex = (enlargedPhotoIndex + a_direction + count) % count;

		RE::PlaySound("UIMenuFocus");
		selectedPhotoIndex = enlargedPhotoIndex;
		scrollToSelection = true;
	}

	void Manager::RequestDelete()
	{
		if (selectedPhotoIndex != -1 && !showDeleteMsg) {
			showDeleteMsg = true;
		}
	}

	void Manager::ToggleLoadScreenForSelected()
	{
		if (showDeleteMsg) {
			return;
		}

		const auto target = GetActiveIndex();
		if (target == -1 || target >= photos.size()) {
			return;
		}

		const auto& photo = photos[target];
		if (photo->index == -1 || !photo->hasLoadScreenDDS) {
			return;
		}

		MANAGER(Screenshot)->ToggleLoadScreenForIndex(photo->index);
	}

	void Manager::Navigate(std::int32_t a_dx, std::int32_t a_dy)
	{
		if (showDeleteMsg || photos.empty()) {
			return;
		}

		if (enlargedPhotoIndex != -1) {
			if (a_dx != 0) {
				StepEnlarged(a_dx > 0 ? +1 : -1);
			}
			return;
		}

		const auto count = static_cast<std::int32_t>(photos.size());

		if (selectedPhotoIndex == -1) {
			selectedPhotoIndex = std::clamp(firstVisibleRow * columns, 0, count - 1);
		}

		const auto delta = a_dx != 0 ? a_dx : a_dy * columns;
		if (delta == 0) {
			return;
		}

		selectedPhotoIndex = ((selectedPhotoIndex + delta) % count + count) % count;
		scrollToSelection = true;
	}

	void Manager::Draw()
	{
		if (!activated) {
			return;
		}

		ClampIndices();

		if (enlargedPhotoIndex != -1) {
			DrawEnlarged();
			FlushTextures();
			return;
		}

		const auto viewportSize = ImGui::GetNativeViewportSize();
		const auto windowSize = ImVec2(viewportSize.x * 0.8f, viewportSize.y * 0.8f);

		ImGui::SetNextWindowPos(ImGui::GetNativeViewportCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

		ImGui::Begin("##Gallery", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
		{
			DrawHeader();
			DrawGrid();

			ImGui::PushFont(nullptr, MANAGER(IconFont)->GetLargeFontSize());
			ImGui::SetCursorPosY(GetBottomBarPosY());
			if (showDeleteMsg) {
				DrawDeleteBanner();
			} else {
				DrawBottomBar();
			}
			ImGui::PopFont();
		}
		ImGui::End();

		PrefetchTextures();
		FlushTextures();
	}

	void Manager::DrawHeader()
	{
		ImGui::PushFont(nullptr, MANAGER(IconFont)->GetLargeFontSize());
		ImGui::TextUnformatted("$PM_PhotoGallery_Menu"_T);

		ImGui::SameLine();

		ImGui::CenteredText(GetCounter().c_str());
		ImGui::PopFont();

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));
	}

	std::string Manager::GetCounter()
	{
		return std::format("{:03} / {:03}", GetActiveIndex() + 1, photos.size());  // 999
	}

	void Manager::DrawGrid()
	{
		const auto  contentAvail = ImGui::GetContentRegionAvail();
		const float bottomBarHeight = GetBottomBarHeight();

		const auto& style = ImGui::GetStyle();
		const float scrollbarSize = style.ScrollbarSize;
		const float availGridWidth = contentAvail.x - scrollbarSize;
		const float availGridHeight = contentAvail.y - bottomBarHeight;

		ImVec2 cellSize{ (availGridWidth - style.ItemSpacing.x * (columns + 1)) / columns, 0.0f };
		cellSize.y = cellSize.x / gridAspectRatio;
		if (cellSize.y * visibleRows + style.ItemSpacing.y * (visibleRows + 1) > availGridHeight) {
			cellSize.y = (availGridHeight - style.ItemSpacing.y * (visibleRows + 1)) / visibleRows;
			cellSize.x = cellSize.y * gridAspectRatio;
		}

		ImVec2 gap{ (availGridWidth - cellSize.x * columns) / (columns + 1), (availGridHeight - cellSize.y * visibleRows) / (visibleRows + 1) };

		const auto totalRows = static_cast<std::int32_t>((photos.size() + columns - 1) / columns);
		const auto maxTopRow = std::max(0, totalRows - visibleRows);

		const auto mousePos = ImGui::GetMousePos();

		if (static auto menuMgr = MANAGER(MenuIntegration); menuMgr->GetConsoleOpen()) {
			mouseMoved = false;
		} else {
			mouseMoved = std::abs(mousePos.x - lastMousePos.x) > 1.0f || std::abs(mousePos.y - lastMousePos.y) > 1.0f;
			if (mouseMoved) {
				lastMousePos = mousePos;
			}
		}

		if (const auto wheel = ImGui::GetIO().MouseWheel; wheel != 0.0f) {
			firstVisibleRow -= static_cast<std::int32_t>(wheel > 0.0f ? std::ceil(wheel) : std::floor(wheel));
		}

		const auto startPos = ImGui::GetCursorPos();
		const bool scrollable = totalRows > visibleRows;

		const auto  windowPos = ImGui::GetWindowPos();
		const float trackX = windowPos.x + ImGui::GetWindowWidth() - scrollbarSize * 0.65f;
		const float trackTop = windowPos.y + startPos.y + gap.y;
		const float trackHeight = availGridHeight - gap.y * 2.0f;
		const float thumbHeight = scrollable ? trackHeight * static_cast<float>(visibleRows) / totalRows : trackHeight;

		bool thumbHovered = false;
		bool thumbHeld = false;

		if (scrollable) {
			const float thumbTravel = trackHeight - thumbHeight;

			const float thumbTop = trackTop + thumbTravel * (maxTopRow > 0 ? static_cast<float>(firstVisibleRow) / maxTopRow : 0.0f);

			ImGui::SetCursorScreenPos(ImVec2(windowPos.x + ImGui::GetWindowWidth() - scrollbarSize, trackTop));
			ImGui::InvisibleButton("##GalleryScrollbar", ImVec2(scrollbarSize, trackHeight));

			thumbHovered = ImGui::IsItemHovered();
			thumbHeld = ImGui::IsItemActive();

			if (ImGui::IsItemActivated()) {
				const bool onThumb = mousePos.y >= thumbTop && mousePos.y <= thumbTop + thumbHeight;
				scrollbarDragOffsetY = onThumb ? mousePos.y - thumbTop : thumbHeight * 0.5f;
			}

			if (thumbHeld && thumbTravel > 0.0f) {
				const float targetThumbTop = mousePos.y - scrollbarDragOffsetY;
				const float t = std::clamp((targetThumbTop - trackTop) / thumbTravel, 0.0f, 1.0f);
				firstVisibleRow = static_cast<std::int32_t>(std::lround(t * maxTopRow));
			}
		}

		if (scrollToSelection && selectedPhotoIndex != -1) {
			const auto selRow = selectedPhotoIndex / columns;
			if (selRow < firstVisibleRow) {
				firstVisibleRow = selRow;
			} else if (selRow > firstVisibleRow + visibleRows - 1) {
				firstVisibleRow = selRow - visibleRows + 1;
			}
			scrollToSelection = false;
		}

		firstVisibleRow = std::clamp(firstVisibleRow, 0, maxTopRow);
		lastVisibleRow = std::min(firstVisibleRow + visibleRows - 1, std::max(0, totalRows - 1));

		for (std::int32_t row = 0; row < visibleRows; row++) {
			for (std::int32_t col = 0; col < columns; col++) {
				const auto photoIdx = (firstVisibleRow + row) * columns + col;
				if (photoIdx >= photos.size()) {
					break;
				}
				ImVec2 cell{ (float)col, (float)row };
				ImGui::SetCursorPos(startPos + gap + cell * (cellSize + gap));
				DrawPhotoCell(photoIdx, cellSize);
			}
		}

		// scrollbar
		if (scrollable) {
			const auto drawList = ImGui::GetWindowDrawList();

			const float thumbTravel = trackHeight - thumbHeight;
			const float thumbTop = trackTop + thumbTravel * (maxTopRow > 0 ? static_cast<float>(firstVisibleRow) / maxTopRow : 0.0f);

			const auto grabColor = ImGui::GetColorU32(
				thumbHeld    ? ImGuiCol_ScrollbarGrabActive :
				thumbHovered ? ImGuiCol_ScrollbarGrabHovered :
							   ImGuiCol_ScrollbarGrab);

			drawList->AddRectFilled(ImVec2(trackX, trackTop), ImVec2(trackX + scrollbarSize * 0.3f, trackTop + trackHeight), ImGui::GetColorU32(ImGuiCol_FrameBg), style.ScrollbarRounding);
			drawList->AddRectFilled(ImVec2(trackX, thumbTop), ImVec2(trackX + scrollbarSize * 0.3f, thumbTop + thumbHeight), grabColor, style.ScrollbarRounding);
		}

		ImGui::SetCursorPos(ImVec2(startPos.x, startPos.y + availGridHeight));
	}

	void Manager::DrawPhotoCell(std::int32_t a_photoIdx, const ImVec2& a_cellSize)
	{
		const auto  drawList = ImGui::GetWindowDrawList();
		const auto& style = ImGui::GetStyle();
		const auto  borderSize = style.WindowBorderSize;

		if (a_photoIdx == -1) {
			ImGui::Dummy(a_cellSize);
			drawList->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32_BLACK);
			drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_FrameBgHovered), 0.0, borderSize);
			return;
		}

		const auto& photo = photos[a_photoIdx];

		ImGui::PushID(a_photoIdx);
		const bool clicked = ImGui::InvisibleButton("##photo", a_cellSize);
		ImGui::PopID();

		const auto cellMin = ImGui::GetItemRectMin();
		const auto cellMax = ImGui::GetItemRectMax();

		// background
		drawList->AddRectFilled(cellMin, cellMax, ImGui::GetColorU32(ImGuiCol_FrameBg));

		bool imageDrawn = false;

		if (const auto thumbnail = photo->GetThumbnail()) {
			const auto  areaSize = ImVec2(cellMax.x - cellMin.x, cellMax.y - cellMin.y);
			const float scale = std::max(areaSize.x / thumbnail->size.x, areaSize.y / thumbnail->size.y);

			const float cropU = (1.0f - areaSize.x / (thumbnail->size.x * scale)) * 0.5f;
			const float cropV = (1.0f - areaSize.y / (thumbnail->size.y * scale)) * 0.5f;

			imageDrawn = true;

			drawList->AddImage(thumbnail->srView.Get(), cellMin, cellMax, ImVec2(cropU, cropV), ImVec2(1.0f - cropU, 1.0f - cropV));
		}

		if (mouseMoved && ImGui::IsItemHovered() && !showDeleteMsg) {
			selectedPhotoIndex = a_photoIdx;
		}

		// enlarge
		if (clicked && !showDeleteMsg) {
			RE::PlaySound("UIMenuFocus");
			selectedPhotoIndex = a_photoIdx;
			enlargedPhotoIndex = a_photoIdx;
			hideEnlargedUI = false;
		}

		if (imageDrawn && photo->hasLoadScreenDDS && photo->index != -1) {
			const bool excluded = MANAGER(Screenshot)->IsImageExcludedFromLoadScreen(photo->index);

			constexpr auto icon = ICON_FA_IMAGE;
			const auto     iconSize = ImGui::CalcTextSize(icon);
			const auto     padding = borderSize + 1.0f;

			const ImVec2 iconPos{ cellMax.x - iconSize.x - padding * 2.0f, cellMin.y + padding };

			drawList->AddText(iconPos + style.TextShadowOffset, !excluded ? ImGui::GetColorU32(ImGuiCol_TextShadow) : ImGui::GetColorU32(ImGuiCol_TextShadowDisabled, 0.68f), icon);
			drawList->AddText(iconPos, !excluded ? ImGui::GetColorU32(ImGuiCol_Text) : ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.68f), icon);
		}

		// border
		drawList->AddRect(cellMin, cellMax, a_photoIdx == selectedPhotoIndex ? ImGui::GetColorU32(ImGuiCol_SliderGrabActive) : ImGui::GetColorU32(ImGuiCol_FrameBgHovered), 0.0, borderSize);
	}

	void Manager::DrawEnlarged()
	{
		if (enlargedPhotoIndex < 0 || enlargedPhotoIndex >= photos.size()) {
			enlargedPhotoIndex = -1;
			return;
		}

		const auto& photo = photos[enlargedPhotoIndex];

		if (const auto count = static_cast<std::int32_t>(photos.size()); count > 1) {
			QueueLoad(photos[(enlargedPhotoIndex - 1 + count) % count], true);
			QueueLoad(photos[(enlargedPhotoIndex + 1) % count], true);
		}
		QueueLoad(photo, true);

		const static auto center = ImGui::GetNativeViewportCenter();
		const static auto pos = ImGui::GetNativeViewportPos();
		const static auto size = ImGui::GetNativeViewportSize();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);

		ImGui::Begin("##GalleryEnlarged", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
		{
			const auto windowSize = ImGui::GetWindowSize();

			auto texture = photo->GetFullRes();
			if (!texture) {
				texture = photo->GetThumbnail();
			}

			if (texture) {
				const float scale = std::min(windowSize.x / texture->size.x, windowSize.y / texture->size.y);
				const auto  imageSize = ImVec2(texture->size.x * scale, texture->size.y * scale);

				ImGui::SetCursorPos(ImVec2((windowSize.x - imageSize.x) * 0.5f, (windowSize.y - imageSize.y) * 0.5f));
				ImGui::Image(texture->srView.Get(), imageSize);
			}

			ImGui::PopStyleVar(2);

			ImGui::PushFont(nullptr, MANAGER(IconFont)->GetLargeFontSize());

			const auto offsetY = GetBottomBarHeight();
			ImGui::SetNextWindowPos(ImVec2(center.x, size.y - offsetY), ImGuiCond_Always, ImVec2(0.5, 0.5));

			if (showDeleteMsg || !hideEnlargedUI) {
				ImGui::Begin("##GalleryCounter", 0, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				{
					ImGui::ExtendWindowPastBorder("##GalleryEnlarged");

					if (showDeleteMsg) {
						DrawDeleteBanner();
					} else {
						const auto leftArrow = MANAGER(IconFont)->GetStepperLeft();
						const auto rightArrow = MANAGER(IconFont)->GetStepperRight();

						const float textHeight = ImGui::GetTextLineHeight();
						const float rowHeight = std::max({ leftArrow->size.y, textHeight });
						const float rowTop = ImGui::GetCursorPosY();

						const auto center_on_row = [&](float a_height) {
							ImGui::SetCursorPosY(rowTop + (rowHeight - a_height) * 0.5f);
						};

						center_on_row(leftArrow->size.y);
						ImGui::Image(leftArrow->srView.Get(), leftArrow->size);

						ImGui::SameLine();
						center_on_row(textHeight);
						ImGui::TextUnformatted(GetCounter().c_str());

						ImGui::SameLine();
						center_on_row(rightArrow->size.y);
						ImGui::Image(rightArrow->srView.Get(), rightArrow->size);
					}
				}
				ImGui::End();
			}

			if (!showDeleteMsg && !hideEnlargedUI) {
				const static auto hideUILabel = "$PM_TOGGLEMENUS"_T;
				const static auto deleteLabel = "$PM_DELETE"_T;
				const static auto backLabel = "$PM_EXIT"_T;

				const ImGui::ButtonBarItem items[] = {
					{ MANAGER(Hotkeys)->ToggleMenusIcon(), hideUILabel },
					{ MANAGER(Hotkeys)->GalleryDeleteIcon(), deleteLabel },
					{ MANAGER(Hotkeys)->EscapeIcon(), backLabel },
				};

				auto itemsWidth = ImGui::PrecalcButtonBarWidth(items);
				ImGui::SetNextWindowPos(ImVec2(size.x - itemsWidth * 0.75f, size.y - offsetY), ImGuiCond_Always, ImVec2(0.5, 0.5));

				ImGui::Begin("##GalleryBar", 0, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
				{
					ImGui::ExtendWindowPastBorder("##GalleryEnlarged");

					ImGui::ButtonBar(items, itemsWidth, 0.5f);
				}
				ImGui::End();
			}

			ImGui::PopFont();
		}
		ImGui::End();
	}

	void Manager::DrawDeleteBanner()
	{
		if (selectedPhotoIndex < 0 || selectedPhotoIndex >= photos.size()) {
			showDeleteMsg = false;
			return;
		}

		ImGui::CenteredText("$PM_DELETE_PHOTO"_T);

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));

		ImGui::BeginChild("##GalleryBarDelete", ImVec2(0.0f, 0.0f), enlargedPhotoIndex != -1 ? (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY) : ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
		{
			const static auto confirmLabel = "$PM_CONFIRM"_T;
			const static auto cancelLabel = "$PM_CANCEL"_T;

			const ImGui::ButtonBarItem items[] = {
				{ MANAGER(Hotkeys)->GalleryEnlargeIcon(), confirmLabel },
				{ MANAGER(Hotkeys)->EscapeIcon(), cancelLabel },
			};
			ImGui::ButtonBar(items, 0.5f);
		}
		ImGui::EndChild();
	}

	void Manager::DrawBottomBar() const
	{
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetUserStyleVar(ImGui::USER_STYLE::kSeparatorThickness));

		ImGui::BeginChild("##GalleryBar", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
		{
			const static auto enlargeLabel = "$PM_ENLARGE"_T;
			const static auto deleteLabel = "$PM_DELETE"_T;
			const static auto exitLabel = "$PM_EXIT"_T;
			const static auto hideLSLabel = "$PM_HIDE_LOADSCREEN"_T;
			const static auto showLSLabel = "$PM_SHOW_LOADSCREEN"_T;

			bool        showLSHint = false;
			const char* lsLabel = hideLSLabel;
			if (selectedPhotoIndex != -1 && selectedPhotoIndex < photos.size()) {
				const auto& selected = photos[selectedPhotoIndex];
				showLSHint = selected->hasLoadScreenDDS;
				if (showLSHint && MANAGER(Screenshot)->IsImageExcludedFromLoadScreen(selected->index)) {
					lsLabel = showLSLabel;
				}
			}

			const ImGui::ButtonBarItem items[] = {
				{ MANAGER(Hotkeys)->GalleryEnlargeIcon(), enlargeLabel },
				{ MANAGER(Hotkeys)->GalleryLoadScreenIcon(), lsLabel, true, !showLSHint },
				{ MANAGER(Hotkeys)->GalleryDeleteIcon(), deleteLabel },
				{ MANAGER(Hotkeys)->EscapeIcon(), exitLabel },
			};
			ImGui::ButtonBar(items, 1.0f);
		}
		ImGui::EndChild();
	}

	void Manager::OnDataLoad()
	{
		activeGlobal = RE::TESForm::LookupByEditorID<RE::TESGlobal>("PhotoGallery_IsActive");
	}
}
