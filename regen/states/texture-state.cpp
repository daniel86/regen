/*
 * texture-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <regen/utility/string-util.h>

#include "texture-state.h"

namespace regen {
	std::ostream &operator<<(std::ostream &out, const TextureState::Mapping &mode) {
		switch (mode) {
			case TextureState::MAPPING_FLAT:
				return out << "flat";
			case TextureState::MAPPING_CUBE:
				return out << "cube";
			case TextureState::MAPPING_TUBE:
				return out << "tube";
			case TextureState::MAPPING_SPHERE:
				return out << "sphere";
			case TextureState::MAPPING_CUBE_REFLECTION:
				return out << "cube_reflection";
			case TextureState::MAPPING_REFRACTION:
				return out << "refraction";
			case TextureState::MAPPING_CUBE_REFRACTION:
				return out << "cube_refraction";
			case TextureState::MAPPING_PLANAR_REFLECTION:
				return out << "planar_reflection";
			case TextureState::MAPPING_PARABOLOID_REFLECTION:
				return out << "paraboloid_reflection";
			case TextureState::MAPPING_CUSTOM:
				return out << "custom";
			case TextureState::MAPPING_TEXCO:
				return out << "texco";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, TextureState::Mapping &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "flat") mode = TextureState::MAPPING_FLAT;
		else if (val == "cube") mode = TextureState::MAPPING_CUBE;
		else if (val == "tube") mode = TextureState::MAPPING_TUBE;
		else if (val == "sphere") mode = TextureState::MAPPING_SPHERE;
		else if (val == "cube_reflection") mode = TextureState::MAPPING_CUBE_REFLECTION;
		else if (val == "cube_refraction") mode = TextureState::MAPPING_CUBE_REFRACTION;
		else if (val == "refraction") mode = TextureState::MAPPING_REFRACTION;
		else if (val == "planar_reflection") mode = TextureState::MAPPING_PLANAR_REFLECTION;
		else if (val == "paraboloid_reflection") mode = TextureState::MAPPING_PARABOLOID_REFLECTION;
		else if (val == "texco") mode = TextureState::MAPPING_TEXCO;
		else if (val == "custom") mode = TextureState::MAPPING_CUSTOM;
		else {
			REGEN_WARN("Unknown Texture Mapping '" << val <<
												   "'. Using default CUSTOM Mapping.");
			mode = TextureState::MAPPING_CUSTOM;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const TextureState::MapTo &mode) {
		switch (mode) {
			case TextureState::MAP_TO_COLOR:
				return out << "COLOR";
			case TextureState::MAP_TO_DIFFUSE:
				return out << "DIFFUSE";
			case TextureState::MAP_TO_AMBIENT:
				return out << "AMBIENT";
			case TextureState::MAP_TO_SPECULAR:
				return out << "SPECULAR";
			case TextureState::MAP_TO_SHININESS:
				return out << "SHININESS";
			case TextureState::MAP_TO_EMISSION:
				return out << "EMISSION";
			case TextureState::MAP_TO_LIGHT:
				return out << "LIGHT";
			case TextureState::MAP_TO_ALPHA:
				return out << "ALPHA";
			case TextureState::MAP_TO_NORMAL:
				return out << "NORMAL";
			case TextureState::MAP_TO_HEIGHT:
				return out << "HEIGHT";
			case TextureState::MAP_TO_DISPLACEMENT:
				return out << "DISPLACEMENT";
			case TextureState::MAP_TO_CUSTOM:
				return out << "CUSTOM";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, TextureState::MapTo &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "COLOR") mode = TextureState::MAP_TO_COLOR;
		else if (val == "DIFFUSE") mode = TextureState::MAP_TO_DIFFUSE;
		else if (val == "AMBIENT") mode = TextureState::MAP_TO_AMBIENT;
		else if (val == "SPECULAR") mode = TextureState::MAP_TO_SPECULAR;
		else if (val == "SHININESS") mode = TextureState::MAP_TO_SHININESS;
		else if (val == "EMISSION") mode = TextureState::MAP_TO_EMISSION;
		else if (val == "LIGHT") mode = TextureState::MAP_TO_LIGHT;
		else if (val == "ALPHA") mode = TextureState::MAP_TO_ALPHA;
		else if (val == "NORMAL") mode = TextureState::MAP_TO_NORMAL;
		else if (val == "HEIGHT") mode = TextureState::MAP_TO_HEIGHT;
		else if (val == "DISPLACEMENT") mode = TextureState::MAP_TO_DISPLACEMENT;
		else if (val == "CUSTOM") mode = TextureState::MAP_TO_CUSTOM;
		else {
			REGEN_WARN("Unknown Texture Map-To '" << val <<
												  "'. Using default CUSTOM Map-To.");
			mode = TextureState::MAP_TO_CUSTOM;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const TextureState::TransferTexco &mode) {
		switch (mode) {
			case TextureState::TRANSFER_TEXCO_PARALLAX:
				return out << "PARALLAX";
			case TextureState::TRANSFER_TEXCO_PARALLAX_OCC:
				return out << "PARALLAX_OCC";
			case TextureState::TRANSFER_TEXCO_RELIEF:
				return out << "RELIEF";
			case TextureState::TRANSFER_TEXCO_FISHEYE:
				return out << "FISHEYE";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, TextureState::TransferTexco &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "PARALLAX") mode = TextureState::TRANSFER_TEXCO_PARALLAX;
		else if (val == "PARALLAX_OCC") mode = TextureState::TRANSFER_TEXCO_PARALLAX_OCC;
		else if (val == "RELIEF") mode = TextureState::TRANSFER_TEXCO_RELIEF;
		else if (val == "FISHEYE") mode = TextureState::TRANSFER_TEXCO_FISHEYE;
		else {
			REGEN_WARN("Unknown Texture Texco-Transfer '" << val <<
														  "'. Using default PARALLAX Texco-Transfer.");
			mode = TextureState::TRANSFER_TEXCO_PARALLAX;
		}
		return in;
	}
}
using namespace regen;

#define REGEN_TEX_NAME(x) REGEN_STRING(x << stateID_)

GLuint TextureState::idCounter_ = 0;

TextureState::TextureState(const ref_ptr<Texture> &texture, const std::string &name)
		: State(),
		  stateID_(++idCounter_),
		  texcoChannel_(0u),
		  ignoreAlpha_(GL_FALSE) {
	set_blendMode(BLEND_MODE_SRC);
	set_blendFactor(1.0f);
	set_mapping(MAPPING_TEXCO);
	set_mapTo(MAP_TO_CUSTOM);
	set_texture(texture);
	if (!name.empty()) {
		set_name(name);
	}
}

TextureState::TextureState()
		: State(),
		  stateID_(++idCounter_),
		  samplerType_("sampler2D"),
		  texcoChannel_(0u),
		  ignoreAlpha_(GL_FALSE) {
	set_blendMode(BLEND_MODE_SRC);
	set_blendFactor(1.0f);
	set_mapping(MAPPING_TEXCO);
	set_mapTo(MAP_TO_CUSTOM);
}

void TextureState::set_texture(const ref_ptr<Texture> &tex) {
	texture_ = tex;
	samplerType_ = tex->samplerType();
	if (tex.get()) {
		set_name(REGEN_STRING("Texture" << tex->id()));
		shaderDefine(REGEN_TEX_NAME("TEX_SAMPLER_TYPE"), tex->samplerType());
		shaderDefine(REGEN_TEX_NAME("TEX_DIM"), REGEN_STRING(tex->numComponents()));
	}
}

const ref_ptr<Texture> &TextureState::texture() const { return texture_; }

void TextureState::set_name(const std::string &name) {
	name_ = name;
	shaderDefine(REGEN_TEX_NAME("TEX_NAME"), name_);
	shaderDefine("HAS_" + name_, "TRUE");
}

const std::string &TextureState::name() const { return name_; }

void TextureState::set_samplerType(const std::string &samplerType) { samplerType_ = samplerType; }

const std::string &TextureState::samplerType() const { return samplerType_; }

GLuint TextureState::stateID() const { return stateID_; }

void TextureState::set_texcoChannel(GLuint texcoChannel) {
	texcoChannel_ = texcoChannel;
	shaderDefine(REGEN_TEX_NAME("TEX_TEXCO"), REGEN_STRING("texco" << texcoChannel_));
}

GLuint TextureState::texcoChannel() const { return texcoChannel_; }

void TextureState::set_ignoreAlpha(GLboolean v) {
	ignoreAlpha_ = v;
	shaderDefine(REGEN_TEX_NAME("TEX_IGNORE_ALPHA"), v ? "TRUE" : "FALSE");
}

GLboolean TextureState::ignoreAlpha() const { return ignoreAlpha_; }

void TextureState::set_blendFactor(GLfloat blendFactor) {
	blendFactor_ = blendFactor;
	shaderDefine(REGEN_TEX_NAME("TEX_BLEND_FACTOR"), REGEN_STRING(blendFactor_));
}

void TextureState::set_blendMode(BlendMode blendMode) {
	blendMode_ = blendMode;
	shaderDefine(REGEN_TEX_NAME("TEX_BLEND_KEY"), REGEN_STRING("regen.states.blending." << blendMode_));
	shaderDefine(REGEN_TEX_NAME("TEX_BLEND_NAME"), REGEN_STRING("blend_" << blendMode_));
}

void TextureState::set_blendFunction(const std::string &blendFunction, const std::string &blendName) {
	blendFunction_ = blendFunction;
	blendName_ = blendName;

	shaderFunction(blendName_, blendFunction_);
	shaderDefine(REGEN_TEX_NAME("TEX_BLEND_KEY"), blendName_);
	shaderDefine(REGEN_TEX_NAME("TEX_BLEND_NAME"), blendName_);
}

void TextureState::set_mapTo(MapTo id) {
	mapTo_ = id;
	shaderDefine(REGEN_TEX_NAME("TEX_MAPTO"), REGEN_STRING(mapTo_));
}

void TextureState::set_mapping(TextureState::Mapping mapping) {
	mapping_ = mapping;
	shaderDefine(REGEN_TEX_NAME("TEX_MAPPING_KEY"), REGEN_STRING("regen.states.textures.texco_" << mapping));
	shaderDefine(REGEN_TEX_NAME("TEX_MAPPING_NAME"), REGEN_STRING("texco_" << mapping));
	shaderDefine(REGEN_TEX_NAME("TEX_TEXCO"), REGEN_STRING("texco" << texcoChannel_));
}

void TextureState::set_mappingFunction(const std::string &mappingFunction, const std::string &mappingName) {
	mappingFunction_ = mappingFunction;
	mappingName_ = mappingName;

	shaderFunction(mappingName_, mappingFunction_);
	shaderDefine(REGEN_TEX_NAME("TEX_MAPPING_KEY"), mappingName_);
	shaderDefine(REGEN_TEX_NAME("TEX_MAPPING_NAME"), mappingName_);
}

///////
///////


void TextureState::set_texelTransferFunction(const std::string &transferFunction, const std::string &transferName) {
	transferKey_ = "";
	transferName_ = transferName;
	transferFunction_ = transferFunction;

	shaderFunction(transferName_, transferFunction_);
	shaderDefine(REGEN_TEX_NAME("TEX_TRANSFER_KEY"), transferName_);
	shaderDefine(REGEN_TEX_NAME("TEX_TRANSFER_NAME"), transferName_);
}

void TextureState::set_texelTransferKey(const std::string &transferKey, const std::string &transferName) {
	transferFunction_ = "";
	transferKey_ = transferKey;
	if (transferName.empty()) {
		std::list<std::string> path;
		boost::split(path, transferKey, boost::is_any_of("."));
		transferName_ = *path.rbegin();
	} else {
		transferName_ = transferName;
	}
	shaderDefine(REGEN_TEX_NAME("TEX_TRANSFER_KEY"), transferKey_);
	shaderDefine(REGEN_TEX_NAME("TEX_TRANSFER_NAME"), transferName_);
}

///////
///////

void TextureState::set_texcoTransferFunction(const std::string &transferFunction, const std::string &transferName) {
	transferTexcoKey_ = "";
	transferTexcoName_ = transferName;
	transferTexcoFunction_ = transferFunction;

	shaderFunction(transferTexcoName_, transferTexcoFunction_);
	shaderDefine(REGEN_TEX_NAME("TEXCO_TRANSFER_KEY"), transferTexcoName_);
	shaderDefine(REGEN_TEX_NAME("TEXCO_TRANSFER_NAME"), transferTexcoName_);
}

void TextureState::set_texcoTransfer(TransferTexco mode) {
	switch (mode) {
		case TRANSFER_TEXCO_FISHEYE:
			set_texcoTransferKey("regen.states.textures.fisheyeTransfer");
			break;
		case TRANSFER_TEXCO_PARALLAX:
			set_texcoTransferKey("regen.states.textures.parallaxTransfer");
			break;
		case TRANSFER_TEXCO_PARALLAX_OCC:
			set_texcoTransferKey("regen.states.textures.parallaxOcclusionTransfer");
			break;
		case TRANSFER_TEXCO_RELIEF:
			set_texcoTransferKey("regen.states.textures.reliefTransfer");
			break;
	}
}

void TextureState::set_texcoTransferKey(const std::string &transferKey, const std::string &transferName) {
	transferTexcoFunction_ = "";
	transferTexcoKey_ = transferKey;
	if (transferName.empty()) {
		std::list<std::string> path;
		boost::split(path, transferKey, boost::is_any_of("."));
		transferTexcoName_ = *path.rbegin();
	} else {
		transferTexcoName_ = transferName;
	}
	shaderDefine(REGEN_TEX_NAME("TEXCO_TRANSFER_KEY"), transferTexcoKey_);
	shaderDefine(REGEN_TEX_NAME("TEXCO_TRANSFER_NAME"), transferTexcoName_);
}

///////
///////

void TextureState::set_texcoFlipping(TexcoFlipping mode) {
	switch (mode) {
		case TEXCO_FLIP_X:
			shaderDefine(REGEN_TEX_NAME("TEX_FLIPPING_MODE"), "x");
			break;
		case TEXCO_FLIP_Y:
			shaderDefine(REGEN_TEX_NAME("TEX_FLIPPING_MODE"), "y");
			break;
		case TEXCO_FLIP_NONE:
			shaderDefine(REGEN_TEX_NAME("TEX_FLIPPING_MODE"), "none");
			break;
	}
}

///////
///////

void TextureState::enable(RenderState *rs) {
	lastTexChannel_ = texture_->channel();
	if (lastTexChannel_ == -1) {
		texture_->begin(rs, rs->reserveTextureChannel());
	}
	State::enable(rs);
}

void TextureState::disable(RenderState *rs) {
	State::disable(rs);
	if (lastTexChannel_ == -1) {
		texture_->end(rs, texture_->channel());
		rs->releaseTextureChannel();
	}
}
