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
    , GL_GEOMETRY_SHADER
    , GL_FRAGMENT_SHADER
#ifdef GL_COMPUTE_SHADER
    , GL_COMPUTE_SHADER
#endif
};
static const GLint glslStageCount__ = sizeof(glslStages__)/sizeof(GLenum);

static string getValue(const string &in)
{
  std::string val = in;
  boost::to_upper(val);
  if(boost::starts_with(val, "GL_")) {
    val = val.substr(3);
  }
  return val;
}

const GLenum* glenum::glslStages()
{ return glslStages__; }
GLint glenum::glslStageCount()
{ return glslStageCount__; }

string glenum::glslStageName(GLenum stage)
{
  switch(stage) {
  case GL_NONE:                   return "NONE";
  case GL_VERTEX_SHADER:          return "VERTEX_SHADER";
#ifdef GL_TESS_CONTROL_SHADER
  case GL_TESS_CONTROL_SHADER:    return "TESS_CONTROL_SHADER";
#endif
#ifdef GL_TESS_EVALUATION_SHADER
  case GL_TESS_EVALUATION_SHADER: return "TESS_EVALUATION_SHADER";
#endif
  case GL_GEOMETRY_SHADER:        return "GEOMETRY_SHADER";
  case GL_FRAGMENT_SHADER:        return "FRAGMENT_SHADER";
#ifdef GL_COMPUTE_SHADER
  case GL_COMPUTE_SHADER:         return "COMPUTE_SHADER";
#endif
  default: return "UNKNOWN_SHADER";
  }
}

string glenum::glslStagePrefix(GLenum stage)
{
  switch(stage) {
  case GL_VERTEX_SHADER:          return "vs";
#ifdef GL_TESS_CONTROL_SHADER
  case GL_TESS_CONTROL_SHADER:    return "tcs";
#endif
#ifdef GL_TESS_EVALUATION_SHADER
  case GL_TESS_EVALUATION_SHADER: return "tes";
#endif
  case GL_GEOMETRY_SHADER:        return "gs";
  case GL_FRAGMENT_SHADER:        return "fs";
#ifdef GL_COMPUTE_SHADER
  case GL_COMPUTE_SHADER:         return "cs";
#endif
  default: return "unk";
  }
}

string glenum::glslDataType(GLenum pixelType, GLuint valsPerElement)
{
  switch(pixelType) {
  case GL_BYTE:
  case GL_UNSIGNED_BYTE:
  case GL_SHORT:
  case GL_UNSIGNED_SHORT:
  case GL_INT:
    switch(valsPerElement) {
    case 1:  return "int";
    case 2:  return "ivec2";
    case 3:  return "ivec3";
    case 4:  return "ivec4";
    }
    break;
  case GL_UNSIGNED_INT:
    switch(valsPerElement) {
    case 1:  return "uint";
    case 2:  return "uvec2";
    case 3:  return "uvec3";
    case 4:  return "uvec4";
    }
    break;
  case GL_FLOAT:
  case GL_DOUBLE:
  default:
    switch(valsPerElement) {
    case 1:  return "float";
    case 2:  return "vec2";
    case 3:  return "vec3";
    case 4:  return "vec4";
    case 9:  return "mat3";
    case 16: return "mat4";
    }
    break;
  }
  return "unk";
}

GLenum glenum::cubeMapLayer(GLuint layer)
{
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

GLenum glenum::depthFunction(const string &val_)
{
  std::string val = getValue(val_);
  if(val == "NEVER")            return GL_NEVER;
  else if(val == "GREATER")     return GL_GREATER;
  else if(val == "NOTEQUAL")    return GL_NOTEQUAL;
  else if(val == "GEQUAL")      return GL_GEQUAL;
  else if(val == "ALWAYS")      return GL_ALWAYS;
  else if(val == "LEQUAL")      return GL_LEQUAL;
  else if(val == "LESS")        return GL_LESS;
  else if(val == "EQUAL")       return GL_EQUAL;
  else if(val == "NONE")        return GL_NONE;
  REGEN_WARN("Unknown depth function '" << val_ << "'. Using default LEQUAL.");
  return GL_LEQUAL;
}

GLenum glenum::cullFace(const string &val_)
{
  std::string val = getValue(val_);
  if(val == "FRONT")                return GL_FRONT;
  else if(val == "BACK")            return GL_BACK;
  else if(val == "FRONT_AND_BACK")  return GL_FRONT_AND_BACK;
  else if(val == "NONE")            return GL_NONE;
  REGEN_WARN("Unknown cull face '" << val_ << "'. Using default FRONT.");
  return GL_FRONT;
}

GLenum glenum::pixelType(const string &val_)
{
  std::string val = getValue(val_);
  if(val == "HALF_FLOAT")           return GL_HALF_FLOAT;
  else if(val == "FLOAT")           return GL_FLOAT;
  else if(val == "UNSIGNED_BYTE")   return GL_UNSIGNED_BYTE;
  else if(val == "BYTE")            return GL_BYTE;
  else if(val == "SHORT")           return GL_SHORT;
  else if(val == "UNSIGNED_SHORT")  return GL_UNSIGNED_SHORT;
  else if(val == "INT")             return GL_INT;
  else if(val == "UNSIGNED_INT")    return GL_UNSIGNED_INT;
  else if(val == "DOUBLE")          return GL_DOUBLE;
  else if(val == "NONE")            return GL_NONE;
  REGEN_WARN("Unknown pixel type '" << val_ << "'. Using default UNSIGNED_BYTE.");
  return GL_UNSIGNED_BYTE;
}

GLenum glenum::fillMode(const string &val_)
{
  std::string val = getValue(val_);
  if(val == "FILL")       return GL_FILL;
  else if(val == "LINE")  return GL_LINE;
  else if(val == "POINT") return GL_POINT;
  else if(val == "NONE")  return GL_NONE;
  REGEN_WARN("Unknown fill mode '" << val_ << "'. Using default GL_FILL.");
  return GL_FILL;
}

GLenum glenum::filterMode(const string &val_)
{
  std::string val = getValue(val_);
  if(val == "NEAREST")                      return GL_NEAREST;
  else if(val == "LINEAR")                  return GL_LINEAR;
  else if(val == "NEAREST_MIPMAP_NEAREST")  return GL_NEAREST_MIPMAP_NEAREST;
  else if(val == "LINEAR_MIPMAP_NEAREST")   return GL_LINEAR_MIPMAP_NEAREST;
  else if(val == "NEAREST_MIPMAP_LINEAR")   return GL_NEAREST_MIPMAP_LINEAR;
  else if(val == "LINEAR_MIPMAP_LINEAR")    return GL_LINEAR_MIPMAP_LINEAR;
  else if(val == "NONE")                    return GL_NONE;
  REGEN_WARN("Unknown filter mode '" << val_ << "'. Using default GL_LINEAR.");
  return GL_LINEAR;
}

GLenum glenum::wrappingMode(const string &val_)
{
  std::string val = getValue(val_);
  if(val == "CLAMP")                return GL_CLAMP;
  else if(val == "CLAMP_TO_BORDER") return GL_CLAMP_TO_BORDER;
  else if(val == "CLAMP_TO_EDGE")   return GL_CLAMP_TO_EDGE;
  else if(val == "MIRRORED_REPEAT") return GL_MIRRORED_REPEAT;
  else if(val == "REPEAT")          return GL_REPEAT;
  else if(val == "NONE")            return GL_NONE;
  REGEN_WARN("Unknown wrapping mode '" << val_ << "'. Using default GL_CLAMP.");
  return GL_CLAMP;
}

GLenum glenum::textureFormat(const string &val_)
{
  std::string val = getValue(val_);
  if(val == "RED")       return GL_RED;
  else if(val == "RG")   return GL_RG;
  else if(val == "RGB")  return GL_RGB;
  else if(val == "RGBA") return GL_RGBA;
  else if(val == "NONE") return GL_NONE;
  REGEN_WARN("Unknown texture format mode '" << val_ << "'. Using default GL_RGBA.");
  return GL_RGBA;
}

GLenum glenum::textureInternalFormat(const string &val_)
{
  std::string val = getValue(val_);

  if(val == "RED")       return GL_RED;
  else if(val == "RG")   return GL_RG;
  else if(val == "RGB")  return GL_RGB;
  else if(val == "RGBA") return GL_RGBA;
  else if(val == "NONE") return GL_NONE;

  else if(val == "R16F")    return GL_R16F;
  else if(val == "RG16F")   return GL_RG16F;
  else if(val == "RGB16F")  return GL_RGB16F;
  else if(val == "RGBA16F") return GL_RGBA16F;
  else if(val == "R32F")    return GL_R32F;
  else if(val == "RG32F")   return GL_RG32F;
  else if(val == "RGB32F")  return GL_RGB32F;
  else if(val == "RGBA32F") return GL_RGBA32F;
  else if(val == "R11F_G11F_B10F") return GL_R11F_G11F_B10F;

  else if(val == "R8UI")     return GL_R8UI;
  else if(val == "RG8UI")    return GL_RG8UI;
  else if(val == "RGB8UI")   return GL_RGB8UI;
  else if(val == "RGBA8UI")  return GL_RGBA8UI;
  else if(val == "R16UI")    return GL_R16UI;
  else if(val == "RG16UI")   return GL_RG16UI;
  else if(val == "RGB16UI")  return GL_RGB16UI;
  else if(val == "RGBA16UI") return GL_RGBA16UI;
  else if(val == "R32UI")    return GL_R32UI;
  else if(val == "RG32UI")   return GL_RG32UI;
  else if(val == "RGB32UI")  return GL_RGB32UI;
  else if(val == "RGBA32UI") return GL_RGBA32UI;

  else if(val == "R8I")      return GL_R8I;
  else if(val == "RG8I")     return GL_RG8I;
  else if(val == "RGB8I")    return GL_RGB8I;
  else if(val == "RGBA8I")   return GL_RGBA8I;
  else if(val == "R16I")     return GL_R16I;
  else if(val == "RG16I")    return GL_RG16I;
  else if(val == "RGB16I")   return GL_RGB16I;
  else if(val == "RGBA16I")  return GL_RGBA16I;
  else if(val == "R32I")     return GL_R32I;
  else if(val == "RG32I")    return GL_RG32I;
  else if(val == "RGB32I")   return GL_RGB32I;
  else if(val == "RGBA32I")  return GL_RGBA32I;

  else if(val == "R8")     return GL_R8;
  else if(val == "RG8")    return GL_RG8;
  else if(val == "RGB8")   return GL_RGB8;
  else if(val == "RGBA8")  return GL_RGBA8;
  else if(val == "R16")    return GL_R16;
  else if(val == "RG16")   return GL_RG16;
  else if(val == "RGB16")  return GL_RGB16;
  else if(val == "RGBA16") return GL_RGBA16;

  REGEN_WARN("Unknown internal texture format mode '" << val_ << "'. Using default GL_FILL.");
  return GL_RGBA;
}

GLenum glenum::textureFormat(GLuint numComponents)
{
  switch (numComponents) {
  case 1: return GL_RED;
  case 2: return GL_RG;
  case 3: return GL_RGB;
  case 4: return GL_RGBA;
  }
  return GL_RGBA;
}

GLenum glenum::textureInternalFormat(GLenum pixelType, GLuint numComponents, GLuint bytesPerComponent)
{
  GLuint i = 1;
  if(bytesPerComponent<=8)  i=0;
  else if(bytesPerComponent<=16) i=1;
  else if(bytesPerComponent<=32) i=2;

  GLuint j=numComponents-1;

  if(pixelType==GL_FLOAT || pixelType==GL_DOUBLE || pixelType==GL_HALF_FLOAT) {
    static GLenum values[3][4] = {
        {GL_NONE, GL_NONE,  GL_NONE,   GL_NONE},
        {GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F},
        {GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F}
    };
    return values[i][j];
  }
  else if(pixelType==GL_UNSIGNED_INT) {
    static GLenum values[3][4] = {
        {GL_R8UI,  GL_RG8UI,  GL_RGB8UI,  GL_RGBA8UI},
        {GL_R16UI, GL_RG16UI, GL_RGB16UI, GL_RGBA16UI},
        {GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI}
    };
    return values[i][j];
  }
  else if(pixelType==GL_INT) {
    static GLenum values[3][4] = {
        {GL_R8I,  GL_RG8I,  GL_RGB8I,  GL_RGBA8I},
        {GL_R16I, GL_RG16I, GL_RGB16I, GL_RGBA16I},
        {GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I}
    };
    return values[i][j];
  }
  else {
    static GLenum values[3][4] = {
        {GL_R8,   GL_RG8,  GL_RGB8,  GL_RGBA8},
        {GL_R16,  GL_RG16, GL_RGB16, GL_RGBA16},
        {GL_NONE, GL_NONE, GL_NONE,  GL_NONE}
    };
    return values[i][j];
  }
  return GL_RGBA;
}
