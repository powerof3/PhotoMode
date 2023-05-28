#pragma once

#include "Screenshots.h"

namespace LoadScreen
{
	struct Transform
	{
		float        scale{ 1.0f };
		RE::NiPoint3 rotationalOffset{};
		RE::NiPoint3 translateOffset{};
	};

	enum class Type : std::uint32_t
	{
		kNone,
		kFullScreen,
		kPainting
	};

	class Manager final : public ISingleton<Manager>
	{
	public:
		void LoadSettings(CSimpleIniA& a_ini);
		void SaveSettings(CSimpleIniA& a_ini) const;
		void Revert();

		void Draw();

		void PopulateLoadScreenObjects();

		Type               GetScreenshotModelType() const;
		RE::TESObjectSTAT* LoadScreenshotModel();

		std::optional<Transform> GetModelTransform() const;
		static std::string       GetScreenshotTexture(Screenshot::Type a_ssType);
		const char*              GetCameraShotPath(const char* a_path) const;

		void ApplyScreenshotTexture(RE::BSGeometry* a_canvas) const;

	private:
		static constexpr std::array<const char*, 3> ssType{ "DISABLED", "SCREENSHOT TEXTURE", "PAINTING TEXTURE" };

		// members
		Screenshot::Type fullscreenSSType{ Screenshot::Type::kScreenshot };
		std::int32_t     fullscreenChance{ 50 };

		Screenshot::Type paintingSSType{ Screenshot::Type::kPainting };
		std::int32_t     paintingChance{ 50 };

		std::vector<RE::TESObjectSTAT*> paintingModels{};
		Transform                       paintingTransform{ 0.75f, RE::NiPoint3(0, 0, 0), RE::NiPoint3(0, 0, 0) };
		RE::TESObjectSTAT*              fullscreenModel{};
		Transform                       fullscreenTransform{ 2.0f, RE::NiPoint3(0, 0, 0), RE::NiPoint3(-45.0, 0, 0) };

		struct
		{
			RE::TESObjectSTAT* obj{};
			Type               type{ Type::kNone };
			Screenshot::Type   ssType{ Screenshot::Type::kNone };
			std::string        texturePath{};

		} current;
	};

	void InstallHook();
}
