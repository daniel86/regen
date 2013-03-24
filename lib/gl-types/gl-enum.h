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

namespace ogle {
/**
 * \brief Interface to GL enumerations.
 */
class GLEnum
{
public:
  /**
   * Read texture pixel type from string.
   * @param val the type string.
   * @return the type enum or GL_NONE.
   */
  static GLenum pixelType(const string &val);
  /**
   * Convert number of components to texture format enum.
   * @param numComponents number of components per texel.
   * @return the format enum.
   */
  static GLenum textureFormat(GLuint numComponents);
  /**
   * @param pixelType the texture pixel type.
   * @param numComponents number of components per texels.
   * @param bytesPerComponent bytes per component.
   * @return
   */
  static GLenum textureInternalFormat(GLenum pixelType, GLuint numComponents, GLuint bytesPerComponent);

  /**
   * Maps [0,5] to cube map layer enum.
   */
  static GLenum cubeMapLayer(GLuint layer);

  /**
   * Array of known shader stages Enumerations.
   */
  static const GLenum* glslStages();
  /**
   * Number of known shader stages.
   */
  static GLint glslStageCount();
  /**
   * Maps stage enum to name representation.
   */
  static string glslStageName(GLenum stage);
  /**
   * Maps stage enum to prefix for input variables in GLSL code.
   */
  static string glslStagePrefix(GLenum stage);
  /**
   * Maps pixel type and values per element to the GLSL data type.
   */
  static string glslDataType(GLenum pixelType, GLuint valsPerElement);
};
} // namespace

#endif /* GL_ENUM_H_ */
