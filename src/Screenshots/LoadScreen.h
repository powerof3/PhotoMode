#pragma once

namespace LoadScreen
{
	enum class Type : std::uint8_t
	{
		kNone,
		kFullScreen,
		kPainting
	};

	struct Transform
	{
		float        scale{ 1.0f };
		RE::NiPoint3 rotationalOffset{};
		RE::NiPoint3 translateOffset{};
	};

	class Manager final : public REX::Singleton<Manager>
	{
	public:
		void LoadMCMSettings(const CSimpleIniA& a_ini);
		void InitLoadScreenObjects();

		RE::TESObjectSTAT* LoadScreenshotModel();

		std::optional<Transform> GetModelTransform() const;
		const char*              GetCameraShotPath(const char* a_path) const;

		void ApplyScreenshotTexture(RE::BSGeometry* a_canvas) const;

	private:
		Type        GetScreenshotModelType() const;
		std::string GetScreenshotTexture() const;

		// members
		std::int32_t fullscreenChance{ 50 };
		std::int32_t paintingChance{ 50 };

		std::vector<RE::TESObjectSTAT*> paintingModels{};
		Transform                       paintingTransform{ 0.6f, RE::NiPoint3(), RE::NiPoint3() };
		RE::TESObjectSTAT*              fullscreenModel{};
		Transform                       fullscreenTransform{ 2.0f, RE::NiPoint3(), RE::NiPoint3(-45.0, 0, 0) };

		struct
		{
			RE::TESObjectSTAT* obj{};
			Type               type{ Type::kNone };
			std::string        texturePath{};

		} current;
	};
}
