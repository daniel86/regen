#include "ground.h"
#include "regen/textures/texture-loader.h"
#include "regen/states/depth-state.h"
#include "regen/meshes/mesh-vector.h"

using namespace regen;

Ground::Ground() : SkirtQuad() {
	rectangleConfig_.usage = BUFFER_USAGE_STATIC_DRAW;
	rectangleConfig_.isNormalRequired = false;
	rectangleConfig_.isTexcoRequired = false;
	rectangleConfig_.isTangentRequired = false;
	rectangleConfig_.posScale = Vec3f(1.0f);
	rectangleConfig_.rotation = Vec3f(0.0f, 0.0f, M_PI);
	rectangleConfig_.centerAtOrigin = true;

	u_mapCenter_ = ref_ptr<ShaderInput3f>::alloc("mapCenter");
	u_mapCenter_->setUniformData(Vec3f(0.0f));
	u_mapCenter_->setSchema(InputSchema::position());

	u_mapSize_ = ref_ptr<ShaderInput3f>::alloc("mapSize");
	u_mapSize_->setUniformData(Vec3f(1.0f));
	u_mapSize_->setSchema(InputSchema::scale());

	u_skirtSize_ = ref_ptr<ShaderInput1f>::alloc("skirtSize");
	u_skirtSize_->setUniformData(0.05f);
	u_skirtSize_->setSchema(InputSchema::scale());

	groundMaterial_ = ref_ptr<Material>::alloc();
	joinStates(groundMaterial_);
	groundShaderDefines_ = ref_ptr<State>::alloc();
	joinStates(groundShaderDefines_);
	weightUpdateState_ = ref_ptr<State>::alloc();
	weightUpdateState_->joinStates(groundShaderDefines_);
}

void Ground::setLODConfig(uint32_t numPatchesPerRow,
		const std::vector<GLuint> &levelOfDetails) {
	rectangleConfig_.levelOfDetails = levelOfDetails;
	numPatchesPerRow_ = numPatchesPerRow;
	updatePatchSize();
}

void Ground::setMapGeometry(
				const Vec3f &mapCenter,
				const Vec3f &mapSize) {
	rectangleConfig_.translation = mapCenter;
	mapCenter_ = mapCenter;
	mapSize_ = mapSize;
	u_mapCenter_->setVertex(0, mapCenter_);
	u_mapSize_->setVertex(0, mapSize_);
	groundMaterial_->set_maxOffset(mapSize_.y);
	updatePatchSize();
}

void Ground::setMapTextures(
		const ref_ptr<Texture2D> &heightMap,
		const ref_ptr<Texture2D> &normalMap) {
	heightMap_ = heightMap;
	normalMap_ = normalMap;
	auto heightMapState = groundMaterial_->set_texture(
			heightMap_, TextureState::MAP_TO_HEIGHT, "heightMap");
	heightMapState->set_mapping(ShaderFunction::createImport("regen.terrain.ground.groundUV"));
	heightMapState->set_blendMode(ShaderFunction::createImport("regen.terrain.ground.groundHeightBlend"));
	groundMaterial_->set_texture(
			normalMap_, TextureState::MAP_TO_CUSTOM, "normalMap");
	// TODO: consider using: glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -bias) to force sharper detail
}

void Ground::setMaterial(const MaterialConfig &newConfig) {
	if (newConfig.colorFile.empty()) {
		REGEN_WARN("Ground::setMaterial() called without albedo map.");
		return;
	}
	MaterialConfig *ptr_cfg = nullptr;
	// try to overwrite existing material
	for (auto &oldConfig: materialConfigs_) {
		if (oldConfig.type == newConfig.type) {
			ptr_cfg = &oldConfig;
			break;
		}
	}
	if (!ptr_cfg) {
		// add new material
		auto &newCfg = materialConfigs_.emplace_back();
		ptr_cfg = &newCfg;
	}
	ptr_cfg->type = newConfig.type;
	ptr_cfg->colorFile = newConfig.colorFile;
	ptr_cfg->normalFile = newConfig.normalFile;
	ptr_cfg->maskFile = newConfig.maskFile;
	ptr_cfg->heightRange = newConfig.heightRange;
	ptr_cfg->heightSmoothStep = newConfig.heightSmoothStep;
	ptr_cfg->slopeRange = newConfig.slopeRange;
	ptr_cfg->slopeSmoothStep = newConfig.slopeSmoothStep;
	ptr_cfg->isFallback = newConfig.isFallback;
	ptr_cfg->uvScale = newConfig.uvScale;
}

void Ground::updatePatchSize() {
	// compute number of patches in x/z direction based on map size
	// and number of patches per row (this is used for direction with less extent)
	// FIXME: last row of patches may not fit exactly! could be fixed by scaling model transform.
	//         or patches could have width!=height
	if (mapSize_.x > mapSize_.z) {
		patchSize_ = mapSize_.z / static_cast<float>(numPatchesPerRow_);
		numPatches_.y = numPatchesPerRow_;
		numPatches_.x = static_cast<uint32_t>(std::ceil(mapSize_.x / patchSize_));
	}
	else {
		patchSize_ = mapSize_.x / static_cast<float>(numPatchesPerRow_);
		numPatches_.x = numPatchesPerRow_;
		numPatches_.y = static_cast<uint32_t>(std::ceil(mapSize_.z / patchSize_));
	}
	rectangleConfig_.posScale = Vec3f(patchSize_, 1.0f, patchSize_);
	REGEN_INFO("Ground num patches: " << numPatches_ << " with patch size: " << patchSize_);
}

void Ground::updateGroundPatches() {
	// TODO: we do not really need model transform here, offset would be enough!
	//          --> support both here
	auto numPatches = numPatches_.x * numPatches_.y;
	auto &tf = modelTransform_->get();
	float offsetX = mapCenter_.x - (mapSize_.x / 2.0f);
	float offsetZ = mapCenter_.z - (mapSize_.z / 2.0f);
	auto patchHalfSize = patchSize_ / 2.0f;
	uint32_t tfIndex = 0;

	tf->setInstanceData(numPatches, 1, nullptr);
	auto *tfData = (Mat4f*)tf->clientData();
	for (uint32_t xIdx=0; xIdx<numPatches_.x; ++xIdx) {
		for (uint32_t zIdx=0; zIdx<numPatches_.y; ++zIdx) {
			auto xPos = offsetX + (static_cast<float>(xIdx) * patchSize_) + patchHalfSize;
			auto zPos = offsetZ + (static_cast<float>(zIdx) * patchSize_) + patchHalfSize;
			auto &patchTF = tfData[tfIndex++];
			patchTF = Mat4f::identity();
			patchTF.translate(Vec3f(
					xPos,
					mapCenter_.y - mapSize_.y * 0.5f,
					zPos));
		}
	}
}

void Ground::updateAttributes() {
	Rectangle::updateAttributes();
	updateGroundPatches();
	// update bounding box
	auto minPos = rectangleConfig_.translation;
	auto maxPos = rectangleConfig_.translation;
	minPos.x -= patchSize_ * 0.5f;
	minPos.z -= patchSize_ * 0.5f;
	maxPos.x += patchSize_ * 0.5f;
	maxPos.z += patchSize_ * 0.5f;
	// NOTE: vertex offset only pushes upwards.
	maxPos.y += mapSize_.y;
	minPos.y -= skirtSize_;
	set_bounds(minPos, maxPos);
}

void Ground::createResources() {
	u_skirtSize_->setVertex(0, skirtSize_);
	groundUBO_ = ref_ptr<UBO>::alloc("Ground");
	groundUBO_->addBlockInput(u_mapCenter_); // vec3
	groundUBO_->addBlockInput(u_skirtSize_); // float
	groundUBO_->addBlockInput(u_mapSize_);   // vec3
	groundUBO_->update();
	joinShaderInput(groundUBO_);

	updateMaterialMaps();
	createWeightPass();
}

void Ground::updateMaterialMaps() {
	if (materialAlbedoState_.get()) {
		disjoinStates(materialAlbedoState_);
		disjoinStates(materialNormalState_);
	}
	if (materialConfigs_.empty()) {
		REGEN_WARN("Ground::updateMaterialMaps() called without materials.");
		return;
	}
	std::vector<std::string> albedoFiles;
	std::vector<std::string> normalFiles;
	std::vector<std::string> maskFiles;
	uint32_t materialIdx = 0;
	MaterialConfig *fallbackCfg = nullptr;
	uint32_t fallbackIdx = 0;

	groundShaderDefines_->shaderDefine(
		"NUM_MATERIALS", REGEN_STRING(materialConfigs_.size()));
	for (auto &cfg: materialConfigs_) {
		std::string materialType = REGEN_STRING(cfg.type);
		if (cfg.isFallback) {
			fallbackCfg = &cfg;
			fallbackIdx = materialIdx;
		}

		groundShaderDefines_->shaderDefine(
			REGEN_STRING("HAS_" << materialType << "_MATERIAL"), "TRUE");
		groundShaderDefines_->shaderDefine(
			REGEN_STRING("GROUND_MATERIAL_" << materialIdx++),
			materialType);
		groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_IDX"),
				REGEN_STRING(materialIdx-1));
		groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_UV_SCALE"),
				REGEN_STRING(cfg.uvScale));
		albedoFiles.push_back(cfg.colorFile);
		if (!cfg.normalFile.empty()) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_HAS_NORMAL"), "TRUE");
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_NORMAL_IDX"),
				REGEN_STRING(normalFiles.size()));
			normalFiles.push_back(cfg.normalFile);
		}
		if (!cfg.maskFile.empty()) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_HAS_MASK"), "TRUE");
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_MASK_IDX"),
				REGEN_STRING(normalFiles.size()));
			maskFiles.push_back(cfg.maskFile);
		}
		// add defines for the weighting functions
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_SLOPE_MIN"),
			REGEN_STRING(cfg.slopeRange.x * DEGREE_TO_RAD));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_SLOPE_MAX"),
			REGEN_STRING(cfg.slopeRange.y * DEGREE_TO_RAD));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_SLOPE_SMOOTH"),
			REGEN_STRING(cfg.slopeSmoothStep * DEGREE_TO_RAD));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_HEIGHT_MIN"),
			REGEN_STRING(cfg.heightRange.x));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_HEIGHT_MAX"),
			REGEN_STRING(cfg.heightRange.y));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_HEIGHT_SMOOTH"),
			REGEN_STRING(cfg.heightSmoothStep));

		int slopeMode = -1;
		if (cfg.slopeRange.x > 0.0001f) {
			if (cfg.slopeRange.y < 179.9999f) slopeMode = 2;
			else slopeMode = 1;
		} else if (cfg.slopeRange.y < 179.9999f) {
			slopeMode = 0;
		}
		if (slopeMode != -1) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_SLOPE_MODE"), REGEN_STRING(slopeMode));
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_HAS_SLOPE_RANGE"), "TRUE");
		}
		else {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_SLOPE_MODE"), "0");
		}

		int heightMode = -1;
		if (cfg.heightRange.x > 0.0001f) {
			if (cfg.heightRange.y < 0.9999f) heightMode = 2;
			else heightMode = 1;
		} else if (cfg.heightRange.y < 0.9999f) {
			heightMode = 0;
		}
		if (heightMode != -1) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_HEIGHT_MODE"), REGEN_STRING(heightMode));
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_HAS_HEIGHT_RANGE"), "TRUE");
		}
		else {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_HEIGHT_MODE"), "0");
		}
	}
	if (fallbackCfg) {
		groundShaderDefines_->shaderDefine(
			REGEN_STRING("HAS_fallback_MATERIAL"), "TRUE");
		groundShaderDefines_->shaderDefine(
			REGEN_STRING("fallback_MATERIAL_IDX"), REGEN_STRING(fallbackIdx));
	}
	{
		materialAlbedoTex_ = textures::loadArray(albedoFiles, true);
		materialAlbedoState_ = ref_ptr<TextureState>::alloc(materialAlbedoTex_, "groundMaterialAlbedo");
		materialAlbedoState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
		materialAlbedoState_->set_mapping(TextureState::MAPPING_CUSTOM);
		joinStates(materialAlbedoState_);
	}
	if (!normalFiles.empty()) {
		materialNormalTex_ = textures::loadArray(normalFiles, true);
		materialNormalState_ = ref_ptr<TextureState>::alloc(materialNormalTex_, "groundMaterialNormal");
		materialNormalState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
		materialNormalState_->set_mapping(TextureState::MAPPING_CUSTOM);
		joinStates(materialNormalState_);
	}
	if (!maskFiles.empty()) {
		materialMaskTex_ = textures::loadArray(maskFiles, true);
		materialMaskState_ = ref_ptr<TextureState>::alloc(materialMaskTex_, "groundMaterialMask");
		materialMaskState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
		materialMaskState_->set_mapping(TextureState::MAPPING_CUSTOM);
		joinStates(materialMaskState_);
	}
}

void Ground::createWeightPass() {
	// we create weight maps that store weights for 4 materials each.
	auto numWeightMaps = static_cast<uint32_t>(std::ceil(
			static_cast<float>(materialConfigs_.size()) / 4.0f));
	std::vector<GLenum> attachments(numWeightMaps);
	weightMaps_.resize(numWeightMaps);
	groundShaderDefines_->shaderDefine(
		"NUM_WEIGHT_MAPS", REGEN_STRING(numWeightMaps));

	auto fbo = ref_ptr<FBO>::alloc(weightMapSize_, weightMapSize_);
	for (uint32_t i=0; i<numWeightMaps; ++i) {
		auto tex = fbo->addTexture(1,
				GL_TEXTURE_2D, GL_RGBA, GL_RGBA16F, GL_FLOAT);
		weightMaps_[i] = ref_ptr<Texture2D>::dynamicCast(tex);
		attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		if (useWeightMapMips_) {
			tex->begin(RenderState::get());
			tex->filter().push(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
			tex->end(RenderState::get());
		}

		joinStates(ref_ptr<TextureState>::alloc(
				weightMaps_[i],
				"groundMaterialWeights" + REGEN_STRING(i)));
	}
	weightFBO_ = ref_ptr<FBOState>::alloc(fbo);
	weightFBO_->setDrawBuffers(attachments);
	weightUpdateState_->joinStates(weightFBO_);

	// bind ground UBO for size information
	weightUpdateState_->joinShaderInput(groundUBO_);
	// bind material masks
	if (materialMaskState_.get()) {
		weightUpdateState_->joinStates(materialMaskState_);
	}
	{ // bind height map and normal map
		weightUpdateState_->joinStates(ref_ptr<TextureState>::alloc(heightMap_, "heightMap"));
		weightUpdateState_->joinStates(ref_ptr<TextureState>::alloc(normalMap_, "normalMap"));
	}
	{ // disable depth test/write
		auto depth = ref_ptr<DepthState>::alloc();
		depth->set_useDepthTest(GL_FALSE);
		depth->set_useDepthWrite(GL_FALSE);
		weightUpdateState_->joinStates(depth);
	}
	// run weight pass
	weightUpdatePass_ = ref_ptr<FullscreenPass>::alloc("regen.terrain.ground.weights");
	weightUpdateState_->joinStates(weightUpdatePass_);

	StateConfigurer shaderConfigurer;
	shaderConfigurer.addState(weightUpdateState_.get());
	weightUpdatePass_->createShader(shaderConfigurer.cfg());
}

void Ground::updateWeightMaps() {
	auto *rs = RenderState::get();
	weightUpdateState_->enable(rs);
	weightUpdateState_->disable(rs);
	if(useWeightMapMips_) {
		for (auto &tex : weightMaps_) {
			tex->begin(RenderState::get());
			tex->setupMipmaps(0);
			tex->end(RenderState::get());
		}
	}
}

ref_ptr<Ground> Ground::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();
	auto ground = ref_ptr<Ground>::alloc();

	if (input.hasAttribute("weight-map-size")) {
		ground->setWeightMapSize(input.getValue<uint32_t>("weight-map-size", 2048));
	}
	if (input.hasAttribute("weight-mips")) {
		ground->setUseWeightMapMips(input.getValue<bool>("weight-mips", false));
	}
	if (input.hasAttribute("skirt-size")) {
		ground->setSkirtSize(input.getValue<float>("skirt-size", 0.05f));
	}
	// early add mesh vector to scene for children handling below
	auto out_ = ref_ptr<MeshVector>::alloc();
	out_->push_back(ground);
	ctx.scene()->putResource<MeshVector>(input.getName(), out_);

	std::vector<ref_ptr<scene::SceneInputNode>> handledChildren;
	for (auto &n: input.getChildren()) {
		if (n->getCategory() == "materials") {
			MaterialConfig materialCfg;
			for (auto &materialSpec: n->getChildren()) {
				auto matName = materialSpec->getValue<std::string>("name", "");
				auto variant = materialSpec->getValue<std::string>("variant", "0");
				MaterialDescription materialDescr(matName, variant);
				materialCfg.type = materialSpec->getValue<std::string>("type", "dirt");
				if (materialDescr.hasMap(TextureState::MAP_TO_DIFFUSE)) {
					materialCfg.colorFile = materialDescr.getMaps(TextureState::MAP_TO_DIFFUSE).front();
				} else if (materialDescr.hasMap(TextureState::MAP_TO_COLOR)) {
					materialCfg.colorFile = materialDescr.getMaps(TextureState::MAP_TO_COLOR).front();
				} else {
					REGEN_WARN("No color map found for " << input.getDescription() << ".");
					continue;
				}
				if (materialDescr.hasMap(TextureState::MAP_TO_NORMAL)) {
					materialCfg.normalFile = materialDescr.getMaps(TextureState::MAP_TO_NORMAL).front();
				} else {
					materialCfg.normalFile = "";
				}
				materialCfg.maskFile = materialSpec->getValue<std::string>("mask-file", "");
				materialCfg.isFallback = materialSpec->getValue<bool>("is-fallback", false);
				materialCfg.uvScale = materialSpec->getValue<float>("uv-scale", 0.5f);
				materialCfg.heightRange = materialSpec->getValue<Vec2f>(
						"height-range", Vec2f(0.0f,1.0f));
				materialCfg.heightSmoothStep = materialSpec->getValue<float>(
						"height-smooth", 0.1f);
				materialCfg.slopeRange = materialSpec->getValue<Vec2f>(
						"slope-range", Vec2f(0.0f,180.0f));
				materialCfg.slopeSmoothStep = materialSpec->getValue<float>(
						"slope-smooth", 10.0f);
				ground->setMaterial(materialCfg);
			}
			handledChildren.push_back(n);
		} else if (n->getCategory() == "transform") {
			// note: skip transform, we create one below
			handledChildren.push_back(n);
		}
	}
	for (auto &n: handledChildren) {
		// make sure mesh loading does not attempt to load materials again (this will cause a warning)
		input.removeChild(n);
	}

	auto lodVec = input.getValue<Vec3ui>(
			"lod-levels", Vec3ui(5,3,1));
	auto patchDensity = input.getValue<uint32_t>("patch-density", 9);
	ground->setLODConfig(patchDensity, {lodVec.x, lodVec.y, lodVec.z});
	ground->setMapGeometry(
			input.getValue<Vec3f>("map-center", Vec3f(0.0f)),
			input.getValue<Vec3f>("map-size", Vec3f(1.0f)));

	auto heightMap = scene->getResource<Texture2D>(input.getValue("height-map"));
	auto normalMap = scene->getResource<Texture2D>(input.getValue("normal-map"));
	if (!heightMap.get()) {
		REGEN_WARN("No height map found for " << input.getDescription() << ".");
		return {};
	}
	if (!normalMap.get()) {
		REGEN_WARN("No normal map found for " << input.getDescription() << ".");
		return {};
	}
	ground->setMapTextures(heightMap, normalMap);

	auto tfName = input.getValue("tf");
	auto modelTransform = ref_ptr<ModelTransformation>::alloc();
	scene->putResource<ModelTransformation>(tfName, modelTransform);
	ground->setModelTransform(modelTransform);
	ground->updateAttributes();
	modelTransform->bufferContainer()->updateBuffer();

	ground->createResources();
	ground->updateWeightMaps();

	return ground;
}
