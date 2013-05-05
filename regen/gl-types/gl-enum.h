/*
 * gl-enum.h
 *
 *  Created on: 24.02.2013
 *      Author: daniel
 */

#ifndef GL_ENUM_H_
#define GL_ENUM_H_

#include <GL/glew.h>

using namespace std;
#include <string>

namespace regen {
  namespace glenum {
    /**
     * Read texture pixel type from string.
     * @param val the type string.
     * @return the type enum or GL_NONE.
     */
    GLenum pixelType(const string &val);
    /**
     * Convert number of components to texture format enum.
     * @param numComponents number of components per texel.
     * @return the format enum.
     */
    GLenum textureFormat(GLuint numComponents);
    /**
     * @param pixelType the texture pixel type.
     * @param numComponents number of components per texels.
     * @param bytesPerComponent bytes per component.
     * @return
     */
    GLenum textureInternalFormat(GLenum pixelType, GLuint numComponents, GLuint bytesPerComponent);
    /**
     * Maps [0,5] to cube map layer enum.
     */
    GLenum cubeMapLayer(GLuint layer);

    /**
     * Array of known shader stages Enumerations.
     */
    const GLenum* glslStages();
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
  }
} // namespace

#endif /* GL_ENUM_H_ */
