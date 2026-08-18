#pragma once

namespace Shared
{
	inline bool CanShowMenu()
	{
		static constexpr std::array badMenus{
			RE::MainMenu::MENU_NAME,
			RE::MistMenu::MENU_NAME,
			RE::LoadingMenu::MENU_NAME,
			RE::FaderMenu::MENU_NAME,
			"LootMenu"sv,
			"CustomMenu"sv
		};

		const auto UI = RE::UI::GetSingleton();
		if (!UI || std::ranges::any_of(badMenus, [&](const auto& menuName) { return UI->IsMenuOpen(menuName); })) {
			return false;
		}

		const auto* controlMap = RE::ControlMap::GetSingleton();
		if (!controlMap) {
			return false;
		}

		switch (controlMap->contextPriorityStack.back()) {
		case RE::UserEvents::INPUT_CONTEXT_ID::kGameplay:
		case RE::UserEvents::INPUT_CONTEXT_ID::kTFCMode:
		case RE::UserEvents::INPUT_CONTEXT_ID::kConsole:
		case RE::UserEvents::INPUT_CONTEXT_ID::kCursor:
			return true;
		default:
			return false;
		}
	}

	inline std::int32_t GetScreenshotIndex(const std::string& a_path)
	{
		boost::smatch       matches;
		static boost::regex screenshotPattern{ R"(Screenshot_?(\d+))", boost::regex::icase };
		if (boost::regex_search(a_path, matches, screenshotPattern)) {
			if (matches.size() > 1) {
				return string::to_num<std::int32_t>(matches[1].str());
			}
		}
		return -1;
	}

	inline std::filesystem::path GetThumbnailPath(const std::filesystem::path& a_thumbnailDir, const std::filesystem::path& a_srcPNG)
	{
		std::error_code ec;
		auto            absPath = std::filesystem::weakly_canonical(a_srcPNG, ec);
		if (ec || absPath.empty()) {
			absPath = std::filesystem::absolute(a_srcPNG, ec);
		}

		return a_thumbnailDir / std::format("{:X}.png", hash::fnv1a_64(string::tolower(absPath.string())));
	}

	inline std::expected<void, std::error_code> GetOrCreateDirectory(const std::filesystem::path& a_dir)
	{
		std::error_code ec;
		if (std::filesystem::exists(a_dir, ec)) {
			return {};
		}
		std::filesystem::create_directories(a_dir, ec);
		if (ec) {
			return std::unexpected(ec);
		}
		return {};
	}

	inline std::filesystem::path GetDocumentsFolder(std::string_view a_subPath)
	{
		if (auto directory = logger::log_directory()) {
			directory->remove_filename();
			*directory /= a_subPath;
			return *directory;
		}
		return {};
	}

	template <class F>
	void ForEachFile(const std::filesystem::path& a_dir, std::string_view a_extension, F&& a_func)
	{
		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator(a_dir, ec)) {
			if (entry.is_regular_file(ec) && entry.path().extension() == a_extension) {
				a_func(entry.path());
			}
		}
	}

	// https://stackoverflow.com/questions/70257751/move-a-file-or-folder-to-the-recyclebin-trash-c17
	inline bool RecycleSaves(const std::wstring& path)
	{
		const std::wstring widestr = path + L'\0';

		SHFILEOPSTRUCT fileOp;
		fileOp.hwnd = nullptr;
		fileOp.wFunc = FO_DELETE;
		fileOp.pFrom = widestr.c_str();
		fileOp.pTo = nullptr;
		fileOp.fFlags = FOF_ALLOWUNDO | FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;

		return SHFileOperation(&fileOp) == 0;
	}

	// std::filesystem::remove doesn't remove files managed by Root Builder
	inline bool RemoveFile(const std::filesystem::path& a_path)
	{
		const HANDLE handle = ::CreateFileW(a_path.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, nullptr);
		if (handle == INVALID_HANDLE_VALUE) {
			std::error_code ec;
			std::filesystem::remove(a_path, ec);
			return !ec;
		}
		::CloseHandle(handle);
		return true;
	}
}
