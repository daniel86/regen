/*
 * gl-enum.cpp
 *
 *  Created on: 24.02.2013
 *      Author: daniel
 */

#include "gl-enum.h"
namespace ogle {

const GLenum* glslStageEnums()
{
  static const GLenum glslStages__[] = {
      GL_VERTEX_SHADER,
      GL_TESS_CONTROL_SHADER,
      GL_TESS_EVALUATION_SHADER,
      GL_GEOMETRY_SHADER,
      GL_FRAGMENT_SHADER,
      GL_COMPUTE_SHADER
  };
  return glslStages__;
}
GLint glslStageCount()
{
  return 6;
}

string glslStageName(GLenum stage)
{
  switch(stage) {
  case GL_VERTEX_SHADER:          return "VERTEX_SHADER";
  case GL_TESS_CONTROL_SHADER:    return "TESS_CONTROL_SHADER";
  case GL_TESS_EVALUATION_SHADER: return "TESS_EVALUATION_SHADER";
  case GL_GEOMETRY_SHADER:        return "GEOMETRY_SHADER";
  case GL_FRAGMENT_SHADER:        return "FRAGMENT_SHADER";
  case GL_COMPUTE_SHADER:         return "COMPUTE_SHADER";
  default: return "UNKNOWN_SHADER";
  }
}

string glslStagePrefix(GLenum stage)
{
  switch(stage) {
  case GL_VERTEX_SHADER:          return "vs";
  case GL_TESS_CONTROL_SHADER:    return "tcs";
  case GL_TESS_EVALUATION_SHADER: return "tes";
  case GL_GEOMETRY_SHADER:        return "gs";
  case GL_FRAGMENT_SHADER:        return "fs";
  case GL_COMPUTE_SHADER:         return "cs";
  default: return "unk";
  }
}

string glslDataType(GLenum pixelType, GLuint valsPerElement)
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
  case GL_UNSIGNED_INT:
    switch(valsPerElement) {
    case 1:  return "uint";
    case 2:  return "uvec2";
    case 3:  return "uvec3";
    case 4:  return "uvec4";
    }
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
  }
  return "unk";
}

///////////
///////////

GLenum texFormat(GLuint numComponents)
{
  switch(numComponents) {
  case 1: return GL_LUMINANCE;
  case 2: return GL_RG;
  case 4: return GL_RGBA;
  default: return GL_RGB;
  }
}

GLenum texInternalFormat(GLuint numComponents,
    GLuint bytesPerComponent, GLboolean useFloatFormat)
{
  switch(numComponents) {
  case 1:
    if(bytesPerComponent==8) {
      return (useFloatFormat ? GL_R8 : GL_R8);
    } else if(bytesPerComponent==16) {
      return (useFloatFormat ? GL_R16F : GL_R16);
    } else if(bytesPerComponent==32) {
      return (useFloatFormat ? GL_R32F : GL_R32UI);
    }
  case 2:
    if(bytesPerComponent==8) {
      return (useFloatFormat ? GL_RG8 : GL_RG8);
    } else if(bytesPerComponent==16) {
      return (useFloatFormat ? GL_RG16F : GL_RG16);
    } else if(bytesPerComponent==32) {
      return (useFloatFormat ? GL_RG32F : GL_RG32UI);
    }
  case 3:
    if(bytesPerComponent==8) {
      return (useFloatFormat ? GL_RGB8 : GL_RGB8);
    } else if(bytesPerComponent==16) {
      return (useFloatFormat ? GL_RGB16F : GL_RGB16);
    } else if(bytesPerComponent==32) {
      return (useFloatFormat ? GL_RGB32F : GL_RGB32UI);
    }
  case 4:
    if(bytesPerComponent==8) {
      return (useFloatFormat ? GL_RGBA8 : GL_RGBA8);
    } else if(bytesPerComponent==16) {
      return (useFloatFormat ? GL_RGBA16F : GL_RGBA16);
    } else if(bytesPerComponent==32) {
      return (useFloatFormat ? GL_RGBA32F : GL_RGBA32UI);
    }
  }
  return GL_RGB;
}

GLenum cubeMapLayerEnum(GLuint layer) {
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

}
