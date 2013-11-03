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
     * Specifies the depth comparison function.
     * @param val input string.
     * @return GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL,
     *         GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, or GL_ALWAYS.
     */
    GLenum compareFunction(const string &val);
    /**
     * Specifies the texture comparison mode for currently bound depth textures.
     * That is, a texture whose internal format is GL_DEPTH_COMPONENT_*
     */
    GLenum compareMode(const string &val_);
    /**
     * Specifies how source and destination colors are combined.
     * It must be GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX.
     * Initially, both the RGB blend equation and the alpha blend equation are set to GL_FUNC_ADD.
     */
    GLenum blendFunction(const string &val_);

    /**
     * The cull face specifies whether front- or back-facing facets are candidates for culling.
     * @param val input string.
     * @return GL_FRONT, GL_BACK, or GL_FRONT_AND_BACK.
     */
    GLenum cullFace(const string &val);

    /**
     * Fill mode selects a polygon rasterization mode.
     * @param val input string.
     * @return GL_FILL, L_LINE or GL_POINT.
     */
    GLenum fillMode(const string &val);

    /**
     * The filter mode is used whenever the level-of-detail function
     * used when sampling from the texture determines that the texture should
     * be minified or magnified.
     * @param val input string.
     * @return GL_NEAREST, GL_LINEAR,
     *         GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR,
     *         GL_LINEAR_MIPMAP_NEAREST or GL_LINEAR_MIPMAP_LINEAR.
     */
    GLenum filterMode(const string &val);

    /**
     * The wrap parameter for texture coordinates.
     * @param val input string.
     * @return GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE,
     *         GL_MIRRORED_REPEAT or GL_REPEAT.
     */
    GLenum wrappingMode(const string &val);

    /**
     * Defines base type of texel data.
     * @param val the type string.
     * @return GL_HALF_FLOAT, GL_FLOAT, GL_UNSIGNED_BYTE
     *         GL_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT,
     *         GL_UNSIGNED_INT or GL_DOUBLE.
     */
    GLenum pixelType(const string &val);

    /**
     * Defines the format of texel data that is visible to the outside.
     * @param numComponents Number of components per texel.
     * @return On of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
     */
    GLenum textureFormat(GLuint numComponents);
    /**
     * Defines the format of texel data that is visible to the outside.
     * @param val the type string.
     * @return On of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
     */
    GLenum textureFormat(const string &val);
    /**
     * Sets the swizzle that will be applied to the rgba components of a texel before it is returned to the shader.
     * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
     */
    GLenum textureSwizzle(const string &val_);

    /**
     * Defines the format of texel data that is used texture intern.
     * @param pixelType the texture pixel type.
     * @param numComponents number of components per texels.
     * @param bytesPerComponent bytes per component.
     * @return On of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
     */
    GLenum textureInternalFormat(GLenum pixelType, GLuint numComponents, GLuint bytesPerComponent);
    /**
     * Defines the format of texel data that is used texture intern.
     * @param val the type string.
     * @return On of the GL_R,GL_RG,GL_RGB,GL_RGBA constants.
     */
    GLenum textureInternalFormat(const string &val);

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
