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

// blend mode describes how a texture
// will be mixed with existing pixels
typedef enum {
  BLEND_MODE_SRC, // overwrite the existing pixel values
  BLEND_MODE_ALPHA, // mix based on alpha
  BLEND_MODE_FRONT_TO_BACK,
  BLEND_MODE_MIX,
  BLEND_MODE_MULTIPLY,
  BLEND_MODE_ADD,
  BLEND_MODE_ADD_NORMALIZED,
  BLEND_MODE_SUBSTRACT,
  BLEND_MODE_DIVIDE,
  BLEND_MODE_DIFFERENCE,
  BLEND_MODE_LIGHTEN,
  BLEND_MODE_DARKEN,
  BLEND_MODE_SCREEN,
  BLEND_MODE_OVERLAY,
  BLEND_MODE_HUE,
  BLEND_MODE_SATURATION,
  BLEND_MODE_VALUE,
  BLEND_MODE_COLOR,
  BLEND_MODE_DODGE,
  BLEND_MODE_BURN,
  BLEND_MODE_SOFT,
  BLEND_MODE_LINEAR,
  BLEND_MODE_SMOOTH_ADD,
  BLEND_MODE_SIGNED_ADD,
  BLEND_MODE_LAST
}TextureBlendMode;

// how a texture should be mapped on geometry
typedef enum {
  MAPPING_UV,
  MAPPING_FLAT,
  MAPPING_CUBE,
  MAPPING_TUBE,
  MAPPING_SPHERE,
  MAPPING_REFLECTION,
  MAPPING_REFRACTION,
  MAPPING_REFLECTION_REFRACTION,
  MAPPING_LAST
}TextureMapping;

// should texture affect color/normals/....
typedef enum {
  MAP_TO_COLOR,  // colormap (material color)
  MAP_TO_DIFFUSE,  // diffusemap (color)
  MAP_TO_AMBIENT,  // ambientmap (color)
  MAP_TO_SPECULAR, // specularmap (color)
  MAP_TO_SHININESS, // shininessmap (color)
  MAP_TO_EMISSION, // emissionmap (color)
  MAP_TO_LIGHT, // lightmap (color)
  MAP_TO_SHADOW, // shadowmaps
  MAP_TO_DIFFUSE_REFLECTION, // diffusemap (reflection)
  MAP_TO_SPECULAR_REFLECTION, // specularmap (reflection)
  MAP_TO_REFLECTION,  // reflectionmap (maps to material reflection)
  MAP_TO_ALPHA, // alphamap
  MAP_TO_NORMAL,  // normalmap
  MAP_TO_HEIGHT, // heightmap
  MAP_TO_DISPLACEMENT, // displacementmap
  MAP_TO_VOLUME, // volume data, use volume renderer
  MAP_TO_VOLUME_SLICE, // volume data, use shell texturing
  MAP_TO_LAST
}TextureMapTo;

/**
 * A texture is an OpenGL Object that contains one or more images
 * that all have the same image format. A texture can be used in two ways.
 * It can be the source of a texture access from a Shader,
 * or it can be used as a render target.
 */
class Texture : public RectBufferObject
{
public:
  Texture(
      GLuint numTextures=1,
      GLenum target=GL_TEXTURE_2D,
      GLenum format=GL_RGBA,
      GLenum internalFormat=GL_RGBA8,
      GLenum pixelType=GL_UNSIGNED_INT,
      GLint border=0,
      GLuint width=0,
      GLuint height=0);
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
   * Specifies if mipmaps should be generated for this texture.
   */
  const GLboolean useMipmaps() const;

  /**
   * Number of samples used for multisampling
   */
  GLsizei numSamples() const;
  /**
   * Number of samples used for multisampling
   */
  void set_numSamples(GLsizei v);

  /**
   * Name of this texture in shader programs.
   */
  void set_name(const string &name);
  /**
   * Name of this texture in shader programs.
   */
  const string& name() const;

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
   * Textures must be associated to texture coordinate channels.
   */
  void set_texcoChannel(GLuint channel);
  /**
   * Textures must be associated to texture coordinate channels.
   */
  GLuint texcoChannel() const;

  /**
   * Explicit request to the application to ignore the alpha channel
   * of the texture.
   */
  void set_ignoreAlpha(GLboolean v);
  /**
   * Explicit request to the application to ignore the alpha channel
   * of the texture.
   */
  GLboolean ignoreAlpha() const;

  /**
   * Explicit request to the application to process the alpha channel
   * of the texture.
   */
  void set_useAlpha(GLboolean v);
  /**
   * Explicit request to the application to process the alpha channel
   * of the texture.
   */
  GLboolean useAlpha() const;

  /**
   * Specifies if texel values should be inverted
   * when the texture is sampled.
   */
  void set_invert(GLboolean invert);
  /**
   * Specifies if texel values should be inverted
   * when the texture is sampled.
   */
  GLboolean invert() const;

  /**
   * Specifies texel brightness factor when the texture is sampled.
   */
  void set_brightness(GLfloat brightness);
  /**
   * Specifies texel brightness factor when the texture is sampled.
   */
  float brightness() const;

  /**
   * Specifies texel contrast factor when the texture is sampled.
   */
  void set_contrast(GLfloat contrast);
  /**
   * Specifies texel contrast factor when the texture is sampled.
   */
  float contrast() const;

  /**
   * Specifies how this texture should be mised with existing
   * pixels.
   */
  void set_blendMode(TextureBlendMode blendMode);
  /**
   * Specifies how this texture should be mised with existing
   * pixels.
   */
  TextureBlendMode blendMode() const;

  /**
   * Specifies how this texture should be mised with existing
   * pixels.
   */
  void set_blendFactor(GLfloat factor);
  /**
   * Specifies how this texture should be mised with existing
   * pixels.
   */
  GLfloat blendFactor() const;

  /**
   * Defines the height scaling of a bump map
   * (for stuff like Parallax Occlusion Mapping)
   */
  void set_heightScale(GLfloat heightScale);
  /**
   * Defines the height scaling of a bump map
   * (for stuff like Parallax Occlusion Mapping)
   */
  GLfloat heightScale() const;

  void set_mapping(TextureMapping mapping);
  TextureMapping mapping() const;

  void addMapTo(TextureMapTo id);
  const set<TextureMapTo>& mapTo() const;

  /**
   * 1/width
   */
  GLfloat texelSizeX();
  /**
   * 1/height
   */
  GLfloat texelSizeY();

  /**
   * Bind this texture to the currently activated
   * texture unit.
   */
  inline virtual void bind() const {
    glBindTexture(targetType_, ids_[bufferIndex_]);
  }
  /**
   * Activates given texture unit and binds this texture
   * to it.
   */
  inline virtual void activateBind(GLuint unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(targetType_, ids_[bufferIndex_]);
  }

  /**
   * Set the current viewport to the size
   * of this texture.
   */
  inline void set_viewport() {
    glViewport(0, 0, width_, height_);
  }

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
  inline void set_filter(GLenum mag=GL_LINEAR, GLenum min=GL_LINEAR) {
    glTexParameteri(targetType_, GL_TEXTURE_MAG_FILTER, mag);
    glTexParameteri(targetType_, GL_TEXTURE_MIN_FILTER, min);
  }

  /**
   * Sets the wrap parameter for texture coordinates s,t to either GL_CLAMP,
   * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
   * GL_REPEAT.
   */
  inline virtual void set_wrapping(GLenum wrapMode=GL_CLAMP) {
    glTexParameterf(targetType_, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameterf(targetType_, GL_TEXTURE_WRAP_T, wrapMode);
  }
  /**
   * Sets the wrap parameter for texture coordinates s to either GL_CLAMP,
   * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
   * GL_REPEAT.
   */
  inline virtual void set_wrappingU(GLenum wrapMode=GL_CLAMP) {
    glTexParameterf(targetType_, GL_TEXTURE_WRAP_S, wrapMode);
  }
  /**
   * Sets the wrap parameter for texture coordinates t to either GL_CLAMP,
   * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
   * GL_REPEAT.
   */
  inline virtual void set_wrappingV(GLenum wrapMode=GL_CLAMP) {
    glTexParameterf(targetType_, GL_TEXTURE_WRAP_T, wrapMode);
  }
  /**
   * Sets the wrap parameter for texture coordinates r to either GL_CLAMP,
   * GL_CLAMP_TO_BORDER, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, or
   * GL_REPEAT.
   */
  inline virtual void set_wrappingW(GLenum wrapMode=GL_CLAMP) {
    glTexParameterf(targetType_, GL_TEXTURE_WRAP_R, wrapMode);
  }

  /**
   * Specifies the texture comparison mode for currently bound depth textures.
   * That is, a texture whose internal format is GL_DEPTH_COMPONENT_*;
   * Permissible values are: GL_COMPARE_R_TO_TEXTURE, GL_NONE
   * And specifies the comparison operator used when
   * mode is set to GL_COMPARE_R_TO_TEXTURE.
   */
  inline void set_compare(GLenum mode=GL_NONE, GLenum func=GL_EQUAL) {
    glTexParameteri(targetType_, GL_TEXTURE_COMPARE_MODE, mode);
    glTexParameteri(targetType_, GL_TEXTURE_COMPARE_FUNC, func);
  }

  /**
   * Specifies a single symbolic constant indicating how depth values
   * should be treated during filtering and texture application.
   * Accepted values are GL_LUMINANCE, GL_INTENSITY, and GL_ALPHA.
   * The initial value is GL_LUMINANCE.
   */
  inline void set_depthMode(GLenum depthMode=GL_INTENSITY) {
    glTexParameteri(targetType_, GL_DEPTH_TEXTURE_MODE, depthMode);
  }

  /**
   * Set texture environment parameters.
   * Six texture functions may be specified:
   * GL_ADD, GL_MODULATE, GL_DECAL, GL_BLEND,
   * GL_REPLACE, or GL_COMBINE.
   */
  inline void set_envMode(GLenum envMode=GL_MODULATE) {
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, envMode);
  }

  /**
   * Generates mipmaps for the texture.
   * Make sure to set the base level before.
   * @param mode: Should be GL_NICEST, GL_DONT_CARE or GL_FASTEST
   */
  inline void setupMipmaps(GLenum mode=GL_DONT_CARE) {
    // glGenerateMipmap was introduced in opengl3.0
    // before glBuildMipmaps or GL_GENERATE_MIPMAP was used, but we do not need them ;)
    glGenerateMipmap(targetType_);
    glHint(GL_GENERATE_MIPMAP_HINT, mode);
    useMipmaps_ = true;
  }

  /**
   * Specify the texture image.
   */
  virtual void texImage() const = 0;
  /**
   * Returns GLSL sampler type used for this texture.
   */
  virtual string samplerType() const = 0;

protected:
    string name_;
    GLuint texcoChannel_;
    GLenum targetType_;
    // format of pixel data
    GLenum format_;
    GLenum internalFormat_;
    // type for pixels
    GLenum pixelType_;
    GLint border_;

    TextureBlendMode blendMode_;
    TextureMapping mapping_;
    set<TextureMapTo> mapTo_;

    // pixel data, or null for empty texture
    GLvoid *data_;
    // true if texture encodes data in tangent space.
    GLboolean isInTSpace_;
    GLboolean useMipmaps_;
    GLboolean useAlpha_;
    GLboolean ignoreAlpha_;

    GLboolean invert_;

    GLfloat brightness_;
    GLfloat contrast_;
    GLfloat blendFactor_;
    GLfloat heightScale_;

    GLuint numSamples_;
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
  virtual string samplerType() const;
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
  virtual string samplerType() const;
private:
    Texture2D(const Texture2D&);
};

/**
 * The image in this texture (only one image. No mipmapping)
 * is 2-dimensional. Texture coordinates used for these
 * textures are not normalized.
 */
class TextureRectangle : public Texture2D {
public:
  TextureRectangle(GLuint numTextures=1);
  // override
  virtual string samplerType() const;
};

/**
 * Texture with depth format.
 */
class DepthTexture2D : public Texture2D {
public:
  DepthTexture2D(GLuint numTextures=1);
private:
  DepthTexture2D(const Texture2D&);
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
      GLboolean fixedSampleLaocations=false);
  // override
  virtual void texImage() const;
  virtual string samplerType() const;
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
class CubeMapTexture : public Texture2D {
public:
  enum CubeSide { FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM };
  static GLenum cubeSideToGLSide_[];

  CubeMapTexture(GLuint numTextures=1);

  void set_data(CubeSide side, void *data);
  void cubeTexImage(CubeSide side) const;

  // override
  virtual void texImage() const;
  virtual string samplerType() const;
protected:
  void* cubeData_[6];

private:
  CubeMapTexture(const CubeMapTexture&);
};

class NoiseTexture2D : public Texture2D {
public:
  NoiseTexture2D(GLuint width, GLuint height);
};

typedef map<TextureMapTo, list< pair<Texture*,bool> > > TextureMapToMap;

#endif /* _TEXTURE_H_ */
