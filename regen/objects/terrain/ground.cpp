#include "ground.h"
#include "regen/textures/texture-loader.h"
#include "regen/states/depth-state.h"
#include "regen/objects/composite-mesh.h"
#include "regen/objects/primitives/blanket.h"

using namespace regen;

uint32_t Ground::MAX_BLENDED_MATERIALS = 3;
float Ground::MIN_MATERIAL_WEIGHT = 0.1f;

Ground::Ground() : SkirtQuad() {
	rectangleConfig_.updateHint.frequency = BUFFER_UPDATE_NEVER;
	rectangleConfig_.updateHint.scope = BUFFER_UPDATE_FULLY;
	rectangleConfig_.accessMode = BUFFER_GPU_ONLY;
	rectangleConfig_.mapMode = BUFFER_MAP_DISABLED;
	rectangleConfig_.isNormalRequired = false;
	rectangleConfig_.isTexcoRequired = false;
	rectangleConfig_.isTangentRequired = false;
	rectangleConfig_.posScale = Vec3f::one();
	rectangleConfig_.rotation = Vec3f(0.0f, 0.0f, M_PI);
	rectangleConfig_.centerAtOrigin = true;

	u_mapCenter_ = ref_ptr<ShaderInput3f>::alloc("mapCenter");
	u_mapCenter_->setUniformData(Vec3f::zero());
	u_mapCenter_->setSchema(InputSchema::position());

	u_mapSize_ = ref_ptr<ShaderInput3f>::alloc("mapSize");
	u_mapSize_->setUniformData(Vec3f::one());
	u_mapSize_->setSchema(InputSchema::scale());

	u_skirtSize_ = ref_ptr<ShaderInput1f>::alloc("skirtSize");
	u_skirtSize_->setUniformData(0.05f);
	u_skirtSize_->setSchema(InputSchema::scale());

	u_uvScale_ = ref_ptr<ShaderInput1f>::alloc("groundMaterialUVScale");
	u_uvScale_->set_forceArray(true);
	u_uvScale_->setSchema(InputSchema::scale());
	u_normalIdx_ = ref_ptr<ShaderInput1i>::alloc("groundMaterialNormalIdx");
	u_normalIdx_->set_forceArray(true);

	groundMaterial_ = ref_ptr<Material>::alloc(BufferUpdateFlags::NEVER);
	groundShaderDefines_ = ref_ptr<State>::alloc();
	weightUpdateState_ = ref_ptr<State>::alloc();
	weightUpdateState_->joinStates(groundShaderDefines_);
	joinSkirtStates(groundMaterial_);
	joinSkirtStates(groundShaderDefines_);
}

void Ground::setLODConfig(uint32_t numPatchesPerRow,
		const std::vector<uint32_t> &levelOfDetails) {
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
	groundMaterial_->set_texture(normalMap_, TextureState::MAP_TO_CUSTOM, "normalMap");
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
	ptr_cfg->height = newConfig.height;
	ptr_cfg->slope = newConfig.slope;
	ptr_cfg->isFallback = newConfig.isFallback;
	ptr_cfg->uvScale = newConfig.uvScale;
}

void Ground::setBiome(const BiomeDescription &biome) {
	biomeConfigs_.push_back(biome);
}

void Ground::addMask(
		const ref_ptr<Texture> &maskTexture,
		const std::vector<MaskChannel> &maskChannels,
		const MaskFallback &maskFallback) {
	if (!maskTexture) {
		REGEN_WARN("Ground::addMask() called without mask texture.");
		return;
	}
	if (maskChannels.empty()) {
		REGEN_WARN("Ground::addMask() called without mask channels.");
		return;
	}
	const uint32_t maskIdx = groundMasks_.size();
	auto &newMask = groundMasks_.emplace_back();
	newMask.texture = maskTexture;
	newMask.channels = maskChannels;
	newMask.fallback = maskFallback;
	newMask.state = ref_ptr<TextureState>::alloc(maskTexture, REGEN_STRING("groundMask" << maskIdx));
	newMask.state->set_mapTo(TextureState::MAP_TO_CUSTOM);
	newMask.state->set_mapping(TextureState::MAPPING_CUSTOM);
	joinStates(newMask.state);

	// add defines for the channels.
#define _MASK_KEY(x) REGEN_STRING("MASK_" << maskIdx << "_" << x)
	shaderDefine(_MASK_KEY("NUM_CHANNELS"), REGEN_STRING(maskChannels.size()));
	if (maskFallback.channelIdx >= 0) {
		shaderDefine(_MASK_KEY("FALLBACK_CHANNEL"), REGEN_STRING(maskFallback.channelIdx));
		shaderDefine(_MASK_KEY("FALLBACK_INTENSITY"), REGEN_STRING(maskFallback.intensity));
	}
	for (size_t i = 0; i < maskChannels.size(); ++i) {
		auto &ch = maskChannels[i];
		shaderDefine(_MASK_KEY("BLEND_MODE_" << i), REGEN_STRING(ch.blendMode));
		shaderDefine(_MASK_KEY("CHANNEL_" << i), REGEN_STRING(ch.channelIdx));
		shaderDefine(_MASK_KEY("INVERT_" << i), ch.invert ? "1" : "0");
		shaderDefine(_MASK_KEY("BLEND_FACTOR_" << i), REGEN_STRING(ch.blendFactor));
	}
#undef _MASK_KEY
	shaderDefine("NUM_GROUND_MASKS", REGEN_STRING(groundMasks_.size()));
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
	auto numPatches = numPatches_.x * numPatches_.y;
	float offsetX = mapCenter_.x - (mapSize_.x / 2.0f);
	float offsetZ = mapCenter_.z - (mapSize_.z / 2.0f);
	auto patchHalfSize = patchSize_ / 2.0f;
	uint32_t tfIndex = 0;

	tf_->set_numInstances(numPatches);
	tf_->modelOffset()->setInstanceData(numPatches, 1, nullptr);
	auto tfData = tf_->modelOffset()->mapClientData<Vec4f>(BUFFER_GPU_WRITE);
	for (uint32_t xIdx=0; xIdx<numPatches_.x; ++xIdx) {
		for (uint32_t zIdx=0; zIdx<numPatches_.y; ++zIdx) {
			auto xPos = offsetX + (static_cast<float>(xIdx) * patchSize_) + patchHalfSize;
			auto zPos = offsetZ + (static_cast<float>(zIdx) * patchSize_) + patchHalfSize;
			auto &patchTF = tfData.w[tfIndex++];
			patchTF.x = xPos;
			patchTF.y = mapCenter_.y - mapSize_.y * 0.5f;
			patchTF.z = zPos;
		}
	}
}

void Ground::updateAttributes() {
	SkirtQuad::updateAttributes();
	updateGroundPatches();
	// update bounding box
	Vec3f minPos = rectangleConfig_.translation;
	Vec3f maxPos = rectangleConfig_.translation;
	minPos.x -= patchSize_ * 0.5f;
	minPos.z -= patchSize_ * 0.5f;
	maxPos.x += patchSize_ * 0.5f;
	maxPos.z += patchSize_ * 0.5f;
	minPos.y -= skirtSize_;
	maxPos.y += 0.1f*mapSize_.y;
	minPos.y -= 0.1f*mapSize_.y;

	set_bounds(minPos, maxPos);
	if(skirtMesh_.get()) {
		skirtMesh_->set_bounds(minPos, maxPos);
	}
}

void Ground::setSkirtInput(const ref_ptr<ShaderInput> &in) {
	setInput(in);
	if(skirtMesh_.get()) {
		skirtMesh_->setInput(in);
	}
}

void Ground::joinSkirtStates(const ref_ptr<State> &state) {
	joinStates(state);
	if(skirtMesh_.get()) {
		skirtMesh_->joinStates(state);
	}
}

void Ground::setupGroundBlanket(Mesh &blanket) {
	blanket.setInput(u_mapCenter_);
	blanket.setInput(u_skirtSize_);
	blanket.setInput(u_mapSize_);
	blanket.setInput(u_uvScale_);
	blanket.setInput(u_normalIdx_);

	blanket.joinStates(groundMaterial_);
	blanket.joinStates(groundShaderDefines_);
	blanket.joinStates(materialAlbedoState_);
	blanket.joinStates(materialNormalState_);
	if (materialMaskState_.get()) {
		blanket.joinStates(materialMaskState_);
	}
	for (uint32_t i=0; i<weightMaps_.size(); ++i) {
		if (!weightMaps_[i].get()) continue;
		blanket.joinStates(ref_ptr<TextureState>::alloc(
			weightMaps_[i], "groundMaterialWeights" + REGEN_STRING(i)));
	}
}

void Ground::createResources() {
	u_skirtSize_->setVertex(0, skirtSize_);
	setSkirtInput(u_mapCenter_);
	setSkirtInput(u_skirtSize_);
	setSkirtInput(u_mapSize_);

	updateMaterialMaps();

	// initialize UV scale and normal index for each material
	std::vector<float_t> uvScales(materialConfigs_.size(), 1.0f);
	std::vector<int32_t> normalIndices(materialConfigs_.size(), 0);
	uint32_t normalIdx = 0;
	for (uint32_t i = 0; i < materialConfigs_.size(); ++i) {
		uvScales[i] = materialConfigs_[i].uvScale;
		if (!materialConfigs_[i].normalFile.empty()) {
			normalIndices[i] = normalIdx++;
		} else {
			normalIndices[i] = numNormalMaps_; // fallback to UP normal
		}
	}
	u_uvScale_->set_numArrayElements(materialConfigs_.size());
	u_uvScale_->setUniformUntyped((byte*)uvScales.data());
	u_normalIdx_->set_numArrayElements(materialConfigs_.size());
	u_normalIdx_->setUniformUntyped((byte*)normalIndices.data());
	setSkirtInput(u_uvScale_);
	setSkirtInput(u_normalIdx_);

	createWeightPass();
}

static int getMode(const Vec2f &range, float maxValue) {
	if (range.x > 0.0001f) {
		if (range.y < maxValue) return 2;
		else return 1;
	} else if (range.y < maxValue) {
		return 0;
	}
	return -1;
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
	std::vector<TextureDescription> albedoFiles;
	std::vector<TextureDescription> normalFiles;
	std::vector<TextureDescription> maskFiles;
	uint32_t materialIdx = 0;
	uint32_t biomeIdx = 0;
	MaterialConfig *fallbackCfg = nullptr;
	uint32_t fallbackIdx = 0;

	groundShaderDefines_->shaderDefine(
		"NUM_MATERIALS", REGEN_STRING(materialConfigs_.size()));
	groundShaderDefines_->shaderDefine(
		"NUM_BIOMES", REGEN_STRING(biomeConfigs_.size()));
	groundShaderDefines_->shaderDefine(
		"MAX_NUM_BLENDED_MATERIALS", REGEN_STRING(MAX_BLENDED_MATERIALS));
	groundShaderDefines_->shaderDefine(
		"MIN_MATERIAL_WEIGHT", REGEN_STRING(MIN_MATERIAL_WEIGHT));
	if (biomeConfigs_.size() > 0) {
		groundShaderDefines_->shaderDefine("HAS_BIOMES", "TRUE");
	}
	for (auto &cfg: biomeConfigs_) {
		groundShaderDefines_->shaderDefine(
			REGEN_STRING("HAS_" << cfg.name << "_BIOME"), "TRUE");
		groundShaderDefines_->shaderDefine(
			REGEN_STRING("GROUND_BIOME_" << biomeIdx++),
			cfg.name);

		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_SLOPE_MIN"),
			REGEN_STRING(cfg.slope.range.x * DEGREE_TO_RAD));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_SLOPE_MAX"),
			REGEN_STRING(cfg.slope.range.y * DEGREE_TO_RAD));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_SLOPE_SMOOTH"),
			REGEN_STRING(cfg.slope.smooth * DEGREE_TO_RAD));
		int slopeMode = getMode(cfg.slope.range, 179.9999f);
		if (slopeMode != -1) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_SLOPE_MODE"), REGEN_STRING(slopeMode));
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_HAS_SLOPE_RANGE"), "TRUE");
		}
		else {
			groundShaderDefines_->shaderDefine(REGEN_STRING(cfg.name << "_BIOME_SLOPE_MODE"), "0");
		}

		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_HEIGHT_MIN"),
			REGEN_STRING(cfg.height.range.x));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_HEIGHT_MAX"),
			REGEN_STRING(cfg.height.range.y));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_HEIGHT_SMOOTH"),
			REGEN_STRING(cfg.height.smooth));
		int heightMode = getMode(cfg.height.range, 0.9999f);
		if (heightMode != -1) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_HEIGHT_MODE"), REGEN_STRING(heightMode));
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_HAS_HEIGHT_RANGE"), "TRUE");
		}
		else {
			groundShaderDefines_->shaderDefine(REGEN_STRING(cfg.name << "_BIOME_HEIGHT_MODE"), "0");
		}

		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_TEMPERATURE_MIN"),
			REGEN_STRING(cfg.temperature.range.x));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_TEMPERATURE_MAX"),
			REGEN_STRING(cfg.temperature.range.y));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_TEMPERATURE_SMOOTH"),
			REGEN_STRING(cfg.temperature.smooth));
		int tempMode = getMode(cfg.temperature.range, 0.9999f);
		if (tempMode != -1) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_TEMPERATURE_MODE"), REGEN_STRING(tempMode));
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_HAS_TEMPERATURE_RANGE"), "TRUE");
		}
		else {
			groundShaderDefines_->shaderDefine(REGEN_STRING(cfg.name << "_BIOME_TEMPERATURE_MODE"), "0");
		}

		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_MOISTURE_MIN"),
			REGEN_STRING(cfg.humidity.range.x));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_MOISTURE_MAX"),
			REGEN_STRING(cfg.humidity.range.y));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_MOISTURE_SMOOTH"),
			REGEN_STRING(cfg.humidity.smooth));
		int humidMode = getMode(cfg.humidity.range, 0.9999f);
		if (humidMode != -1) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_MOISTURE_MODE"), REGEN_STRING(humidMode));
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_HAS_MOISTURE_RANGE"), "TRUE");
		}
		else {
			groundShaderDefines_->shaderDefine(REGEN_STRING(cfg.name << "_BIOME_MOISTURE_MODE"), "0");
		}

		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_ROCKINESS_MIN"),
			REGEN_STRING(cfg.rockiness.range.x));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_ROCKINESS_MAX"),
			REGEN_STRING(cfg.rockiness.range.y));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_ROCKINESS_SMOOTH"),
			REGEN_STRING(cfg.rockiness.smooth));
		int rockMode = getMode(cfg.rockiness.range, 0.9999f);
		if (rockMode != -1) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_ROCKINESS_MODE"), REGEN_STRING(rockMode));
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_HAS_ROCKINESS_RANGE"), "TRUE");
		}
		else {
			groundShaderDefines_->shaderDefine(REGEN_STRING(cfg.name << "_BIOME_ROCKINESS_MODE"), "0");
		}

		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_CONCAVITY_MIN"),
			REGEN_STRING(cfg.concavity.range.x));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_CONCAVITY_MAX"),
			REGEN_STRING(cfg.concavity.range.y));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(cfg.name << "_BIOME_CONCAVITY_SMOOTH"),
			REGEN_STRING(cfg.concavity.smooth));
		int concMode = getMode(cfg.concavity.range, 0.9999f);
		if (concMode != -1) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_CONCAVITY_MODE"), REGEN_STRING(concMode));
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(cfg.name << "_BIOME_HAS_CONCAVITY_RANGE"), "TRUE");
		} else {
			groundShaderDefines_->shaderDefine(REGEN_STRING(cfg.name << "_BIOME_CONCAVITY_MODE"), "0");
		}
	}
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
		albedoFiles.emplace_back(cfg.colorFile);
		if (!cfg.normalFile.empty()) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_HAS_NORMAL"), "TRUE");
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_NORMAL_IDX"),
				REGEN_STRING(normalFiles.size()));
			normalFiles.emplace_back(cfg.normalFile);
		}
		if (!cfg.maskFile.empty()) {
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_HAS_MASK"), "TRUE");
			groundShaderDefines_->shaderDefine(
				REGEN_STRING(materialType << "_MATERIAL_MASK_IDX"),
				REGEN_STRING(normalFiles.size()));
			maskFiles.emplace_back(cfg.maskFile);
		}
		// add defines for the weighting functions
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_SLOPE_MIN"),
			REGEN_STRING(cfg.slope.range.x * DEGREE_TO_RAD));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_SLOPE_MAX"),
			REGEN_STRING(cfg.slope.range.y * DEGREE_TO_RAD));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_SLOPE_SMOOTH"),
			REGEN_STRING(cfg.slope.smooth * DEGREE_TO_RAD));
		int slopeMode = getMode(cfg.slope.range, 179.9999f);
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

		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_HEIGHT_MIN"),
			REGEN_STRING(cfg.height.range.x));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_HEIGHT_MAX"),
			REGEN_STRING(cfg.height.range.y));
		groundShaderDefines_->shaderDefine(
			REGEN_STRING(materialType << "_MATERIAL_HEIGHT_SMOOTH"),
			REGEN_STRING(cfg.height.smooth));
		int heightMode = getMode(cfg.height.range, 0.9999f);
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
		TextureConfig texCfg;
		texCfg.useMipmaps = true;
		materialAlbedoTex_ = textures::loadArray(albedoFiles, texCfg);
		materialAlbedoState_ = ref_ptr<TextureState>::alloc(materialAlbedoTex_, "groundMaterialAlbedo");
		materialAlbedoState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
		materialAlbedoState_->set_mapping(TextureState::MAPPING_CUSTOM);
		joinSkirtStates(materialAlbedoState_);
	}
	if (!normalFiles.empty()) {
		numNormalMaps_ = static_cast<uint32_t>(normalFiles.size());
		TextureConfig texCfg;
		texCfg.useMipmaps = true;
		texCfg.forcedInternalFormat = GL_RGB8;
		// Append a texture image for the "fallback" normal map which is used by all materials
		// without a normal map. CLear value is set to UP vector in tangent space, normalized to [0,1]
		uint8_t clearValue[4] = { 128u, 128u, 255u };
		normalFiles.emplace_back((byte*)clearValue);
		materialNormalTex_ = textures::loadArray(normalFiles, texCfg);
		materialNormalState_ = ref_ptr<TextureState>::alloc(materialNormalTex_, "groundMaterialNormal");
		materialNormalState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
		materialNormalState_->set_mapping(TextureState::MAPPING_CUSTOM);
		joinSkirtStates(materialNormalState_);
		groundShaderDefines_->shaderDefine(
			REGEN_STRING("GROUND_NUM_NORMAL_MAPS"), REGEN_STRING(numNormalMaps_));
	} else {
		numNormalMaps_ = 0u;
	}
	if (!maskFiles.empty()) {
		TextureConfig texCfg;
		texCfg.useMipmaps = true;
		materialMaskTex_ = textures::loadArray(maskFiles, texCfg);
		materialMaskState_ = ref_ptr<TextureState>::alloc(materialMaskTex_, "groundMaterialMask");
		materialMaskState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
		materialMaskState_->set_mapping(TextureState::MAPPING_CUSTOM);
		joinSkirtStates(materialMaskState_);
	}
}

void Ground::createWeightPass() {
	auto fbo = ref_ptr<FBO>::alloc(weightMapSize_, weightMapSize_);
	std::vector<GLenum> attachments;

	// we create weight maps that store weights for 4 materials each.
	auto numWeightMaps = static_cast<uint32_t>(std::ceil(
			static_cast<float>(materialConfigs_.size()) / 4.0f));
	auto numBiomeMaps = static_cast<uint32_t>(std::ceil(
			static_cast<float>(biomeConfigs_.size()) / 4.0f));
	attachments.resize(numWeightMaps);
	weightMaps_.resize(numWeightMaps);
	groundShaderDefines_->shaderDefine(
		"NUM_WEIGHT_MAPS", REGEN_STRING(numWeightMaps));
	groundShaderDefines_->shaderDefine(
		"NUM_BIOME_MAPS", REGEN_STRING(numBiomeMaps));

	for (uint32_t i=0; i<numWeightMaps; ++i) {
		auto tex = fbo->addTexture(1,
				GL_TEXTURE_2D,
				GL_RGBA,
				GL_RGBA8,
				GL_UNSIGNED_BYTE);
		weightMaps_[i] = ref_ptr<Texture2D>::dynamicCast(tex);
		attachments[i] = GL_COLOR_ATTACHMENT0 + i;
		if (useWeightMapMips_) {
			tex->set_filter(TextureFilter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
		}
		joinSkirtStates(ref_ptr<TextureState>::alloc(
			weightMaps_[i], "groundMaterialWeights" + REGEN_STRING(i)));
	}

	for (uint32_t i=0; i<numBiomeMaps; ++i) {
		auto tex = fbo->addTexture(1,
				GL_TEXTURE_2D,
				GL_RGBA,
				GL_RGBA8,
				GL_UNSIGNED_BYTE);
		biomeMaps_.push_back(ref_ptr<Texture2D>::dynamicCast(tex));
		attachments.push_back(GL_COLOR_ATTACHMENT0 + numWeightMaps + i);
	}

	weightFBO_ = ref_ptr<FBOState>::alloc(fbo);
	weightFBO_->setDrawBuffers(attachments);
	weightUpdateState_->joinStates(weightFBO_);

	// bind ground parameter for size information
	weightUpdateState_->setInput(u_mapCenter_);
	weightUpdateState_->setInput(u_skirtSize_);
	weightUpdateState_->setInput(u_mapSize_);
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
		depth->set_useDepthTest(false);
		depth->set_useDepthWrite(false);
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
			tex->updateMipmaps();
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
	auto out_ = ref_ptr<CompositeMesh>::alloc();
	out_->addMesh(ground);
	ctx.scene()->putResource<CompositeMesh>(input.getName(), out_);

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
				materialCfg.slope = SmoothRange(materialSpec->getValue<Vec3f>(
						"slope", Vec3f(0.0f,180.0f, 10.0f)));
				materialCfg.height = SmoothRange(materialSpec->getValue<Vec3f>(
						"height", Vec3f(0.0f,1.0f,0.1f)));

				ground->setMaterial(materialCfg);
			}
			handledChildren.push_back(n);
		}
		else if (n->getCategory() == "biomes") {
			for (auto &m: n->getChildren()) {
				auto biomeType = m->getValue<std::string>("type", "");
				BiomeDescription biome(biomeType);
				biome.height = SmoothRange(m->getValue<Vec3f>(
					"height", Vec3f(0.0f,1.0f,0.1f)));
				biome.slope = SmoothRange(m->getValue<Vec3f>(
					"slope", Vec3f(0.0f,180.0f, 10.0f)));
				biome.temperature = SmoothRange(m->getValue<Vec3f>(
					"temperature", Vec3f(0.0f,1.0f, 0.1f)));
				biome.humidity = SmoothRange(m->getValue<Vec3f>(
					"humidity", Vec3f(0.0f,1.0f, 0.1f)));
				biome.rockiness = SmoothRange(m->getValue<Vec3f>(
					"rockiness", Vec3f(0.0f,1.0f, 0.1f)));
				biome.concavity = SmoothRange(m->getValue<Vec3f>(
					"concavity", Vec3f(0.0f,1.0f, 0.1f)));
				ground->setBiome(biome);
			}
			handledChildren.push_back(n);
		}
		else if (n->getCategory() == "transform") {
			// note: skip transform, we create one below
			handledChildren.push_back(n);
		}
		else if (n->getCategory() == "masks") {
			handledChildren.push_back(n);
			// Add screen-space masks that modulate the blanket weights
			// (e.g. to remove snow where there are footprints)
			for (auto &maskNode : n->getChildren("mask")) {
				ref_ptr<Texture> maskTexture;
				if (maskNode->hasAttribute("fbo")) {
					auto fboName = maskNode->getValue<std::string>("fbo", "");
					auto fbo = ctx.scene()->getResource<FBO>(fboName);
					if (fbo.get()) {
						uint32_t attachmentIdx = maskNode->getValue<uint32_t>("attachment", 0);
						maskTexture = fbo->colorTextures()[attachmentIdx];
					}
				} else if (maskNode->hasAttribute("texture")) {
					auto texName = maskNode->getValue<std::string>("texture", "");
					maskTexture = ctx.scene()->getResource<Texture2D>(texName);
					if (!maskTexture) {
						REGEN_WARN("No valid texture found for blanket mask in " << maskNode->getDescription() << ".");
					}
				}
				if (!maskTexture.get()) {
					REGEN_WARN("No valid FBO/attachment found for blanket mask in " << maskNode->getDescription() << ".");
					continue;
				}
				MaskFallback maskFallback;
				maskFallback.channelIdx = maskNode->getValue<uint32_t>("fallback-channel", 0);
				maskFallback.intensity = maskNode->getValue<float>("fallback-intensity", 1.0f);

				std::vector<MaskChannel> maskChannels;
				for (auto &n: maskNode->getChildren("channel")) {
					auto &ch = maskChannels.emplace_back();
					ch.channelIdx = n->getValue<uint32_t>("index", 0);
					ch.blendMode = n->getValue<BlendMode>("blend-mode", BLEND_MODE_MULTIPLY);
					ch.blendFactor = n->getValue<float>("blend-factor", 1.0f);
					ch.invert = n->getValue<bool>("invert", false);
				}

				if (maskChannels.empty()) {
					REGEN_WARN("No valid mask channels found for blanket mask in " << maskNode->getDescription() << ".");
				} else {
					ground->addMask(maskTexture, maskChannels, maskFallback);
				}
			}
		}
	}
	for (auto &n: handledChildren) {
		// make sure mesh loading does not attempt to load materials again (this will cause a warning)
		input.removeChild(n);
	}

	auto lodVec = input.getValue<Vec4ui>(
			"lod-levels", Vec4ui(5,3,2, 1));
	auto patchDensity = input.getValue<uint32_t>("patch-density", 9);
	ground->setLODConfig(patchDensity, {lodVec.x, lodVec.y, lodVec.z, lodVec.w});
	ground->setMapGeometry(
			input.getValue<Vec3f>("map-center", Vec3f::zero()),
			input.getValue<Vec3f>("map-size", Vec3f::one()));

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
	auto modelTransform = ref_ptr<ModelTransformation>::alloc(
		ModelTransformation::TF_OFFSET, BufferUpdateFlags::NEVER);
	scene->putResource<ModelTransformation>(tfName, modelTransform);
	ground->setModelTransform(modelTransform);
	ground->updateAttributes();
	modelTransform->tfBuffer()->updateBuffer();

	ground->createResources();
	ground->updateWeightMaps();

	for (uint32_t mapIdx = 0; mapIdx < ground->weightMaps_.size(); ++mapIdx) {
		scene->putResource<Texture>(
			REGEN_STRING(input.getName() << "-weight-map-" << mapIdx),
			ground->weightMaps_[mapIdx]);
	}
	for (uint32_t mapIdx = 0; mapIdx < ground->biomeMaps_.size(); ++mapIdx) {
		scene->putResource<Texture>(
			REGEN_STRING(input.getName() << "-biome-map-" << mapIdx),
			ground->biomeMaps_[mapIdx]);
	}

	return ground;
}
