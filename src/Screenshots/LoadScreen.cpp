#include "LoadScreen.h"

#include "Textures.h"
#include "ImGui/Util.h"

namespace LoadScreen
{
	void Manager::LoadSettings(CSimpleIniA& a_ini)
	{
		ini::get_value(a_ini, fullscreenSSType, "LoadScreen", "ShowFullScreen", ";Display fullscreen screenshots\n;0 - Disabled, 1 - Screenshot, 2 - Painting");
		ini::get_value(a_ini, paintingSSType, "LoadScreen", "ShowPainting", ";Display screenshots as paintings");
		ini::get_value(a_ini, fullscreenChance, "LoadScreen", "FullScreenChance", ";How often screenshots will appear in place of existing loadscreens");
		ini::get_value(a_ini, paintingChance, "LoadScreen", "PaintingChance", nullptr);
	}

	void Manager::SaveSettings(CSimpleIniA& a_ini) const
	{
		a_ini.SetLongValue("LoadScreen", "ShowFullScreen", stl::to_underlying(fullscreenSSType), ";Display fullscreen screenshots\n;0 - Disabled, 1 - Screenshot, 2 - Painting");
		a_ini.SetLongValue("LoadScreen", "ShowPainting", stl::to_underlying(paintingSSType), ";Display screenshots as paintings");
		a_ini.SetLongValue("LoadScreen", "FullScreenChance", fullscreenChance, ";How often screenshots will appear in place of existing loadscreens");
		a_ini.SetLongValue("LoadScreen", "PaintingChance", paintingChance, nullptr);
	}

	void Manager::Revert()
	{
		fullscreenSSType = Screenshot::Type::kScreenshot;
		fullscreenChance = 50;

		paintingSSType = Screenshot::Type::kPainting;
		paintingChance = 50;
	}

	void Manager::Draw()
	{
		ImGui::EnumSlider("$PM_TypeFullScreen"_T, &fullscreenSSType, screenshotTypes);
		ImGui::Slider("$PM_FullScreenChance"_T, &fullscreenChance, 0, 100);

		ImGui::EnumSlider("$PM_TypePainting"_T, &paintingSSType, screenshotTypes);
		ImGui::Slider("$PM_PaintingChance"_T, &paintingChance, 0, 100);
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
			if (fullscreenSSType > Screenshot::Type::kNone && rng.Generate<std::int32_t>(0, 100) <= fullscreenChance) {
				return Type::kFullScreen;
			}

			if (paintingSSType > Screenshot::Type::kNone && rng.Generate<std::int32_t>(0, 100) <= paintingChance) {
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
				current.ssType = fullscreenSSType;
				current.texturePath = GetScreenshotTexture(fullscreenSSType);

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
				current.ssType = paintingSSType;
				current.texturePath = GetScreenshotTexture(paintingSSType);

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

	std::string Manager::GetScreenshotTexture(Screenshot::Type a_ssType)
	{
		const auto screenshotMgr = Screenshot::Manager::GetSingleton();

		switch (a_ssType) {
		case Screenshot::Type::kScreenshot:
			return screenshotMgr->GetRandomScreenshot();
		case Screenshot::Type::kPainting:
			return screenshotMgr->GetRandomPaintingShot();
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
			const auto mgr = Manager::GetSingleton();
			if (auto transform = mgr->GetModelTransform()) {
				func(a_this, transform->scale, transform->rotationalOffset, transform->translateOffset, mgr->GetCameraShotPath(a_cameraShotPath));
				if (const auto canvas = a_this->loadScreenModel ? a_this->loadScreenModel->GetObjectByName("Canvas:0") : nullptr) {
					mgr->ApplyScreenshotTexture(canvas->AsGeometry());
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
