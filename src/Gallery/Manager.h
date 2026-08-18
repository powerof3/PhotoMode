#pragma once

#include "ImGui/Graphics.h"

namespace Gallery
{
	struct Photo
	{
		Photo() = default;
		Photo(std::filesystem::path a_pngPath);

		bool operator<(const Photo& a_rhs) const
		{
			return lastWriteTime < a_rhs.lastWriteTime;
		}

		void ReadHeader();
		void LoadTexture(bool a_fullRes);

		ImGui::Texture* GetThumbnail() { return thumbnail.GetImage(); }
		ImGui::Texture* GetFullRes() { return full.GetImage(); }
		bool            QueueImage(bool a_fullRes) { return (a_fullRes ? full : thumbnail).Queue(); }
		void            UnloadThumbnail() { thumbnail.Unload(); }
		void            UnloadFullRes() { full.Unload(); }

		// members
		std::filesystem::path           pngPath{};
		std::filesystem::path           thumbnailPath{};
		std::filesystem::file_time_type lastWriteTime{};
		std::string                     name{};
		std::int32_t                    index{ -1 };
		float                           aspectRatio{ 16.0f / 9.0f };
		bool                            hasLoadScreenDDS{ false };

	private:
		static constexpr float      thumbnailScale{ 0.25f };
		const std::filesystem::path GetThumbnailPath() const;

		struct Image
		{
			enum class TextureState : std::uint8_t
			{
				kUnloaded,
				kQueued,
				kLoading,
				kReady,
				kFailed
			};

			bool            Load(const std::filesystem::path& a_path, float a_scale, const std::filesystem::path& a_thumbnailOut = {});
			bool            Queue();
			ImGui::Texture* GetImage() { return state.load(std::memory_order_acquire) == TextureState::kReady ? image.get() : nullptr; }
			void            Unload();

			std::atomic<TextureState>       state{ TextureState::kUnloaded };
			std::unique_ptr<ImGui::Texture> image{};
		};

		Image thumbnail;
		Image full;
	};

	using PhotoPtr = std::shared_ptr<Photo>;

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		bool CanShowMenu();
		void LoadMCMSettings(const CSimpleIniA& a_ini);

		[[nodiscard]] bool IsActive() const;
		void               Activate();
		void               Deactivate();
		void               ToggleActive();

		bool OnFrameUpdate();

		void Draw();

		void GoBack();
		void ToggleEnlarge();
		void ToggleUI();
		void RequestDelete();

		void ToggleLoadScreenForSelected();

		void Navigate(std::int32_t a_dx, std::int32_t a_dy);

	private:
		static constexpr std::int32_t columns{ 4 };
		static constexpr std::int32_t visibleRows{ 3 };
		static constexpr std::int32_t prefetchRows{ 3 };
		static constexpr std::int32_t retainRows{ 6 };

		struct LoadRequest
		{
			std::weak_ptr<Photo> photo{};
			bool                 fullRes{ false };
		};

		void CollectPhotos();
		void CollectPhotos(const std::filesystem::path& a_folder);
		void CalcGridAspectRatio();

		std::int32_t GetActiveIndex();
		void         ClampIndices();

		void DeleteSelectedPhoto();

		// texture streaming
		void StartStreaming();
		void StopStreaming();
		void QueueLoad(const PhotoPtr& a_photo, bool a_fullRes);
		void PrefetchTextures();
		void FlushTextures();
		bool IsEnlargedOrNeighbor(std::int32_t a_index) const;

		void StepEnlarged(std::int32_t a_direction);

		void        DrawHeader();
		std::string GetCounter();
		void        DrawGrid();
		void        DrawPhotoCell(std::int32_t a_photoIdx, const ImVec2& a_cellSize);
		void        DrawEnlarged();
		void        DrawDeleteBanner();
		void        DrawBottomBar() const;

		float GetBottomBarHeight() const { return ImGui::GetFrameHeightWithSpacing() * 2.10f; }
		float GetBottomBarPosY() const { return ImGui::GetWindowHeight() - GetBottomBarHeight(); }

		// members
		bool activated{ false };

		std::vector<PhotoPtr> photos{};

		std::int32_t selectedPhotoIndex{ -1 };
		std::int32_t enlargedPhotoIndex{ -1 };

		// grid state
		std::int32_t firstVisibleRow{ 0 };
		std::int32_t lastVisibleRow{ 0 };
		float        scrollbarDragOffsetY;
		bool         scrollToSelection{ false };

		bool  showDeleteMsg{ false };
		float gridAspectRatio{ 16.0f / 9.0f };

		bool   mouseMoved{ false };
		ImVec2 lastMousePos{ -RE::NI_INFINITY, -RE::NI_INFINITY };
		bool   hideEnlargedUI{ false };

		bool recyclePhotos{ false };

		// texture streaming
		std::vector<std::jthread>   streamingThreads{};
		mutable std::mutex          queueLock{};
		std::condition_variable_any queueCV{};
		std::deque<LoadRequest>     loadQueue{};
	};
}
