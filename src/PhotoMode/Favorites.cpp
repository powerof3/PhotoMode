#include "Favorites.h"

namespace PhotoMode::Favorites
{
	const StringSet& Manager::GetFavorites(const std::string& a_key)
	{
		return favorites[a_key];
	}

	bool Manager::IsFavorited(const std::string& a_key, const std::string& a_edid)
	{
		const auto it = favorites.find(a_key);
		return it != favorites.end() && it->second.contains(a_edid);
	}

	void Manager::ToggleFavorite(const std::string& a_key, const std::string& a_edid)
	{
		auto& edids = favorites[a_key];
		
		if (!edids.emplace(a_edid).second) {
			edids.erase(a_edid);
		}
	
		SaveFavorites();
	}

	void Manager::LoadFavorites()
	{
		const auto filePath = GetPath();
		if (filePath.empty()) {
			return;
		}

		std::string buffer;
		if (const auto ec = glz::read_file_json(favorites, filePath.string(), buffer)) {
			logger::error("Failed to load favorites from {} ({})", filePath.string(), glz::format_error(ec, buffer));
		}
	}

	void Manager::SaveFavorites() const
	{
		const auto filePath = GetPath();
		if (filePath.empty()) {
			return;
		}

		std::string buffer;
		if (const auto ec = glz::write_file_json(favorites, filePath.string(), buffer)) {
			logger::error("Failed to save favorites to {} ({})", filePath.string(), glz::format_error(ec, buffer));
		}
	}

	const std::filesystem::path& Manager::GetPath() const
	{
		if (favoritesPath.empty()) {
			if (auto directory = logger::log_directory()) {
				directory->remove_filename();
				*directory /= "Saves\\PhotoMode"sv;

				std::error_code ec;
				if (!std::filesystem::exists(*directory, ec)) {
					std::filesystem::create_directories(*directory, ec);
				}

				favoritesPath = *directory / "Favorites.json";
			}
		}
		return favoritesPath;
	}
}
