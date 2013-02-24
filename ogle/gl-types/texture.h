/*
 * texture.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <string>
#include <set>
#include <map>
#include <list>

#include <ogle/gl-types/buffer-object.h>

/**
 * A texture is an OpenGL Object that contains one or more images
 * that all have the same image format. A texture can be used in two ways.
 * It can be the source of a texture access from a Shader,
 * or it can be used as a render target.
 */
class Texture : public RectBufferObject
{
public:
  Texture(GLuint numTextures=1);
  virtual ~Texture() {}

  /**
   * Specifies the format of the pixel data.
   * Accepted values are GL_COLOR_INDEX, GL_RED, GL_GREEN,
   * GL_BLUE, GL_ALPHA, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA,
   * GL_LUMINANCE, and GL_LUMINANCE_ALPHA
   */
  void set_format(GLenum format);
  /**
   * Specifies the format of the pixel data.
   * Accepted values are GL_COLOR_INDEX, GL_RED, GL_GREEN,
   * GL_BLUE, GL_ALPHA, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA,
   * GL_LUMINANCE, and GL_LUMINANCE_ALPHA
   */
  GLenum format() const;

  /**
   * Specifies the number of color components in the texture.
   * Accepted values are GL_ALPHA*, GL_INTENSITY*, GL_R*,
   * GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
   * GL_LUMINANCE*, GL_SRGB*, GL_SLUMINANCE*, GL_COMPRESSED_*.
   */
  void set_internalFormat(GLenum internalFormat);
  /**
   * Specifies the number of color components in the texture.
   * Accepted values are GL_ALPHA*, GL_INTENSITY*, GL_R*,
   * GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
   * GL_LUMINANCE*, GL_SRGB*, GL_SLUMINANCE*, GL_COMPRESSED_*.
   */
  GLenum internalFormat() const;

  /**
   * Specifies the target texture. Accepted values are GL_TEXTURE_2D,
   * GL_PROXY_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_POSITIVE*,
   * GL_PROXY_TEXTURE_CUBE_MAP.
   */
  GLenum targetType() const;
  /**
   * Specifies the target texture. Accepted values are GL_TEXTURE_2D,
   * GL_PROXY_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_POSITIVE*,
   * GL_PROXY_TEXTURE_CUBE_MAP.
   */
  void set_targetType(GLenum targetType);

  /**
   * Specifies the data type of the pixel data.
   */
  void set_pixelType(GLuint pixelType);
  /**
   * Specifies the data type of the pixel data.
   */
  GLuint pixelType() const;

  /**
   * Number of samples used for multisampling
   */
  GLsizei numSamples() const;
  /**
   * Number of samples used for multisampling
   */
  void set_numSamples(GLsizei v);

  /**
   * Number of components per texel.
   */
  GLuint numComponents() const;

  /**
   * Specifies a pointer to the image data in memory.
   * Initially NULL.
   */
  void set_data(GLvoid *data);
  /**
   * Specifies a pointer to the image data in memory.
   * Initially NULL.
   */
  GLvoid* data() const;

  /**
   * 1/width
   */
  GLfloat texelSizeX() const;
  /**
   * 1/height
   */
  GLfloat texelSizeY() const;

  /**
   * Sets magnification and minifying parameters.
   *
   * The texture magnification function is used when the pixel being textured
   * maps to an area less than or equal to one texture element.
   * It sets the texture magnification function to
   * either GL_NEAREST or GL_LINEAR.
   *
   * The texture minifying function is used whenever the pixel
   * being textured maps to an area greater than one texture element.
   * There are six defined minifying functions.
   * Two of them use the nearest one or nearest four texture elements
   * to compute the texture value. The other four use mipmaps.
   * Accepted values are GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST,
   * GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR,
   * GL_LINEAR_MIPMAP_LINEAR.
   */
  void set_filter(GLenum mag=GL_LINEAR, GLenum min=GL_LINEAR) const;

  /**
   * Sets the minimum level-of-detail parameter.  This value limits the
   * selection of highest resolution mipmap (lowest mipmap level). The initial value is -1000.
   */
  void set_minLoD(GLfloat min) const;
  /**
   * Sets the maximum level-of-detail parameter.  This value limits the
   * selection of the lowest resolution mipmap (highest mipmap level). The initial value is 1000.
   */
  void set_maxLoD(GLfloat max) const;

  /**
   * Sets the index of the highest defined mipmap level. The initial value is 1000.
   */
  void set_maxLevel(GLint maxLevel) const;

  /**
   * Sets the wrap parameter for texture coordinates s,t to either GL_CLAMP,
   * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
   * GL_REPEAT.
   */
  void set_wrapping(GLenum wrapMode=GL_CLAMP) const;
  /**
   * Sets the wrap parameter for texture coordinates s to either GL_CLAMP,
   * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
   * GL_REPEAT.
   */
  void set_wrappingU(GLenum wrapMode=GL_CLAMP) const;
  /**
   * Sets the wrap parameter for texture coordinates t to either GL_CLAMP,
   * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
   * GL_REPEAT.
   */
  void set_wrappingV(GLenum wrapMode=GL_CLAMP) const;
  /**
   * Sets the wrap parameter for texture coordinates r to either GL_CLAMP,
   * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
   * GL_REPEAT.
   */
  void set_wrappingW(GLenum wrapMode=GL_CLAMP) const;

  /**
   * Sets the swizzle that will be applied to the r component of a texel before it is returned to the shader.
   * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
   */
  void set_swizzleR(GLenum swizzleMode) const;
  /**
   * Sets the swizzle that will be applied to the g component of a texel before it is returned to the shader.
   * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
   */
  void set_swizzleG(GLenum swizzleMode) const;
  /**
   * Sets the swizzle that will be applied to the b component of a texel before it is returned to the shader.
   * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
   */
  void set_swizzleB(GLenum swizzleMode) const;
  /**
   * Sets the swizzle that will be applied to the a component of a texel before it is returned to the shader.
   * Valid values for param are GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO and GL_ONE.
   */
  void set_swizzleA(GLenum swizzleMode) const;

  /**
   * Specifies the texture comparison mode for currently bound depth textures.
   * That is, a texture whose internal format is GL_DEPTH_COMPONENT_*;
   * Permissible values are: GL_COMPARE_R_TO_TEXTURE, GL_NONE
   * And specifies the comparison operator used when
   * mode is set to GL_COMPARE_R_TO_TEXTURE.
   */
  void set_compare(GLenum mode=GL_NONE, GLenum func=GL_EQUAL) const;

  /**
   * Set texture environment parameters.
   * Six texture functions may be specified:
   * GL_ADD, GL_MODULATE, GL_DECAL, GL_BLEND,
   * GL_REPLACE, or GL_COMBINE.
   */
  void set_envMode(GLenum envMode=GL_MODULATE) const;

  /**
   * Sets GL_TEXTURE_MAX_ANISOTROPY.
   */
  void set_aniso(GLfloat v) const;

  /**
   * Generates mipmaps for the texture.
   * Make sure to set the base level before.
   * @param mode: Should be GL_NICEST, GL_DONT_CARE or GL_FASTEST
   */
  void setupMipmaps(GLenum mode=GL_DONT_CARE) const;

  /**
   * GLSL sampler type used for this texture.
   */
  const string& samplerType() const;
  /**
   * GLSL sampler type used for this texture.
   */
  void set_samplerType(const string &samplerType);

  /**
   * Bind this texture to the currently activated
   * texture unit.
   */
  inline void bind() const {
    glBindTexture(targetType_, ids_[bufferIndex_]);
  }
  /**
   * Activates given texture unit and binds this texture
   * to it.
   */
  inline void activateBind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(targetType_, ids_[bufferIndex_]);
  }

  /**
   * Set the current viewport to the size
   * of this texture.
   */
  inline void set_viewport() const {
    glViewport(0, 0, width_, height_);
  }

  /**
   * Specify the texture image.
   */
  virtual void texImage() const = 0;

protected:
  GLuint dim_;
  GLenum targetType_;
  // format of pixel data
  GLenum format_;
  GLenum internalFormat_;
  // type for pixels
  GLenum pixelType_;
  GLint border_;

  // pixel data, or null for empty texture
  GLvoid *data_;
  // true if texture encodes data in tangent space.
  GLboolean isInTSpace_;

  GLuint numSamples_;

  string samplerType_;
};

/**
 * Images in this texture all are 1-dimensional.
 * They have width, but no height or depth.
 */
class Texture1D : public Texture {
public:
  Texture1D(GLuint numTextures=1);
  // override
  virtual void texImage() const;
  virtual void texSubImage() const;
};

/**
 * Images in this texture all are 2-dimensional.
 * They have width and height, but no depth.
 */
class Texture2D : public Texture {
public:
  Texture2D(GLuint numTextures=1);
  // override
  virtual void texImage() const;
  virtual void texSubImage() const;
};

/**
 * The image in this texture (only one image. No mipmapping)
 * is 2-dimensional. Texture coordinates used for these
 * textures are not normalized.
 */
class TextureRectangle : public Texture2D {
public:
  TextureRectangle(GLuint numTextures=1);
};

/**
 * Texture with depth format.
 */
class DepthTexture2D : public Texture2D {
public:
  DepthTexture2D(GLuint numTextures=1);
};

/**
 * The image in this texture (only one image. No mipmapping) is 2-dimensional.
 * Each pixel in these images contains multiple samples instead
 * of just one value.
 */
class Texture2DMultisample : public Texture2D {
public:
  Texture2DMultisample(
      GLsizei numSamples,
      GLuint numTextures=1,
      GLboolean fixedSampleLaocations=GL_FALSE);
  // override
  virtual void texImage() const;
private:
  GLboolean fixedsamplelocations_;
};

/**
 * The image in this texture (only one image. No mipmapping) is 2-dimensional.
 * Each pixel in these images contains multiple samples instead
 * of just one value.
 * Uses a depth format.
 */
class DepthTexture2DMultisample : public DepthTexture2D {
public:
  DepthTexture2DMultisample(
      GLsizei numSamples,
      GLboolean fixedSampleLaocations=false);
  // override
  virtual void texImage() const;
private:
  GLboolean fixedsamplelocations_;
};

/**
 * Texture with exactly 6 distinct sets of 2D images,
 * all of the same size. They act as 6 faces of a cube.
 */
class TextureCube : public Texture2D {
public:
  enum CubeSide { FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM };

  TextureCube(GLuint numTextures=1);

  void set_data(CubeSide side, void *data);
  void cubeTexImage(CubeSide side) const;

  void** cubeData();

  // override
  virtual void texImage() const;
protected:
  void* cubeData_[6];
};

class CubeMapDepthTexture : public TextureCube {
public:
  CubeMapDepthTexture(GLuint numTextures=1);
};

/**
 * A 3 dimensional texture.
 */
class Texture3D : public Texture {
public:
  Texture3D(GLuint numTextures=1);

  void set_depth(GLuint depth);
  GLuint depth();

  virtual void texImage() const;
  virtual void texSubImage(GLint layer, GLubyte *subData) const;
protected:
  GLuint numTextures_;
};

class DepthTexture3D : public Texture3D {
public:
  DepthTexture3D(GLuint numTextures=1);
};

/**
 * Array texture of two dimensional textures.
 */
class Texture2DArray : public Texture3D {
public:
  Texture2DArray(GLuint numTextures=1);
};

#endif /* _TEXTURE_H_ */
