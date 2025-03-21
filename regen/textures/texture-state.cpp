/*
 * texture-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <regen/utility/string-util.h>

#include "texture-state.h"
#include "texture-3d.h"
#include "texture-2d.h"

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
			case TextureState::MAPPING_INSTANCE_REFRACTION:
				return out << "instance_refraction";
			case TextureState::MAPPING_PLANAR_REFLECTION:
				return out << "planar_reflection";
			case TextureState::MAPPING_PARABOLOID_REFLECTION:
				return out << "paraboloid_reflection";
			case TextureState::MAPPING_CUSTOM:
				return out << "custom";
			case TextureState::MAPPING_TEXCO:
				return out << "texco";
			case TextureState::MAPPING_XZ_PLANE:
				return out << "xz_plane";
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
		else if (val == "instance_refraction") mode = TextureState::MAPPING_INSTANCE_REFRACTION;
		else if (val == "planar_reflection") mode = TextureState::MAPPING_PLANAR_REFLECTION;
		else if (val == "paraboloid_reflection") mode = TextureState::MAPPING_PARABOLOID_REFLECTION;
		else if (val == "texco") mode = TextureState::MAPPING_TEXCO;
		else if (val == "custom") mode = TextureState::MAPPING_CUSTOM;
		else if (val == "xz_plane") mode = TextureState::MAPPING_XZ_PLANE;
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
			case TextureState::MAP_TO_VERTEX_MASK:
				return out << "VERTEX_MASK";
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
		else if (val == "VERTEX_MASK") mode = TextureState::MAP_TO_VERTEX_MASK;
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
			case TextureState::TRANSFER_TEXCO_NOISE:
				return out << "NOISE";
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
		else if (val == "NOISE") mode = TextureState::TRANSFER_TEXCO_NOISE;
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
		shaderDefine(REGEN_TEX_NAME("TEX_TEXEL_X"), REGEN_STRING(1.0 / tex->width()));
		shaderDefine(REGEN_TEX_NAME("TEX_TEXEL_Y"), REGEN_STRING(1.0 / tex->height()));
		shaderDefine(REGEN_TEX_NAME("TEX_WIDTH"), REGEN_STRING(tex->width()));
		shaderDefine(REGEN_TEX_NAME("TEX_HEIGHT"), REGEN_STRING(tex->height()));

		auto tex3d = dynamic_cast<Texture3D *>(tex.get());
		if (tex3d) {
			shaderDefine(REGEN_TEX_NAME("TEX_DEPTH"), REGEN_STRING(tex3d->depth()));
		}
	}
}

void TextureState::set_name(const std::string &name) {
	name_ = name;
	shaderDefine(REGEN_TEX_NAME("TEX_NAME"), name_);
	shaderDefine("HAS_" + name_, "TRUE");
}

void TextureState::set_texcoChannel(GLuint texcoChannel) {
	texcoChannel_ = texcoChannel;
	shaderDefine(REGEN_TEX_NAME("TEX_TEXCO"), REGEN_STRING("texco" << texcoChannel_));
}

void TextureState::set_ignoreAlpha(GLboolean v) {
	ignoreAlpha_ = v;
	shaderDefine(REGEN_TEX_NAME("TEX_IGNORE_ALPHA"), v ? "TRUE" : "FALSE");
}

void TextureState::set_discardAlpha(bool v, float threshold) {
	if (v) {
		shaderDefine("DISCARD_ALPHA", "TRUE");
		shaderDefine("DISCARD_ALPHA_THRESHOLD", REGEN_STRING(threshold));
	}
}

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
		case TRANSFER_TEXCO_NOISE:
			set_texcoTransferKey("regen.states.textures.noiseTransfer");
			break;
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

ref_ptr<Texture> TextureState::getTexture(
		scene::SceneLoader *scene,
		scene::SceneInputNode &input,
		const std::string &idKey,
		const std::string &bufferKey,
		const std::string &attachmentKey) {
	ref_ptr<Texture> tex;
	// Find the texture resource
	if (input.hasAttribute(idKey)) {
		tex = scene->getResource<Texture>(input.getValue(idKey));
	} else if (input.hasAttribute(bufferKey)) {
		ref_ptr<FBO> fbo = scene->getResource<FBO>(input.getValue(bufferKey));
		if (fbo.get() == nullptr) {
			REGEN_WARN("Unable to find FBO '" << input.getValue(bufferKey) <<
											  "' for " << input.getDescription() << ".");
			return tex;
		}
		const auto val = input.getValue<std::string>(attachmentKey, "0");
		if (val == "depth") {
			tex = fbo->depthTexture();
		}
		else if (val == "stencil") {
			if (fbo->stencilTexture().get()) {
				tex = fbo->stencilTexture();
			} else {
				tex = fbo->depthStencilTexture();
			}
		}
		else {
			std::vector<ref_ptr<Texture> > &textures = fbo->colorTextures();

			unsigned int attachment;
			std::stringstream ss(val);
			ss >> attachment;

			if (attachment < textures.size()) {
				tex = textures[attachment];
			} else {
				REGEN_WARN("Invalid attachment '" << val <<
												  "' for " << input.getDescription() << ".");
			}
		}
	}
	if (!tex.get()) {
		REGEN_WARN("No texture found for " << input.getDescription() << ".");
	}
	return tex;
}

ref_ptr<TextureState> TextureState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	ref_ptr<Texture> tex = getTexture(scene, input);
	if (tex.get() == nullptr) {
		REGEN_WARN("Skipping unidentified texture node for " << input.getDescription() << ".");
		return {};
	}
	if (input.hasAttribute("mip-level") && input.getValue<int>("mip-level", 0) > 0) {
		auto mipLevel = input.getValue<unsigned int>("mip-level", 1);
		auto mipTex = dynamic_cast<TextureMips2D *>(tex.get());
		if (mipTex) {
			auto maxLevel = mipTex->numMips();
			if (mipLevel < maxLevel) {
				tex = mipTex->mipRefs()[mipLevel - 1];
			} else {
				REGEN_WARN("Mip level " << mipLevel << " is out of range for texture '" << tex->name() << "'.");
			}
		} else {
			REGEN_WARN("Texture '" << tex->name() << "' is not a mip texture.");
		}
	}

	// Set-Up the texture state
	ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(
			tex, input.getValue("name"));

	texState->set_ignoreAlpha(
			input.getValue<bool>("ignore-alpha", false));
	texState->set_mapTo(input.getValue<TextureState::MapTo>(
			"map-to", TextureState::MAP_TO_CUSTOM));

	// Describes how a texture will be mixed with existing pixels.
	texState->set_blendMode(
			input.getValue<BlendMode>("blend-mode", BLEND_MODE_SRC));
	texState->set_blendFactor(
			input.getValue<GLfloat>("blend-factor", 1.0f));

	const std::string blendFunctionName = input.getValue("blend-function-name");
	if (input.hasAttribute("blend-function")) {
		texState->set_blendFunction(
				input.getValue("blend-function"),
				blendFunctionName);
	}

	// Defines how a texture should be mapped on geometry.
	texState->set_mapping(input.getValue<TextureState::Mapping>(
			"mapping", TextureState::MAPPING_TEXCO));

	const std::string mappingFunctionName = input.getValue("mapping-function-name");
	if (input.hasAttribute("mapping-function")) {
		texState->set_mappingFunction(
				input.getValue("mapping-function"),
				mappingFunctionName);
	}

	// texel transfer wraps sampled texels before returning them.
	const std::string texelTransferName = input.getValue("texel-transfer-name");
	if (input.hasAttribute("texel-transfer-key")) {
		texState->set_texelTransferKey(
				input.getValue("texel-transfer-key"),
				texelTransferName);
	} else if (input.hasAttribute("texel-transfer-function")) {
		texState->set_texelTransferFunction(
				input.getValue("texel-transfer-function"),
				texelTransferName);
	}

	// texel transfer wraps computed texture coordinates before returning them.
	if (input.hasAttribute("texco-transfer")) {
		texState->set_texcoTransfer(input.getValue<TextureState::TransferTexco>(
				"texco-transfer", TextureState::TRANSFER_TEXCO_RELIEF));
	}
	if (input.hasAttribute("texco-flipping")) {
		auto flipModeName = input.getValue("texco-flipping");
		if (flipModeName == "x") {
			texState->set_texcoFlipping(TextureState::TEXCO_FLIP_X);
		} else if (flipModeName == "y") {
			texState->set_texcoFlipping(TextureState::TEXCO_FLIP_Y);
		} else {
			texState->set_texcoFlipping(TextureState::TEXCO_FLIP_NONE);
		}
	}
	if (input.hasAttribute("sampler-type")) {
		texState->set_samplerType(input.getValue("sampler-type"));
	}
	const std::string texcoTransferName = input.getValue("texco-transfer-name");
	if (input.hasAttribute("texco-transfer-key")) {
		texState->set_texcoTransferKey(
				input.getValue("texco-transfer-key"),
				texcoTransferName);
	} else if (input.hasAttribute("texco-transfer-function")) {
		texState->set_texcoTransferFunction(
				input.getValue("texco-transfer-function"),
				texcoTransferName);
	}

	return texState;
}

ref_ptr<State> TextureIndexState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	const std::string texName = input.getValue("name");

	ref_ptr<Texture> tex = TextureState::getTexture(ctx.scene(), input);
	if (tex.get() == nullptr) {
		REGEN_WARN("Skipping unidentified texture node for " << input.getDescription() << ".");
		return {};
	}

	if (input.hasAttribute("value")) {
		auto index = input.getValue<GLuint>("index", 0u);
		return ref_ptr<TextureSetIndex>::alloc(tex, index);
	} else if (input.getValue<bool>("set-next-index", true)) {
		return ref_ptr<TextureNextIndex>::alloc(tex);
	} else {
		REGEN_WARN("Skipping " << input.getDescription() << " because no index set.");
		return {};
	}
}
