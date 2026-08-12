#pragma once

namespace PhotoMode::Favorites
{
	class Manager : public REX::Singleton<Manager>
	{
	public:
		void LoadFavorites();

		const StringSet& GetFavorites(const std::string& a_type);
		bool             IsFavorited(const std::string& a_type, const std::string& a_edid);
		void             ToggleFavorite(const std::string& a_type, const std::string& a_edid);

	private:
		void                         SaveFavorites() const;
		const std::filesystem::path& GetPath() const;

		std::map<std::string, StringSet> favorites;  // type, favorites
		mutable std::filesystem::path    favoritesPath{};
	};
}
namespace Favorites = PhotoMode::Favorites;
