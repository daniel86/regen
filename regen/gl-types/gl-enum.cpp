/*
 * gl-enum.cpp
 *
 *  Created on: 24.02.2013
 *      Author: daniel
 */

#include "gl-enum.h"
using namespace regen;

const GLenum* GLEnum::glslStages()
{
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
  return glslStages__;
}
GLint GLEnum::glslStageCount()
{
  return 6;
}

string GLEnum::glslStageName(GLenum stage)
{
  switch(stage) {
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

string GLEnum::glslStagePrefix(GLenum stage)
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

string GLEnum::glslDataType(GLenum pixelType, GLuint valsPerElement)
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

GLenum GLEnum::cubeMapLayer(GLuint layer)
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

GLenum GLEnum::pixelType(const string &val)
{
  if(val == "GL_HALF_FLOAT")           return GL_HALF_FLOAT;
  else if(val == "GL_FLOAT")           return GL_FLOAT;
  else if(val == "GL_UNSIGNED_BYTE")   return GL_UNSIGNED_BYTE;
  else if(val == "GL_BYTE")            return GL_BYTE;
  else if(val == "GL_SHORT")           return GL_SHORT;
  else if(val == "GL_UNSIGNED_SHORT")  return GL_UNSIGNED_SHORT;
  else if(val == "GL_INT")             return GL_INT;
  else if(val == "GL_UNSIGNED_INT")    return GL_UNSIGNED_INT;
  else if(val == "GL_DOUBLE")          return GL_DOUBLE;
  return GL_NONE;
}

GLenum GLEnum::textureFormat(GLuint numComponents)
{
  switch (numComponents) {
  case 1: return GL_LUMINANCE;
  case 2: return GL_RG;
  case 3: return GL_RGB;
  case 4: return GL_RGBA;
  }
  return GL_RGBA;
}

GLenum GLEnum::textureInternalFormat(GLenum pixelType, GLuint numComponents, GLuint bytesPerComponent)
{
  GLuint i=bytesPerComponent/8 - 1;
  if(i>3) { return GL_NONE; } // max 32 bit
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
