#include "LoadScreen.h"

#include "Textures.h"

namespace LoadScreen
{
	void Manager::LoadSettings(const CSimpleIniA& a_ini)
	{
		fullscreenChance = a_ini.GetLongValue("LoadScreen", "iChanceFullScreenArt", fullscreenChance);
		paintingChance = a_ini.GetLongValue("LoadScreen", "iChancePainting", paintingChance);
	}

	void Manager::InitLoadScreenObjects()
	{
		std::vector<std::string> painting_paths;

		const auto iterator = std::filesystem::directory_iterator(R"(Data\Meshes\PhotoMode\Paintings)");
		for (const auto& entry : iterator) {
			if (entry.exists()) {
				if (const auto& path = entry.path(); !path.empty() && path.extension() == ".nif") {
					auto pathStr = entry.path().string();
					painting_paths.push_back(Mesh::Sanitize(pathStr));
				}
			}
		}

		if (const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::TESObjectSTAT>()) {
			paintingModels.reserve(painting_paths.size());
			for (auto& path : painting_paths) {
				if (const auto staticObj = factory->Create()) {
					staticObj->SetModel(path.data());
					paintingModels.emplace_back(staticObj);
				}
			}

			if (fullscreenModel = factory->Create(); fullscreenModel) {
				fullscreenModel->SetModel("Meshes\\PhotoMode\\FullScreen01.nif");
			}
		}
	}

	Type Manager::GetScreenshotModelType() const
	{
		if (Screenshot::Manager::GetSingleton()->CanDisplayScreenshot()) {
			auto rng = RNG();

			// process fullscreen art first
			if (rng.Generate<std::int32_t>(0, 100) <= fullscreenChance) {
				return Type::kFullScreen;
			}

			if (rng.Generate<std::int32_t>(0, 100) <= paintingChance) {
				return Type::kPainting;
			}
		}

		return Type::kNone;
	}

	RE::TESObjectSTAT* Manager::LoadScreenshotModel()
	{
		current.type = GetScreenshotModelType();

		switch (current.type) {
		case Type::kFullScreen:
			{
				current.obj = fullscreenModel;
				current.ssType = Screenshot::Type::kScreenshot;
				current.texturePath = GetScreenshotTexture();

				// skip if empty
				if (current.texturePath.empty()) {
					current.obj = nullptr;
					current.ssType = Screenshot::Type::kNone;
				}
			}
			break;
		case Type::kPainting:
			{
				current.obj = paintingModels[RNG().Generate<std::size_t>(0, paintingModels.size() - 1)];  // Load random painting mesh
				current.ssType = Screenshot::Type::kPainting;
				current.texturePath = GetScreenshotTexture();

				// skip if empty
				if (current.texturePath.empty()) {
					current.obj = nullptr;
					current.ssType = Screenshot::Type::kNone;
				}
			}
			break;
		default:
			{
				current.obj = nullptr;
				current.ssType = Screenshot::Type::kNone;
				current.texturePath.clear();
			}
			break;
		}

		return current.obj;
	}

	std::optional<Transform> Manager::GetModelTransform() const
	{
		switch (current.type) {
		case Type::kPainting:
			return paintingTransform;
		case Type::kFullScreen:
			return fullscreenTransform;
		default:
			return std::nullopt;
		}
	}

	std::string Manager::GetScreenshotTexture() const
	{
		switch (current.ssType) {
		case Screenshot::Type::kScreenshot:
			return MANAGER(Screenshot)->GetRandomScreenshot();
		case Screenshot::Type::kPainting:
			return MANAGER(Screenshot)->GetRandomPaintingShot();
		default:
			return {};
		}
	}

	const char* Manager::GetCameraShotPath(const char* a_path) const
	{
		if (current.ssType == Screenshot::Type::kScreenshot) {
			return nullptr;
		}

		return a_path;
	}

	void Manager::ApplyScreenshotTexture(RE::BSGeometry* a_canvas) const
	{
		const auto effect = a_canvas->properties[RE::BSGeometry::States::kEffect];
		const auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect.get());
		const auto material = lightingShader ? static_cast<RE::BSLightingShaderMaterialBase*>(lightingShader->material) : nullptr;

		if (lightingShader && material) {
			if (const auto newMaterial = static_cast<RE::BSLightingShaderMaterialBase*>(material->Create()); newMaterial) {
				newMaterial->CopyMembers(material);
				newMaterial->ClearTextures();

				if (const auto newTextureSet = RE::BSShaderTextureSet::Create(); newTextureSet) {
					newTextureSet->SetTexturePath(RE::BSTextureSet::Texture::kDiffuse, current.texturePath.c_str());
					newTextureSet->SetTexturePath(RE::BSTextureSet::Texture::kNormal, material->textureSet->GetTexturePath(RE::BSTextureSet::Texture::kNormal));
					newMaterial->OnLoadTextureSet(0, newTextureSet);
				}

				lightingShader->SetMaterial(newMaterial, true);

				lightingShader->SetupGeometry(a_canvas);
				lightingShader->FinishSetupGeometry(a_canvas);

				newMaterial->~BSLightingShaderMaterialBase();
				RE::free(newMaterial);
			}
		}
	}

	struct GetLoadScreenModel
	{
		static RE::TESModelTextureSwap* thunk([[maybe_unused]] RE::TESLoadScreen* a_loadScreen)
		{
			if (const auto screenshotModel = Manager::GetSingleton()->LoadScreenshotModel()) {
				return screenshotModel;
			}
			return func(a_loadScreen);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct InitLoadScreen3D
	{
		static void thunk(RE::MistMenu* a_this, float a_scale, const RE::NiPoint3& a_rotateOffset, const RE::NiPoint3& a_translateOffset, const char* a_cameraShotPath)
		{
			if (auto transform = MANAGER(LoadScreen)->GetModelTransform()) {
				func(a_this, transform->scale, transform->rotationalOffset, transform->translateOffset, MANAGER(LoadScreen)->GetCameraShotPath(a_cameraShotPath));

				if (const auto canvas = a_this->loadScreenModel ? a_this->loadScreenModel->GetObjectByName("Canvas:0") : nullptr) {
					MANAGER(LoadScreen)->ApplyScreenshotTexture(canvas->AsGeometry());
				}
			} else {
				func(a_this, a_scale, a_rotateOffset, a_translateOffset, a_cameraShotPath);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHook()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(51048, 51929), OFFSET(0x384, 0x27B) };
		stl::write_thunk_call<GetLoadScreenModel>(target.address());

		REL::Relocation<std::uintptr_t> target2{ RELOCATION_ID(51454, 52313), OFFSET(0x1D1, 0x1C3) };
		stl::write_thunk_call<InitLoadScreen3D>(target2.address());
	}
}
