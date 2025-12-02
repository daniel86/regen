#include <boost/filesystem.hpp>
#include <regen/textures/texture-state.h>
#include "regen/gl/states/atomic-states.h"

#include "material-state.h"
#include "regen/utility/filesystem.h"
#include "regen/textures/texture-loader.h"
#include "regen/objects/assimp-importer.h"

using namespace regen;

Material::Material(const BufferUpdateFlags &updateFlags)
		: State(),
		  fillMode_(GL_FILL),
		  maxOffset_(0.1f),
		  heightMapMode_(HEIGHT_MAP_VERTEX),
		  colorBlendMode_(BLEND_MODE_SRC),
		  colorBlendFactor_(1.0f) {
	texConfig_.useMipmaps = true;
	texConfig_.forcedInternalFormat = GL_NONE;
	texConfig_.forcedFormat = GL_NONE;
	texConfig_.forcedSize = Vec3ui::zero();

	materialSpecular_ = ref_ptr<ShaderInput3f>::alloc("matSpecular");
	materialSpecular_->setUniformData(Vec3f::zero());
	materialSpecular_->setSchema(InputSchema::color());

	materialShininess_ = ref_ptr<ShaderInput1f>::alloc("matShininess");
	materialShininess_->setUniformData(128.0f);
	materialShininess_->setSchema(InputSchema::scalar(0.0f, 128.0f));

	materialDiffuse_ = ref_ptr<ShaderInput3f>::alloc("matDiffuse");
	materialDiffuse_->setUniformData(Vec3f::create(1.0f));
	materialDiffuse_->setSchema(InputSchema::color());

	materialAlpha_ = ref_ptr<ShaderInput1f>::alloc("matAlpha");
	materialAlpha_->setUniformData(1.0f);
	materialAlpha_->setSchema(InputSchema::alpha());

	materialRefractionIndex_ = ref_ptr<ShaderInput1f>::alloc("matRefractionIndex");
	materialRefractionIndex_->setUniformData(0.95f);

	shaderDefine("HAS_MATERIAL", "TRUE");

	materialUniforms_ = ref_ptr<UBO>::alloc("Material", updateFlags);
	materialUniforms_->addStagedInput(materialSpecular_);
	materialUniforms_->addStagedInput(materialShininess_);
	materialUniforms_->addStagedInput(materialDiffuse_);
	materialUniforms_->addStagedInput(materialAlpha_);
	materialUniforms_->addStagedInput(materialRefractionIndex_);
	setInput(materialUniforms_);
}

void Material::set_emission(const Vec3f &emission) {
	if (materialEmission_.get() == nullptr) {
		materialEmission_ = ref_ptr<ShaderInput3f>::alloc("matEmission");
		materialEmission_->setUniformData(emission);
		materialEmission_->setSchema(InputSchema::color());
		materialUniforms_->addStagedInput(materialEmission_);
	} else {
		materialEmission_->setUniformData(emission);
	}
}

void Material::setSpecularMultiplier(float factor) {
	if (materialSpecularMultiplier_.get() == nullptr) {
		materialSpecularMultiplier_ = ref_ptr<ShaderInput1f>::alloc("matSpecularMultiplier");
		materialSpecularMultiplier_->setUniformData(factor);
		materialSpecularMultiplier_->setSchema(InputSchema::scalar(0.0f, 100.0f));
		materialUniforms_->addStagedInput(materialSpecularMultiplier_);
	} else {
		materialSpecularMultiplier_->setUniformData(factor);
	}
}

void Material::setEmissionMultiplier(float factor) {
	if (materialEmissionMultiplier_.get() == nullptr) {
		materialEmissionMultiplier_ = ref_ptr<ShaderInput1f>::alloc("matEmissionMultiplier");
		materialEmissionMultiplier_->setUniformData(factor);
		materialEmissionMultiplier_->setSchema(InputSchema::scalar(0.0f, 100.0f));
		materialUniforms_->addStagedInput(materialEmissionMultiplier_);
	} else {
		materialEmissionMultiplier_->setUniformData(factor);
	}
}

void Material::set_fillMode(GLenum fillMode) {
	if (fillMode == fillMode_) return;
	disjoinStates(fillModeState_);
	fillMode_ = fillMode;
	fillModeState_ = ref_ptr<FillModeState>::alloc(fillMode_);
	joinStates(fillModeState_);
}

void Material::set_twoSided(bool twoSided) {
	if (twoSidedState_.get()) {
		disjoinStates(twoSidedState_);
	}
	twoSidedState_ = ref_ptr<ToggleState>::alloc(RenderState::CULL_FACE, twoSided);
	joinStates(twoSidedState_);
	shaderDefine("HAS_TWO_SIDES", twoSided ? "TRUE" : "FALSE");
}

void Material::set_maxOffset(float offset) {
	maxOffset_ = offset;
	auto heightMaps = textures_.find(TextureState::MAP_TO_HEIGHT);
	if (heightMaps != textures_.end()) {
		for (auto &tex: heightMaps->second) {
			tex->set_blendFactor(offset);
		}
	}
}

void Material::set_textureState(const ref_ptr<TextureState> &texState, TextureState::MapTo mapTo) {
	texState->set_mapping(TextureState::MAPPING_TEXCO);
	texState->set_mapTo(TextureState::MAP_TO_CUSTOM);
	switch (mapTo) {
		case TextureState::MAP_TO_HEIGHT:
			// add the offset to the original position
			texState->set_blendMode(BLEND_MODE_ADD);
			// map height map values from [0,1] to [0,maxOffset]
			texState->set_blendFactor(maxOffset_);
			if (heightMapMode_ == HEIGHT_MAP_VERTEX) {
				// only modify vertex position in case no height mapping via UV transfer is used
				texState->set_mapTo(mapTo);
			}
			break;
		case TextureState::MAP_TO_DISPLACEMENT:
			// add the offset to the original position
			texState->set_blendMode(BLEND_MODE_ADD);
			// map height map values from [0,1] to [0,maxOffset]
			texState->set_blendFactor(maxOffset_);
			break;
		case TextureState::MAP_TO_COLOR:
		case TextureState::MAP_TO_DIFFUSE:
			texState->set_blendMode(colorBlendMode_);
			texState->set_blendFactor(colorBlendFactor_);
			texState->set_mapTo(mapTo);
			break;
		default:
			texState->set_blendMode(BLEND_MODE_SRC);
			texState->set_blendFactor(1.0f);
			texState->set_mapTo(mapTo);
			// if the material has a height map which is mapped to UV transfer, then every
			// texture should use the same UV transfer
			if (textures_.find(TextureState::MAP_TO_HEIGHT) != textures_.end()) {
				if (heightMapMode_ == HEIGHT_MAP_RELIEF) {
					texState->set_texcoTransfer(TextureState::TRANSFER_TEXCO_RELIEF);
				} else if (heightMapMode_ == HEIGHT_MAP_PARALLAX) {
					texState->set_texcoTransfer(TextureState::TRANSFER_TEXCO_PARALLAX);
				} else if (heightMapMode_ == HEIGHT_MAP_PARALLAX_OCCLUSION) {
					texState->set_texcoTransfer(TextureState::TRANSFER_TEXCO_PARALLAX_OCC);
				}
			}
			break;
	}
	joinStates(texState);
}

static bool getMapTo(std::string_view fileName, TextureState::MapTo &mapTo) {
	if (fileName.find("diffuse") != std::string::npos) {
		mapTo = TextureState::MAP_TO_DIFFUSE;
	} else if (fileName.find("color") != std::string::npos) {
		mapTo = TextureState::MAP_TO_COLOR;
	} else if (fileName.find("ambient") != std::string::npos) {
		mapTo = TextureState::MAP_TO_AMBIENT;
	} else if (fileName.find("specular") != std::string::npos) {
		mapTo = TextureState::MAP_TO_SPECULAR;
	} else if (fileName.find("emission") != std::string::npos) {
		mapTo = TextureState::MAP_TO_EMISSION;
	} else if (fileName.find("normal") != std::string::npos) {
		mapTo = TextureState::MAP_TO_NORMAL;
	} else if (fileName.find("height") != std::string::npos) {
		mapTo = TextureState::MAP_TO_HEIGHT;
	} else if (fileName.find("displacement") != std::string::npos) {
		mapTo = TextureState::MAP_TO_DISPLACEMENT;
	} else if (fileName.find("displace") != std::string::npos) {
		mapTo = TextureState::MAP_TO_DISPLACEMENT;
	} else if (fileName.find("occlusion") != std::string::npos) {
		mapTo = TextureState::MAP_TO_LIGHT;
	} else {
		return false;
	}
	return true;

}

MaterialDescription::MaterialDescription(std::string_view materialName, std::string_view variant) {
	// find the base path with the textures
	auto basePath0 = REGEN_STRING("res/textures/materials/" << materialName << "/" << variant);
	basePath0 = resourcePath(basePath0);
	if (!boost::filesystem::exists(basePath0)) {
		basePath0 = REGEN_STRING("res/textures/" << materialName << "/" << variant);
		basePath0 = resourcePath(basePath0);
	}
	if (!boost::filesystem::exists(basePath0)) {
		return;
	}

	bool hasTextures = false;
	for (auto &entry: boost::filesystem::directory_iterator(basePath0)) {
		auto &path = entry.path();
		if (entry.is_directory()) {
			if (variant.empty() && !hasTextures) {
				// look for textures in the subdirectory
				for (auto &subEntry: boost::filesystem::directory_iterator(path)) {
					if (subEntry.is_regular_file()) {
						addTexture(subEntry.path());
						hasTextures = true;
					}
				}
				if (hasTextures) {
					break;
				}
			}
		} else {
			addTexture(entry.path());
			hasTextures = true;
		}
	}
}

void MaterialDescription::addTexture(const boost::filesystem::path &path) {
	auto fileName = path.filename().string();
	TextureState::MapTo mapTo = TextureState::MAP_TO_CUSTOM;
	if (!getMapTo(fileName, mapTo)) {
		return;
	}
	textureFiles[mapTo].push_back(path.string());
}

bool Material::set_textures(std::string_view materialName, std::string_view variant) {
	// find the base path with the textures
	MaterialDescription materialDescr(materialName, variant);

	// remove any non-alphanumeric characters from material name
	std::string materialNameStr(materialName);
	materialNameStr.erase(std::remove_if(materialNameStr.begin(), materialNameStr.end(),
										 [](char c) { return !std::isalnum(c); }), materialNameStr.end());

	// iterate over the texture files in the directory
	for (auto &entry: materialDescr.textureFiles) {
		for (auto &filePath: entry.second) {
			auto tex = textures::load(filePath, texConfig_);
			if (tex.get() != nullptr) {
				// extract file name without extension
				auto fileName = boost::filesystem::path(filePath).stem().string();
				// remove any non-alphanumeric characters
				fileName.erase(std::remove_if(fileName.begin(), fileName.end(),
											  [](char c) { return !std::isalnum(c); }), fileName.end());
				auto texName = REGEN_STRING("tex_" << materialNameStr << "_" << fileName << textures_.size());
				auto texState = ref_ptr<TextureState>::alloc(tex, texName);
				textures_[entry.first].push_back(texState);

				if (wrapping_.has_value()) {
					tex->set_wrapping(TextureWrapping::create(wrapping_.value()));
				}
			} else {
				REGEN_WARN("Failed to load texture '" << filePath << "'.");
			}
		}
	}

	for (auto &pair: textures_) {
		for (auto &tex: pair.second) {
			set_textureState(tex, pair.first);
		}
	}
	return true;
}

bool Material::hasTextureType(TextureState::MapTo mapTo) const {
	return textures_.find(mapTo) != textures_.end() && !textures_.at(mapTo).empty();
}

ref_ptr<Texture> Material::getTexture(TextureState::MapTo mapTo) const {
	if (hasTextureType(mapTo)) {
		return textures_.at(mapTo).front()->texture();
	}
	return {};
}

ref_ptr<Texture> Material::getColorTexture() const {
	return getTexture(TextureState::MapTo::MAP_TO_COLOR);
}

ref_ptr<Texture> Material::getDiffuseTexture() const {
	return getTexture(TextureState::MapTo::MAP_TO_DIFFUSE);
}

ref_ptr<Texture> Material::getSpecularTexture() const {
	return getTexture(TextureState::MapTo::MAP_TO_SPECULAR);
}

ref_ptr<Texture> Material::getNormalTexture() const {
	return getTexture(TextureState::MapTo::MAP_TO_NORMAL);
}

ref_ptr<Texture> Material::getHeightTexture() const {
	return getTexture(TextureState::MapTo::MAP_TO_HEIGHT);
}

ref_ptr<TextureState> Material::set_texture(const ref_ptr<Texture> &tex,
											TextureState::MapTo mapTo, const std::string &texName) {
	auto texState = ref_ptr<TextureState>::alloc(tex, texName);
	textures_[mapTo].push_back(texState);
	if (wrapping_.has_value()) {
		tex->set_wrapping(TextureWrapping::create(GL_CLAMP_TO_EDGE));
	}
	set_textureState(texState, mapTo);
	return texState;
}

ref_ptr<TextureState> Material::set_texture(const ref_ptr<Texture> &tex, TextureState::MapTo mapTo) {
	auto texName = REGEN_STRING("materialTexture" << textures_.size());
	return set_texture(tex, mapTo, texName);
}

void Material::set_iron(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.1843137, 0.168627, 0.15686) * 3.0f);
	materialSpecular_->setUniformData(Vec3f(0.11, 0.11, 0.11));
	materialShininess_->setUniformData(9.8);
	if (!variant.empty()) set_textures("iron", variant);
}

void Material::set_steel(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.423529, 0.439216, 0.450980));
	materialSpecular_->setUniformData(Vec3f(0.21, 0.21, 0.21));
	materialShininess_->setUniformData(21.2);
	if (!variant.empty()) set_textures("steel", variant);
}

void Material::set_silver(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.51, 0.51, 0.51));
	materialSpecular_->setUniformData(Vec3f(0.51, 0.51, 0.51));
	materialShininess_->setUniformData(51.2);
	if (!variant.empty()) set_textures("silver", variant);
}

void Material::set_pewter(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.43, 0.47, 0.54));
	materialSpecular_->setUniformData(Vec3f(0.33, 0.33, 0.52));
	materialShininess_->setUniformData(9.8);
	if (!variant.empty()) set_textures("silver", variant);
}

void Material::set_gold(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.75, 0.61, 0.23));
	materialSpecular_->setUniformData(Vec3f(0.63, 0.65, 0.37));
	materialShininess_->setUniformData(51.2);
	if (!variant.empty()) set_textures("gold", variant);
}

void Material::set_copper(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.70, 0.27, 0.08));
	materialSpecular_->setUniformData(Vec3f(0.26, 0.14, 0.09));
	materialShininess_->setUniformData(12.8);
	if (!variant.empty()) set_textures("copper", variant);
}

void Material::set_metal(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.423529, 0.439216, 0.450980));
	materialSpecular_->setUniformData(Vec3f(0.21, 0.21, 0.21));
	materialShininess_->setUniformData(21.2);
	if (!variant.empty()) set_textures("metal", variant);
}

void Material::set_leather(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.37647, 0.3098, 0.23529));
	materialSpecular_->setUniformData(Vec3f(0.21, 0.21, 0.21));
	materialShininess_->setUniformData(21.2);
	if (!variant.empty()) set_textures("leather", variant);
}

void Material::set_stone(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.57647, 0.572549, 0.592157));
	materialSpecular_->setUniformData(Vec3f(0.14, 0.14, 0.14));
	materialShininess_->setUniformData(52.2);
	if (!variant.empty()) set_textures("stone", variant);
}

void Material::set_marble(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.8f, 0.8f, 0.8f));
	materialSpecular_->setUniformData(Vec3f(0.9f, 0.9f, 0.9f));
	materialShininess_->setUniformData(80.0f);
	if (!variant.empty()) set_textures("marble", variant);
}

void Material::set_wood(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.6f, 0.3f, 0.1f));  // Brown diffuse color
	materialSpecular_->setUniformData(Vec3f(0.2f, 0.2f, 0.2f));
	materialShininess_->setUniformData(25.0f);
	if (!variant.empty()) set_textures("wood", variant);
}

void Material::set_jade(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.54, 0.89, 0.63));
	materialSpecular_->setUniformData(Vec3f(0.32, 0.32, 0.32));
	materialShininess_->setUniformData(12.8);
	if (!variant.empty()) set_textures("jade", variant);
}

void Material::set_ruby(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.61, 0.04, 0.04));
	materialSpecular_->setUniformData(Vec3f(0.73, 0.63, 0.63));
	materialShininess_->setUniformData(76.8);
	if (!variant.empty()) set_textures("ruby", variant);
}

void Material::set_chrome(std::string_view variant) {
	materialDiffuse_->setUniformData(Vec3f(0.40, 0.40, 0.40));
	materialSpecular_->setUniformData(Vec3f(0.77, 0.77, 0.77));
	materialShininess_->setUniformData(76.8);
	if (!variant.empty()) set_textures("chrome", variant);
}

namespace regen {
	std::ostream &operator<<(std::ostream &out, const Material::HeightMapMode &mode) {
		switch (mode) {
			case Material::HEIGHT_MAP_VERTEX:
				return out << "VERTEX";
			case Material::HEIGHT_MAP_RELIEF:
				return out << "RELIEF";
			case Material::HEIGHT_MAP_PARALLAX:
				return out << "PARALLAX";
			case Material::HEIGHT_MAP_PARALLAX_OCCLUSION:
				return out << "PARALLAX_OCCLUSION";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Material::HeightMapMode &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "vertex") mode = Material::HEIGHT_MAP_VERTEX;
		else if (val == "relief") mode = Material::HEIGHT_MAP_RELIEF;
		else if (val == "parallax") mode = Material::HEIGHT_MAP_PARALLAX;
		else if (val == "parallax_occlusion") mode = Material::HEIGHT_MAP_PARALLAX_OCCLUSION;
		else {
			REGEN_WARN("Unknown Height Map Mode '" << val <<
												   "'. Using default VERTEX Mapping.");
			mode = Material::HEIGHT_MAP_VERTEX;
		}
		return in;
	}
}

ref_ptr<Material> Material::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	BufferUpdateFlags updateFlags;
	updateFlags.frequency = input.getValue<BufferUpdateFrequency>(
			"update-frequency", BUFFER_UPDATE_PER_FRAME);
	updateFlags.scope = input.getValue<BufferUpdateScope>(
			"update-scope", BUFFER_UPDATE_FULLY);
	ref_ptr<Material> mat = ref_ptr<Material>::alloc(updateFlags);

	if (input.hasAttribute("max-offset")) {
		mat->set_maxOffset(input.getValue<float>("max-offset", 0.1f));
	}
	if (input.hasAttribute("height-map-mode")) {
		mat->set_heightMapMode(input.getValue<Material::HeightMapMode>("height-map-mode",
																	   Material::HEIGHT_MAP_VERTEX));
	}
	if (input.hasAttribute("color-blend-mode")) {
		mat->set_colorBlendMode(input.getValue<BlendMode>("color-blend-mode", BLEND_MODE_SRC));
	}
	if (input.hasAttribute("color-blend-factor")) {
		mat->set_colorBlendFactor(input.getValue<float>("color-blending-factor", 1.0f));
	}

	if (input.hasAttribute("asset")) {
		ref_ptr<AssetImporter> assetLoader =
				ctx.scene()->getResource<AssetImporter>(input.getValue("asset"));
		if (assetLoader.get() == nullptr) {
			REGEN_WARN("Skipping unknown Asset for '" << input.getDescription() << "'.");
		} else {
			const std::vector<ref_ptr<Material> > materials = assetLoader->materials();
			auto materialIndex = input.getValue<uint32_t>("asset-index", 0u);
			if (materialIndex >= materials.size()) {
				REGEN_WARN("Invalid Material index '" << materialIndex <<
													  "' for Asset '" << input.getValue("asset") << "'.");
			} else {
				mat = materials[materialIndex];
			}
		}
	} else if (input.hasAttribute("preset")) {
		std::string presetVal(input.getValue("preset"));
		auto variant = input.getValue<std::string>("variant", "");
		if (presetVal == "jade") mat->set_jade(variant);
		else if (presetVal == "ruby") mat->set_ruby(variant);
		else if (presetVal == "chrome") mat->set_chrome(variant);
		else if (presetVal == "gold") mat->set_gold(variant);
		else if (presetVal == "copper") mat->set_copper(variant);
		else if (presetVal == "silver") mat->set_silver(variant);
		else if (presetVal == "pewter") mat->set_pewter(variant);
		else if (presetVal == "iron") mat->set_iron(variant);
		else if (presetVal == "steel") mat->set_steel(variant);
		else if (presetVal == "metal") mat->set_metal(variant);
		else if (presetVal == "leather") mat->set_leather(variant);
		else if (presetVal == "stone") mat->set_stone(variant);
		else if (presetVal == "wood") mat->set_wood(variant);
		else if (presetVal == "marble") mat->set_marble(variant);
		else {
			mat->set_textures(presetVal, variant);
		}
	}
	if (input.hasAttribute("diffuse"))
		mat->diffuse()->setVertex(0,
								  input.getValue<Vec3f>("diffuse", Vec3f::one()));
	if (input.hasAttribute("specular"))
		mat->specular()->setVertex(0,
								   input.getValue<Vec3f>("specular", Vec3f::zero()));
	if (input.hasAttribute("shininess"))
		mat->shininess()->setVertex(0,
									input.getValue<float>("shininess", 1.0f));
	if (input.hasAttribute("emission"))
		mat->set_emission(input.getValue<Vec3f>("emission", Vec3f::zero()));
	if (input.hasAttribute("textures")) {
		mat->set_textures(
				input.getValue("textures"),
				input.getValue<std::string>("variant", ""));
	}

	mat->alpha()->setVertex(0,
							input.getValue<float>("alpha", 1.0f));
	mat->refractionIndex()->setVertex(0,
									  input.getValue<float>("refractionIndex", 0.95f));
	mat->set_fillMode(glenum::fillMode(
			input.getValue<std::string>("fill-mode", "FILL")));
	if (input.getValue<bool>("two-sided", false)) {
		// this conflicts with shadow mapping front face culling.
		mat->set_twoSided(true);
	}

	return mat;
}
