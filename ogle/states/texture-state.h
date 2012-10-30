/*
 * texture-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef TEXTURE_NODE_H_
#define TEXTURE_NODE_H_

#include <ogle/states/state.h>
#include <ogle/states/blend-state.h>
#include <ogle/gl-types/texture.h>

// should texture affect color/normals/....
typedef enum {
  MAP_TO_COLOR,  // colormap (material color)
  MAP_TO_DIFFUSE,  // diffusemap (color)
  MAP_TO_AMBIENT,  // ambientmap (color)
  MAP_TO_SPECULAR, // specularmap (color)
  MAP_TO_SHININESS, // shininessmap (color)
  MAP_TO_EMISSION, // emissionmap (color)
  MAP_TO_LIGHT, // lightmap (color)
  MAP_TO_ALPHA, // alphamap
  MAP_TO_NORMAL,  // normalmap
  MAP_TO_HEIGHT, // heightmap
  MAP_TO_DISPLACEMENT, // displacementmap
  MAP_TO_CUSTOM
}TextureMapTo;
ostream& operator<<(ostream &out, const TextureMapTo &v);
istream& operator>>(istream &in, TextureMapTo &v);

// how a texture should be mapped on geometry
typedef enum {
  MAPPING_TEXCO,
  MAPPING_FLAT,
  MAPPING_CUBE,
  MAPPING_TUBE,
  MAPPING_SPHERE,
  MAPPING_REFLECTION,
  MAPPING_REFRACTION
}TextureMapping;
ostream& operator<<(ostream &out, const TextureMapping &v);
istream& operator>>(istream &in, TextureMapping &v);

// TODO: put the texco selection stuff here
class TextureState : public State
{
public:
  TextureState(ref_ptr<Texture> tex);

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
   * when the texture is sampled (1- texel).
   */
  void set_invert(GLboolean invert);
  /**
   * Specifies if texel values should be inverted
   * when the texture is sampled (1- texel).
   */
  GLboolean invert() const;

  /**
   * Specifies how this texture should be mixed with existing
   * values.
   */
  void set_blendMode(BlendMode blendMode);
  /**
   * Specifies how this texture should be mixed with existing
   * values.
   */
  BlendMode blendMode() const;

  /**
   * Specifies how this texture should be mixed with existing
   * pixels.
   */
  void set_blendFactor(GLfloat factor);
  /**
   * Specifies how this texture should be mixed with existing
   * pixels.
   */
  GLfloat blendFactor() const;

  /**
   * Specifies texel brightness factor when the texture is sampled.
   */
  void set_texelFactor(GLfloat brightness);
  /**
   * Specifies texel brightness factor when the texture is sampled.
   */
  GLfloat texelFactor() const;

  /**
   * Textures must be associated to texture coordinate channels.
   */
  void set_texcoChannel(GLuint channel);
  /**
   * Textures must be associated to texture coordinate channels.
   */
  GLuint texcoChannel() const;

  void setMapTo(TextureMapTo id);
  TextureMapTo mapTo() const;

  void set_mapping(TextureMapping mapping);
  TextureMapping mapping() const;

  void set_transferKey(const string &transferKey);
  const string& transferKey() const;

  ref_ptr<Texture>& texture();

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);
  virtual void configureShader(ShaderConfig*);

  virtual string name();

  const GLint id() const;
  const string samplerType() const;
  const string& textureName() const;

  GLuint dimension() const { return texture_->dimension(); };

protected:
  ref_ptr<Texture> texture_;
  string transferKey_;

  GLuint textureChannel_;
  GLuint texcoChannel_;

  BlendMode blendMode_;
  GLboolean useAlpha_;
  GLboolean ignoreAlpha_;
  GLboolean invert_;
  GLfloat texelFactor_;
  GLfloat blendFactor_;

  TextureMapTo mapTo_;
  TextureMapping mapping_;
};

class TextureConstantUnitNode : public TextureState
{
public:
  TextureConstantUnitNode(ref_ptr<Texture> &tex, GLuint textureUnit);
  virtual void enable(RenderState*);

  GLuint textureUnit() const;
};

#endif /* TEXTURE_NODE_H_ */
