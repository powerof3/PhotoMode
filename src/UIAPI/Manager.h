#pragma once

#include "PhotoModeUIAPI.h"

namespace UIAPI
{
	using namespace PhotoModeAPI;

	// Records PhotoMode's widget calls for an external frontend (see PhotoModeUIAPI.h)
	class Manager :
		public REX::TSingleton<Manager>,
		public IPhotoMode
	{
	public:
		[[nodiscard]] bool IsRecording() const { return recording; }
		[[nodiscard]] bool IsReplaced(std::uint32_t a_flag) const;
		[[nodiscard]] bool IsCursorOverUI() const { return frontend && frontend->IsCursorOverUI(); }

		void OnActivate() const;
		void OnDeactivate();

		// runs a_func invisible and recorded, then hands the result to the frontend
		template <class F>
		void Update(F&& a_func)
		{
			BeginRecord();
			a_func();
			EndRecord();
		}

		// widgets
		template <class T>
		bool Slider(const char* a_label, T* a_value, T a_min, T a_max, const char* a_format, ImGuiSliderFlags a_flags, float a_speed, bool a_drag)
		{
			const auto id = ImGui::GetID(a_label);

			bool changed = false;
			if (const auto value = PopValue(id)) {
				*a_value = std::clamp(static_cast<T>(*value), a_min, a_max);
				changed = true;
			}

			std::uint32_t flags = (a_flags & ImGuiSliderFlags_NoInput) ? kNoInput : kNone;
			char          text[64];
			if constexpr (std::is_floating_point_v<T>) {
				ImFormatString(text, sizeof(text), a_format ? a_format : "%.2f", static_cast<float>(*a_value));
			} else {
				ImFormatString(text, sizeof(text), a_format ? a_format : "%d", static_cast<std::int32_t>(*a_value));
				flags |= kInteger;
			}

			auto& entry = Add(id, a_drag ? WidgetType::kDrag : WidgetType::kSlider, a_label, text, flags);
			entry.value = static_cast<float>(*a_value);
			entry.min = static_cast<float>(a_min);
			entry.max = static_cast<float>(a_max);
			entry.step = a_speed;

			return changed;
		}

		template <class E>
		bool EnumSlider(const char* a_label, E* a_index, const std::ranges::common_range auto& a_enum, bool a_translate)
		{
			const auto id = ImGui::GetID(a_label);

			std::size_t uIndex;
			if constexpr (std::is_enum_v<E>) {
				uIndex = std::to_underlying(*a_index);
			} else {
				uIndex = *a_index;
			}

			bool changed = false;
			if (const auto value = PopValue(id); value && static_cast<std::size_t>(*value) < a_enum.size()) {
				uIndex = static_cast<std::size_t>(*value);
				*a_index = static_cast<E>(uIndex);
				changed = true;
			}

			auto& entry = Add(id, WidgetType::kEnum, a_label, a_translate ? TRANSLATE(a_enum[uIndex]) : a_enum[uIndex], kNone);
			entry.value = static_cast<float>(uIndex);
			entry.max = static_cast<float>(a_enum.size() - 1);
			for (const auto& option : a_enum) {
				entry.options.emplace_back(a_translate ? TRANSLATE(option) : option);
			}

			return changed;
		}

		bool CheckBox(const char* a_label, bool* a_toggle);
		bool Button(const char* a_label);
		bool Combo(const char* a_label, int* a_currentItem, const std::vector<std::string>& a_items, const StringSet* a_favorites, std::string* a_favToggled);
		bool TabItem(const char* a_label, ImGuiTabItemFlags a_flags);
		bool TreeNode(const char* a_label);
		void Header(const char* a_label);
		void Text(const char* a_label);

		// IPhotoMode
		bool          RegisterFrontend(IFrontend* a_frontend) override;
		void          UnregisterFrontend(IFrontend* a_frontend) override;
		void          SetValue(WidgetID a_id, float a_value) override;
		void          Press(WidgetID a_id) override;
		void          ToggleFavorite(WidgetID a_combo, std::uint32_t a_index) override;
		void          SelectTab(std::uint32_t a_tab) override;
		void          DoAction(Action a_action) override;
		void          GetOptions(WidgetID a_combo, IOptionSink& a_sink) const override;
		std::uint32_t GetActionKey(Action a_action) const override;
		const char*   Translate(const char* a_key) const override { return TRANSLATE(a_key); }
		void          SelectPhoto(std::int32_t a_index, bool a_enlarge) override;
		void          ScrollGallery(std::int32_t a_firstRow) override;
		void          GalleryDoAction(GalleryAction a_action) override;
		std::uint32_t GetGalleryActionKey(GalleryAction a_action) const override;

		// gallery, called by Gallery::Manager
		void OnGalleryActivate() const;
		void OnGalleryDeactivate();
		void OnGalleryInput(RE::InputEvent* const* a_events) const;
		void UpdateGallery();

	private:
		struct Entry
		{
			WidgetID                 id{};
			WidgetID                 parent{};
			WidgetType               type{};
			std::uint8_t             indent{};
			std::uint32_t            flags{};
			std::string              label{};
			std::string              text{};
			float                    value{};
			float                    min{};
			float                    max{};
			float                    step{};
			std::vector<std::string> options{};

			// kCombo, only valid while recording/OnFrame
			const std::vector<std::string>* items{ nullptr };
			const StringSet*                favorites{ nullptr };
		};

		void                 BeginRecord();
		void                 EndRecord();
		Entry&               Add(WidgetID a_id, WidgetType a_type, std::string_view a_label, std::string_view a_text, std::uint32_t a_flags);
		std::optional<float> PopValue(WidgetID a_id);
		bool                 PopPress(WidgetID a_id);
		void                 Queue(std::function<void()> a_task);
		void                 RunQueued();

		// members
		IFrontend* frontend{ nullptr };
		bool       recording{ false };

		std::vector<Entry>         entries{};
		FlatMap<ImGuiID, WidgetID> containers{};  // id pushed by ImGui -> kSubTab/kTreeNode widget id
		std::vector<Widget>        widgets{};
		std::vector<const char*>   options{};
		std::vector<std::string>   tabNames{};
		std::vector<const char*>   tabs{};
		std::string                title{};

		// widget edits are picked up by id while recording, everything else runs before the next pass
		mutable std::mutex                 queueLock{};
		FlatMap<WidgetID, float>           queuedValues{};
		FlatSet<WidgetID>                  queuedPresses{};
		FlatMap<WidgetID, std::uint32_t>   queuedFavorites{};
		std::vector<std::function<void()>> queuedTasks{};

		// gallery, the visible photos
		std::vector<GalleryPhoto> galleryPhotos{};
		std::vector<std::string>  galleryStrings{};
	};
}
