#pragma once

namespace Translation
{
	class Manager final : public REX::Singleton<Manager>
	{
	public:
		static std::string GetGameLanguage();

		void BuildTranslationMap();
		bool LoadTranslation(const std::filesystem::path& a_path);

		template <class T>
		const std::string& GetTranslation(const T& a_key)
		{
			if (const auto it = translationMap.find(a_key); it != translationMap.end()) {
				return it->second;
			}

			static std::string str = "TRANSLATION FAILED";
			return str;
		}

	private:
		StringMap<std::string> translationMap{};
	};
}

#define TRANSLATE(STR) Translation::Manager::GetSingleton()->GetTranslation(STR).c_str()
#define TRANSLATE_S(STR) Translation::Manager::GetSingleton()->GetTranslation(STR)

constexpr const char* operator""_T(const char* str, std::size_t)
{
	return Translation::Manager::GetSingleton()->GetTranslation(str).c_str();
}
