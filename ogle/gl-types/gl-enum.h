/*
 * gl-enum.h
 *
 *  Created on: 24.02.2013
 *      Author: daniel
 */

#ifndef GL_ENUM_H_
#define GL_ENUM_H_

#include <GL/glew.h>
#include <GL/gl.h>

using namespace std;
#include <string>

///////////
//// Shader Enumeration
///////////

/**
 * Array of known shader stages Enumerations.
 */
const GLenum* glslStageEnums();
/**
 * Number of known shader stages.
 */
GLint glslStageCount();

/**
 * Maps stage enum to name representation.
 */
string glslStageName(GLenum stage);

/**
 * Maps stage enum to prefix for input variables in GLSL code.
 */
string glslStagePrefix(GLenum stage);

/**
 * Maps pixel type and values per element to the GLSL data type.
 */
string glslDataType(GLenum pixelType, GLuint valsPerElement);

///////////
//// Texture Enumeration
///////////

/**
 * Maps number of components to texel format.
 */
GLenum texFormat(GLuint numComponents);

/**
 * Maps number of components and bytes per component to internal texel format.
 */
GLenum texInternalFormat(GLuint numComponent,
    GLuint bytesPerComponent, GLboolean useFloatFormat=GL_FALSE);

/**
 * Maps [0,5] to cube map layer enum.
 */
GLenum cubeMapLayerEnum(GLuint layer);

#endif /* GL_ENUM_H_ */
