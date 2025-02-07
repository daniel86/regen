/*
 * gl-enum.cpp
 *
 *  Created on: 24.02.2013
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <regen/utility/logging.h>

#include "gl-enum.h"

using namespace regen;

static const GLenum glslStages__[] = {
		GL_VERTEX_SHADER
#ifdef GL_TESS_CONTROL_SHADER
		, GL_TESS_CONTROL_SHADER
#endif
#ifdef GL_TESS_EVALUATION_SHADER
		, GL_TESS_EVALUATION_SHADER
#endif
		, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER
#ifdef GL_COMPUTE_SHADER
		, GL_COMPUTE_SHADER
#endif
};
static const GLint glslStageCount__ = sizeof(glslStages__) / sizeof(GLenum);

static std::string getValue(const std::string &in) {
	std::string val = in;
	boost::to_upper(val);
	if (boost::starts_with(val, "GL_")) {
		val = val.substr(3);
	}
	return val;
}

const GLenum *glenum::glslStages() { return glslStages__; }

GLint glenum::glslStageCount() { return glslStageCount__; }

std::string glenum::glslStageName(GLenum stage) {
	switch (stage) {
		case GL_NONE:
			return "NONE";
		case GL_VERTEX_SHADER:
			return "VERTEX_SHADER";
#ifdef GL_TESS_CONTROL_SHADER
		case GL_TESS_CONTROL_SHADER:
			return "TESS_CONTROL_SHADER";
#endif
#ifdef GL_TESS_EVALUATION_SHADER
		case GL_TESS_EVALUATION_SHADER:
			return "TESS_EVALUATION_SHADER";
#endif
		case GL_GEOMETRY_SHADER:
			return "GEOMETRY_SHADER";
		case GL_FRAGMENT_SHADER:
			return "FRAGMENT_SHADER";
#ifdef GL_COMPUTE_SHADER
		case GL_COMPUTE_SHADER:
			return "COMPUTE_SHADER";
#endif
		default:
			return "UNKNOWN_SHADER";
	}
}

std::string glenum::glslStagePrefix(GLenum stage) {
	switch (stage) {
		case GL_VERTEX_SHADER:
			return "vs";
#ifdef GL_TESS_CONTROL_SHADER
		case GL_TESS_CONTROL_SHADER:
			return "tcs";
#endif
#ifdef GL_TESS_EVALUATION_SHADER
		case GL_TESS_EVALUATION_SHADER:
			return "tes";
#endif
		case GL_GEOMETRY_SHADER:
			return "gs";
		case GL_FRAGMENT_SHADER:
			return "fs";
#ifdef GL_COMPUTE_SHADER
		case GL_COMPUTE_SHADER:
			return "cs";
#endif
		default:
			return "unk";
	}
}

std::string glenum::glslDataType(GLenum pixelType, GLuint valsPerElement) {
	switch (pixelType) {
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
		case GL_INT:
			switch (valsPerElement) {
				case 1:
					return "int";
				case 2:
					return "ivec2";
				case 3:
					return "ivec3";
				case 4:
					return "ivec4";
			}
			break;
		case GL_UNSIGNED_INT:
			switch (valsPerElement) {
				case 1:
					return "uint";
				case 2:
					return "uvec2";
				case 3:
					return "uvec3";
				case 4:
					return "uvec4";
			}
			break;
		case GL_FLOAT:
		case GL_DOUBLE:
		default:
			switch (valsPerElement) {
				case 1:
					return "float";
				case 2:
					return "vec2";
				case 3:
					return "vec3";
				case 4:
					return "vec4";
				case 9:
					return "mat3";
				case 16:
					return "mat4";
			}
			break;
	}
	return "unk";
}

GLenum glenum::cubeMapLayer(GLuint layer) {
	const GLenum cubeMapLayer[] = {
			GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
	};
	return cubeMapLayer[layer];
}

GLenum glenum::compareFunction(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "NEVER") return GL_NEVER;
	else if (val == "GREATER") return GL_GREATER;
	else if (val == "NOTEQUAL") return GL_NOTEQUAL;
	else if (val == "GEQUAL") return GL_GEQUAL;
	else if (val == "ALWAYS") return GL_ALWAYS;
	else if (val == "LEQUAL") return GL_LEQUAL;
	else if (val == "LESS") return GL_LESS;
	else if (val == "EQUAL") return GL_EQUAL;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown compare function '" << val_ << "'. Using default LEQUAL.");
	return GL_LEQUAL;
}

GLenum glenum::compareMode(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "COMPARE_R_TO_TEXTURE") return GL_COMPARE_R_TO_TEXTURE;
	else if (val == "COMPARE_REF_TO_TEXTURE") return GL_COMPARE_REF_TO_TEXTURE;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown compare mode '" << val_ << "'. Using default NONE.");
	return GL_NONE;
}

GLenum glenum::cullFace(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "FRONT") return GL_FRONT;
	else if (val == "BACK") return GL_BACK;
	else if (val == "FRONT_AND_BACK") return GL_FRONT_AND_BACK;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown cull face '" << val_ << "'. Using default FRONT.");
	return GL_FRONT;
}

GLenum glenum::frontFace(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "CCW") return GL_CCW;
	else if (val == "CW") return GL_CW;
	REGEN_WARN("Unknown front face '" << val_ << "'. Using default GL_CCW.");
	return GL_CCW;
}

GLenum glenum::pixelType(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "HALF_FLOAT") return GL_HALF_FLOAT;
	else if (val == "FLOAT") return GL_FLOAT;
	else if (val == "UNSIGNED_BYTE") return GL_UNSIGNED_BYTE;
	else if (val == "BYTE") return GL_BYTE;
	else if (val == "SHORT") return GL_SHORT;
	else if (val == "UNSIGNED_SHORT") return GL_UNSIGNED_SHORT;
	else if (val == "INT") return GL_INT;
	else if (val == "UNSIGNED_INT") return GL_UNSIGNED_INT;
	else if (val == "DOUBLE") return GL_DOUBLE;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown pixel type '" << val_ << "'. Using default UNSIGNED_BYTE.");
	return GL_UNSIGNED_BYTE;
}

GLuint glenum::pixelComponents(GLenum format) {
	switch (format) {
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_LUMINANCE:
		case GL_DEPTH_COMPONENT:
			return 1;
		case GL_RG:
		case GL_LUMINANCE_ALPHA:
			return 2;
		case GL_RGB:
			return 3;
		case GL_RGBA:
			return 4;
		default:
			return 1;
	}
}

GLenum glenum::fillMode(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "FILL") return GL_FILL;
	else if (val == "LINE") return GL_LINE;
	else if (val == "POINT") return GL_POINT;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown fill mode '" << val_ << "'. Using default GL_FILL.");
	return GL_FILL;
}

GLenum glenum::drawBuffer(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "FRONT") return GL_FRONT;
	else if (val == "BACK") return GL_BACK;
	else if (val == "FRONT_AND_BACK") return GL_FRONT_AND_BACK;
	REGEN_WARN("Unknown draw buffer '" << val_ << "'. Using default GL_FRONT.");
	return GL_FILL;
}

GLenum glenum::primitive(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "PATCHES") return GL_PATCHES;
	else if (val == "POINTS") return GL_POINTS;
	else if (val == "LINES") return GL_LINES;
	else if (val == "LINE_LOOP") return GL_LINE_LOOP;
	else if (val == "LINE_STRIP") return GL_LINE_STRIP;
	else if (val == "LINES_ADJACENCY") return GL_LINES_ADJACENCY;
	else if (val == "LINE_STRIP_ADJACENCY") return GL_LINE_STRIP_ADJACENCY;
	else if (val == "TRIANGLES") return GL_TRIANGLES;
	else if (val == "TRIANGLE_FAN") return GL_TRIANGLE_FAN;
	else if (val == "TRIANGLE_STRIP") return GL_TRIANGLE_STRIP;
	else if (val == "TRIANGLES_ADJACENCY") return GL_TRIANGLES_ADJACENCY;
	else if (val == "TRIANGLE_STRIP_ADJACENCY") return GL_TRIANGLE_STRIP_ADJACENCY;
	REGEN_WARN("Unknown fill mode '" << val_ << "'. Using default GL_TRIANGLES.");
	return GL_TRIANGLES;
}

GLenum glenum::blendFunction(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "ADD") return GL_FUNC_ADD;
	else if (val == "SUBTRACT") return GL_FUNC_SUBTRACT;
	else if (val == "REVERSE_SUBTRACT") return GL_FUNC_REVERSE_SUBTRACT;
	else if (val == "MIN") return GL_MIN;
	else if (val == "MAX") return GL_MAX;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown blend function '" << val_ << "'. Using default GL_FUNC_ADD.");
	return GL_FUNC_ADD;
}

GLenum glenum::filterMode(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "NEAREST") return GL_NEAREST;
	else if (val == "LINEAR") return GL_LINEAR;
	else if (val == "NEAREST_MIPMAP_NEAREST") return GL_NEAREST_MIPMAP_NEAREST;
	else if (val == "LINEAR_MIPMAP_NEAREST") return GL_LINEAR_MIPMAP_NEAREST;
	else if (val == "NEAREST_MIPMAP_LINEAR") return GL_NEAREST_MIPMAP_LINEAR;
	else if (val == "LINEAR_MIPMAP_LINEAR") return GL_LINEAR_MIPMAP_LINEAR;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown filter mode '" << val_ << "'. Using default GL_LINEAR.");
	return GL_LINEAR;
}

GLenum glenum::wrappingMode(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "CLAMP") return GL_CLAMP;
	else if (val == "CLAMP_TO_BORDER") return GL_CLAMP_TO_BORDER;
	else if (val == "CLAMP_TO_EDGE") return GL_CLAMP_TO_EDGE;
	else if (val == "MIRRORED_REPEAT") return GL_MIRRORED_REPEAT;
	else if (val == "REPEAT") return GL_REPEAT;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown wrapping mode '" << val_ << "'. Using default GL_CLAMP.");
	return GL_CLAMP;
}

GLenum glenum::textureFormat(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "RED") return GL_RED;
	else if (val == "RG") return GL_RG;
	else if (val == "RGB") return GL_RGB;
	else if (val == "RGBA") return GL_RGBA;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown texture format mode '" << val_ << "'. Using default GL_RGBA.");
	return GL_RGBA;
}

GLenum glenum::textureTarget(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "TEXTURE_1D") return GL_TEXTURE_1D;
	else if (val == "TEXTURE_1D_ARRAY") return GL_TEXTURE_1D_ARRAY;
	else if (val == "TEXTURE_2D") return GL_TEXTURE_2D;
	else if (val == "TEXTURE_2D_ARRAY") return GL_TEXTURE_2D_ARRAY;
	else if (val == "TEXTURE_2D_MULTISAMPLE") return GL_TEXTURE_2D_MULTISAMPLE;
	else if (val == "TEXTURE_3D") return GL_TEXTURE_3D;
	else if (val == "TEXTURE_CUBE_MAP") return GL_TEXTURE_CUBE_MAP;
	else if (val == "TEXTURE_DEPTH") return GL_TEXTURE_DEPTH;
#ifdef GL_TEXTURE_SHADOW
	else if (val == "TEXTURE_SHADOW") return GL_TEXTURE_SHADOW;
#endif
	REGEN_WARN("Unknown texture target mode '" << val_ << "'. Using default GL_TEXTURE_2D.");
	return GL_TEXTURE_2D;
}

GLenum glenum::textureSwizzle(const std::string &val_) {
	std::string val = getValue(val_);
	if (val == "RED") return GL_RED;
	else if (val == "GREEN") return GL_GREEN;
	else if (val == "BLUE") return GL_BLUE;
	else if (val == "ALPHA") return GL_ALPHA;
	else if (val == "ZERO") return GL_ZERO;
	else if (val == "ONE") return GL_ONE;
	else if (val == "NONE") return GL_NONE;
	REGEN_WARN("Unknown texture format mode '" << val_ << "'. Using default GL_RGBA.");
	return GL_RGBA;
}

GLenum glenum::textureInternalFormat(const std::string &val_) {
	std::string val = getValue(val_);

	if (val == "RED") return GL_RED;
	else if (val == "RG") return GL_RG;
	else if (val == "RGB") return GL_RGB;
	else if (val == "RGBA") return GL_RGBA;
	else if (val == "NONE") return GL_NONE;

	else if (val == "R16F") return GL_R16F;
	else if (val == "RG16F") return GL_RG16F;
	else if (val == "RGB16F") return GL_RGB16F;
	else if (val == "RGBA16F") return GL_RGBA16F;
	else if (val == "R32F") return GL_R32F;
	else if (val == "RG32F") return GL_RG32F;
	else if (val == "RGB32F") return GL_RGB32F;
	else if (val == "RGBA32F") return GL_RGBA32F;
	else if (val == "R11F_G11F_B10F") return GL_R11F_G11F_B10F;

	else if (val == "R8UI") return GL_R8UI;
	else if (val == "RG8UI") return GL_RG8UI;
	else if (val == "RGB8UI") return GL_RGB8UI;
	else if (val == "RGBA8UI") return GL_RGBA8UI;
	else if (val == "R16UI") return GL_R16UI;
	else if (val == "RG16UI") return GL_RG16UI;
	else if (val == "RGB16UI") return GL_RGB16UI;
	else if (val == "RGBA16UI") return GL_RGBA16UI;
	else if (val == "R32UI") return GL_R32UI;
	else if (val == "RG32UI") return GL_RG32UI;
	else if (val == "RGB32UI") return GL_RGB32UI;
	else if (val == "RGBA32UI") return GL_RGBA32UI;

	else if (val == "R8I") return GL_R8I;
	else if (val == "RG8I") return GL_RG8I;
	else if (val == "RGB8I") return GL_RGB8I;
	else if (val == "RGBA8I") return GL_RGBA8I;
	else if (val == "R16I") return GL_R16I;
	else if (val == "RG16I") return GL_RG16I;
	else if (val == "RGB16I") return GL_RGB16I;
	else if (val == "RGBA16I") return GL_RGBA16I;
	else if (val == "R32I") return GL_R32I;
	else if (val == "RG32I") return GL_RG32I;
	else if (val == "RGB32I") return GL_RGB32I;
	else if (val == "RGBA32I") return GL_RGBA32I;

	else if (val == "R8") return GL_R8;
	else if (val == "RG8") return GL_RG8;
	else if (val == "RGB8") return GL_RGB8;
	else if (val == "RGBA8") return GL_RGBA8;
	else if (val == "R16") return GL_R16;
	else if (val == "RG16") return GL_RG16;
	else if (val == "RGB16") return GL_RGB16;
	else if (val == "RGBA16") return GL_RGBA16;

	else if (val == "LUMINANCE") return GL_LUMINANCE;
	else if (val == "LUMINANCE_ALPHA") return GL_LUMINANCE_ALPHA;
	else if (val == "DEPTH_COMPONENT") return GL_DEPTH_COMPONENT;
	else if (val == "DEPTH_STENCIL") return GL_DEPTH_STENCIL;

	REGEN_WARN("Unknown internal texture format mode '" << val_ << "'. Using default GL_RGBA.");
	return GL_RGBA;
}

GLenum glenum::textureFormat(GLuint numComponents) {
	switch (numComponents) {
		case 1:
			return GL_RED;
		case 2:
			return GL_RG;
		case 3:
			return GL_RGB;
		case 4:
			return GL_RGBA;
	}
	return GL_RGBA;
}

GLenum glenum::textureInternalFormat(GLenum pixelType, GLuint numComponents, GLuint bytesPerComponent) {
	GLuint i = 1;
	if (bytesPerComponent <= 8) i = 0;
	else if (bytesPerComponent <= 16) i = 1;
	else if (bytesPerComponent <= 32) i = 2;

	GLuint j = numComponents - 1;

	if (pixelType == GL_FLOAT || pixelType == GL_DOUBLE || pixelType == GL_HALF_FLOAT) {
		static GLenum values[3][4] = {
				{GL_NONE, GL_NONE,  GL_NONE,   GL_NONE},
				{GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F},
				{GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F}
		};
		return values[i][j];
	} else if (pixelType == GL_UNSIGNED_INT) {
		static GLenum values[3][4] = {
				{GL_R8UI,  GL_RG8UI,  GL_RGB8UI,  GL_RGBA8UI},
				{GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RGBA16UI},
				{GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI}
		};
		return values[i][j];
	} else if (pixelType == GL_INT) {
		static GLenum values[3][4] = {
				{GL_R8I,  GL_RG8I,  GL_RGB8I,  GL_RGBA8I},
				{GL_R16I, GL_RG16I, GL_RGB16I, GL_RGBA16I},
				{GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I}
		};
		return values[i][j];
	} else {
		static GLenum values[3][4] = {
				{GL_R8,   GL_RG8,  GL_RGB8,  GL_RGBA8},
				{GL_R16,  GL_RG16, GL_RGB16, GL_RGBA16},
				{GL_NONE, GL_NONE, GL_NONE,  GL_NONE}
		};
		return values[i][j];
	}
	return GL_RGBA;
}
