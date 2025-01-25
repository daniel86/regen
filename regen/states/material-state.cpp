/*
 * material.cpp
 *
 *  Created on: 22.03.2011
 *      Author: daniel
 */

#include <boost/filesystem.hpp>
#include "texture-state.h"
#include "atomic-states.h"

#include "material-state.h"
#include "regen/utility/filesystem.h"
#include "regen/textures/texture-loader.h"

using namespace regen;

Material::Material()
		: HasInputState(VBO::USAGE_DYNAMIC),
		  fillMode_(GL_FILL),
		  mipmapFlag_(GL_DONT_CARE),
		  forcedType_(GL_NONE),
		  forcedInternalFormat_(GL_NONE),
		  forcedFormat_(GL_NONE),
		  forcedSize_(0u),
		  maxOffset_(0.1f),
		  heightMapMode_(HEIGHT_MAP_VERTEX),
		  colorBlendMode_(BLEND_MODE_SRC),
		  colorBlendFactor_(1.0f) {
	materialUniforms_ = ref_ptr<UniformBlock>::alloc("Material");
	setInput(materialUniforms_);

	materialSpecular_ = ref_ptr<ShaderInput3f>::alloc("matSpecular");
	materialSpecular_->setUniformData(Vec3f(0.0f));
	materialUniforms_->addUniform(materialSpecular_);

	materialShininess_ = ref_ptr<ShaderInput1f>::alloc("matShininess");
	materialShininess_->setUniformData(128.0f);
	materialUniforms_->addUniform(materialShininess_);

	materialDiffuse_ = ref_ptr<ShaderInput3f>::alloc("matDiffuse");
	materialDiffuse_->setUniformData(Vec3f(1.0f));
	materialUniforms_->addUniform(materialDiffuse_);

	materialAlpha_ = ref_ptr<ShaderInput1f>::alloc("matAlpha");
	materialAlpha_->setUniformData(1.0f);
	materialUniforms_->addUniform(materialAlpha_);

	materialAmbient_ = ref_ptr<ShaderInput3f>::alloc("matAmbient");
	materialAmbient_->setUniformData(Vec3f(0.0f));
	materialUniforms_->addUniform(materialAmbient_);

	materialRefractionIndex_ = ref_ptr<ShaderInput1f>::alloc("matRefractionIndex");
	materialRefractionIndex_->setUniformData(0.95f);
	materialUniforms_->addUniform(materialRefractionIndex_);

	shaderDefine("HAS_MATERIAL", "TRUE");
}

void Material::set_emission(const Vec3f &emission) {
	if (materialEmission_.get() == nullptr) {
		materialEmission_ = ref_ptr<ShaderInput3f>::alloc("matEmission");
		materialEmission_->setUniformData(emission);
		// TODO: better always include emission term in material UBO?
		materialUniforms_->addUniform(materialEmission_);
	} else {
		materialEmission_->setUniformData(emission);
	}
}

void Material::set_fillMode(GLenum fillMode) {
	if (fillMode == fillMode_) return;
	disjoinStates(fillModeState_);
	fillMode_ = fillMode;
	fillModeState_ = ref_ptr<FillModeState>::alloc(fillMode_);
	joinStates(fillModeState_);
}

void Material::set_twoSided(GLboolean twoSided) {
	if (twoSidedState_.get()) {
		disjoinStates(twoSidedState_);
	}
	twoSidedState_ = ref_ptr<ToggleState>::alloc(RenderState::CULL_FACE, twoSided);
	joinStates(twoSidedState_);
	shaderDefine("HAS_TWO_SIDES", twoSided ? "TRUE" : "FALSE");
}

void Material::set_maxOffset(GLfloat offset) {
	maxOffset_ = offset;
	auto heightMaps = textures_.find(TextureState::MAP_TO_HEIGHT);
	if (heightMaps != textures_.end()) {
		for (auto &tex: heightMaps->second) {
			tex->set_blendFactor(offset);
		}
	}
}

void Material::set_texture(const ref_ptr<TextureState> &texState, TextureState::MapTo mapTo) {
	texState->set_mapping(TextureState::MAPPING_TEXCO);
	texState->set_mapTo(TextureState::MAP_TO_CUSTOM);
	switch(mapTo) {
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
				}
				else if (heightMapMode_ == HEIGHT_MAP_PARALLAX) {
					texState->set_texcoTransfer(TextureState::TRANSFER_TEXCO_PARALLAX);
				}
				else if (heightMapMode_ == HEIGHT_MAP_PARALLAX_OCCLUSION) {
					texState->set_texcoTransfer(TextureState::TRANSFER_TEXCO_PARALLAX_OCC);
				}
			}
			break;
	}
	joinStates(texState);
}

bool Material::getMapTo(std::string_view fileName, TextureState::MapTo &mapTo) {
	if (fileName.find("diffuse") != std::string::npos) {
		mapTo = TextureState::MAP_TO_DIFFUSE;
	}
	else if (fileName.find("color") != std::string::npos) {
		mapTo = TextureState::MAP_TO_COLOR;
	}
	else if (fileName.find("ambient") != std::string::npos) {
		mapTo = TextureState::MAP_TO_AMBIENT;
	}
	else if (fileName.find("specular") != std::string::npos) {
		mapTo = TextureState::MAP_TO_SPECULAR;
	}
	else if (fileName.find("emission") != std::string::npos) {
		mapTo = TextureState::MAP_TO_EMISSION;
	}
	else if (fileName.find("normal") != std::string::npos) {
		mapTo = TextureState::MAP_TO_NORMAL;
	}
	else if (fileName.find("height") != std::string::npos) {
		mapTo = TextureState::MAP_TO_HEIGHT;
	}
	else if (fileName.find("displacement") != std::string::npos) {
		mapTo = TextureState::MAP_TO_DISPLACEMENT;
	}
	else if (fileName.find("displace") != std::string::npos) {
		mapTo = TextureState::MAP_TO_DISPLACEMENT;
	}
	else if (fileName.find("occlusion") != std::string::npos) {
		mapTo = TextureState::MAP_TO_LIGHT;
	}
	else {
		return false;
	}
	return true;

}

bool Material::set_textures(std::string_view materialName, Variant variant) {
	return set_textures(materialName, REGEN_STRING(variant));
}

bool Material::set_textures(std::string_view materialName, std::string_view variant) {
	// find the base path with the textures
	auto basePath0 = REGEN_STRING("res/textures/materials/" << materialName << "/" << variant);
	basePath0 = resourcePath(basePath0);
	if (!boost::filesystem::exists(basePath0)) {
		basePath0 = REGEN_STRING("res/textures/" << materialName << "/" << variant);
		basePath0 = resourcePath(basePath0);
	}
	if (!boost::filesystem::exists(basePath0)) {
		return false;
	}

	// iterate over the texture files in the directory
	for (auto &entry: boost::filesystem::directory_iterator(basePath0)) {
		auto &filePath = entry.path().string();
		auto fileName = entry.path().filename().string();
		TextureState::MapTo mapTo = TextureState::MAP_TO_CUSTOM;
		if (!getMapTo(fileName, mapTo)) {
			continue;
		}
		auto tex = textures::load(
				filePath,
				mipmapFlag_,
				forcedInternalFormat_,
				forcedFormat_,
				forcedType_,
				forcedSize_);
		if (tex.get() != nullptr) {
			auto texName = REGEN_STRING("materialTexture" << textures_.size());
			auto texState = ref_ptr<TextureState>::alloc(tex, texName);
			textures_[mapTo].push_back(texState);

			if (wrapping_.has_value()) {
				tex->begin(RenderState::get(), 0);
				tex->wrapping().push(GL_CLAMP_TO_EDGE);
				tex->end(RenderState::get());
			}
		}
	}

	for (auto &pair: textures_) {
		for (auto &tex : pair.second) {
			set_texture(tex, pair.first);
		}
	}
	return true;
}

void Material::set_iron(Variant variant) {
	materialDiffuse_->setUniformData(Vec3f(0.1843137, 0.168627, 0.15686) * 3.0f);
	materialAmbient_->setUniformData(Vec3f(0.19, 0.19, 0.19));
	materialSpecular_->setUniformData(Vec3f(0.11, 0.11, 0.11));
	materialShininess_->setUniformData(9.8);
	set_textures("iron", variant);
}

void Material::set_steel(Variant variant) {
	materialDiffuse_->setUniformData(Vec3f(0.423529, 0.439216, 0.450980));
	materialAmbient_->setUniformData(Vec3f(0.14, 0.14, 0.14));
	materialSpecular_->setUniformData(Vec3f(0.21, 0.21, 0.21));
	materialShininess_->setUniformData(21.2);
	set_textures("steel", variant);
}

void Material::set_silver(Variant variant) {
	materialAmbient_->setUniformData(Vec3f(0.19, 0.19, 0.19));
	materialDiffuse_->setUniformData(Vec3f(0.51, 0.51, 0.51));
	materialSpecular_->setUniformData(Vec3f(0.51, 0.51, 0.51));
	materialShininess_->setUniformData(51.2);
	set_textures("silver", variant);
}

void Material::set_pewter(Variant variant) {
	materialAmbient_->setUniformData(Vec3f(0.11, 0.06, 0.11));
	materialDiffuse_->setUniformData(Vec3f(0.43, 0.47, 0.54));
	materialSpecular_->setUniformData(Vec3f(0.33, 0.33, 0.52));
	materialShininess_->setUniformData(9.8);
	set_textures("silver", variant);
}

void Material::set_gold(Variant variant) {
	materialAmbient_->setUniformData(Vec3f(0.25, 0.20, 0.07));
	materialDiffuse_->setUniformData(Vec3f(0.75, 0.61, 0.23));
	materialSpecular_->setUniformData(Vec3f(0.63, 0.65, 0.37));
	materialShininess_->setUniformData(51.2);
	set_textures("gold", variant);
}

void Material::set_copper(Variant variant) {
	materialAmbient_->setUniformData(Vec3f(0.19, 0.07, 0.02));
	materialDiffuse_->setUniformData(Vec3f(0.70, 0.27, 0.08));
	materialSpecular_->setUniformData(Vec3f(0.26, 0.14, 0.09));
	materialShininess_->setUniformData(12.8);
	set_textures("copper", variant);
}

void Material::set_metal(Variant variant) {
	materialDiffuse_->setUniformData(Vec3f(0.423529, 0.439216, 0.450980));
	materialAmbient_->setUniformData(Vec3f(0.14, 0.14, 0.14));
	materialSpecular_->setUniformData(Vec3f(0.21, 0.21, 0.21));
	materialShininess_->setUniformData(21.2);
	set_textures("metal", variant);
}

void Material::set_leather(Variant variant) {
	materialDiffuse_->setUniformData(Vec3f(0.37647, 0.3098, 0.23529));
	materialAmbient_->setUniformData(Vec3f(0.1647, 0.1216, 0.0745));
	materialSpecular_->setUniformData(Vec3f(0.21, 0.21, 0.21));
	materialShininess_->setUniformData(21.2);
	set_textures("leather", variant);
}

void Material::set_stone(Variant variant) {
	materialDiffuse_->setUniformData(Vec3f(0.57647, 0.572549, 0.592157));
	materialAmbient_->setUniformData(Vec3f(0.0647, 0.0647, 0.0647));
	materialSpecular_->setUniformData(Vec3f(0.14, 0.14, 0.14));
	materialShininess_->setUniformData(52.2);
	set_textures("stone", variant);
}

void Material::set_marble(Variant variant) {
    materialAmbient_->setUniformData(Vec3f(0.2f, 0.2f, 0.2f));
    materialDiffuse_->setUniformData(Vec3f(0.8f, 0.8f, 0.8f));
    materialSpecular_->setUniformData(Vec3f(0.9f, 0.9f, 0.9f));
    materialShininess_->setUniformData(80.0f);
	set_textures("marble", variant);
}

void Material::set_wood(Variant variant) {
    materialAmbient_->setUniformData(Vec3f(0.2f, 0.1f, 0.05f)); // Dark brown ambient color
    materialDiffuse_->setUniformData(Vec3f(0.6f, 0.3f, 0.1f));  // Brown diffuse color
    materialSpecular_->setUniformData(Vec3f(0.2f, 0.2f, 0.2f));
    materialShininess_->setUniformData(25.0f);
	set_textures("wood", variant);
}

void Material::set_jade(Variant variant) {
	materialAmbient_->setUniformData(Vec3f(0.14, 0.22, 0.16));
	materialDiffuse_->setUniformData(Vec3f(0.54, 0.89, 0.63));
	materialSpecular_->setUniformData(Vec3f(0.32, 0.32, 0.32));
	materialShininess_->setUniformData(12.8);
	set_textures("jade", variant);
}

void Material::set_ruby(Variant variant) {
	materialAmbient_->setUniformData(Vec3f(0.17, 0.01, 0.01));
	materialDiffuse_->setUniformData(Vec3f(0.61, 0.04, 0.04));
	materialSpecular_->setUniformData(Vec3f(0.73, 0.63, 0.63));
	materialShininess_->setUniformData(76.8);
	set_textures("ruby", variant);
}

void Material::set_chrome(Variant variant) {
	materialAmbient_->setUniformData(Vec3f(0.25, 0.25, 0.25));
	materialDiffuse_->setUniformData(Vec3f(0.40, 0.40, 0.40));
	materialSpecular_->setUniformData(Vec3f(0.77, 0.77, 0.77));
	materialShininess_->setUniformData(76.8);
	set_textures("chrome", variant);
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
