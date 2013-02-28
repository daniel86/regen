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
  MAPPING_REFRACTION,
  MAPPING_CUSTOM
}TextureMapping;
ostream& operator<<(ostream &out, const TextureMapping &v);
istream& operator>>(istream &in, TextureMapping &v);

class TextureState : public State
{
public:
  TextureState(const ref_ptr<Texture> &tex, const string &name="");
  TextureState();
  ~TextureState();

  void set_texture(const ref_ptr<Texture> &tex);

  /**
   * Name of this texture in shader programs.
   */
  void set_name(const string &name);
  /**
   * Name of this texture in shader programs.
   */
  const string& name() const;

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
   * Specifies how this texture should be mixed with existing
   * values.
   */
  void set_blendFunction(const string &blendFunction, const string &blendName);
  /**
   * Specifies how this texture should be mixed with existing
   * values.
   */
  const string& blendFunction() const;
  /**
   * Specifies how this texture should be mixed with existing
   * values.
   */
  const string& blendName() const;

  /**
   * Specifies how a texture should be mapped on geometry.
   */
  void set_mapping(TextureMapping mapping);
  /**
   * Specifies how a texture should be mapped on geometry.
   */
  TextureMapping mapping() const;
  /**
   * Specifies how a texture should be mapped on geometry.
   */
  void set_mappingFunction(const string &blendFunction, const string &blendName);
  /**
   * Specifies how a texture should be mapped on geometry.
   */
  const string& mappingFunction() const;
  /**
   * Specifies how a texture should be mapped on geometry.
   */
  const string& mappingName() const;

  /**
   * Transfer key that will be included in shaders.
   */
  void set_transferKey(const string &transferKey, const string &transferName="");
  /**
   * Transfer key that will be included in shaders.
   */
  const string& transferKey() const;
  /**
   * Sets transfer function shader code.
   * For example to scale each texel by 2.0 you can define following
   * transfer function:
   *    'void transfer(inout vec4 texel) { texel *= 2.0; }'
   */
  void set_transferFunction(const string &transferFunction, const string &transferName);
  /**
   * Transfer function shader code.
   */
  const string& transferFunction() const;
  /**
   * Name of texel transfer function.
   */
  const string& transferName() const;

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
   * Textures must be associated to texture coordinate channels.
   */
  void set_texcoChannel(GLuint channel);
  /**
   * Textures must be associated to texture coordinate channels.
   */
  GLuint texcoChannel() const;

  void setMapTo(TextureMapTo id);
  TextureMapTo mapTo() const;

  const ref_ptr<Texture>& texture() const;

  const GLint id() const;
  GLuint stateID() const;

  const string& samplerType() const;
  void set_samplerType(const string&);

  GLuint dimension() const;
  GLint channel() const;
  GLint* channelPtr() const;

  virtual void enable(RenderState*);
  virtual void disable(RenderState*);

protected:
  static GLuint idCounter_;

  GLuint stateID_;

  ref_ptr<Texture> texture_;
  string name_;
  GLint *channelPtr_;

  BlendMode blendMode_;
  GLfloat blendFactor_;
  string blendFunction_;
  string blendName_;

  TextureMapping mapping_;
  string mappingFunction_;
  string mappingName_;

  string transferKey_;
  string transferFunction_;
  string transferName_;

  GLuint texcoChannel_;

  GLboolean useAlpha_;
  GLboolean ignoreAlpha_;

  TextureMapTo mapTo_;
};

class TextureSetBufferIndex : public State
{
public:
  ref_ptr<Texture> tex_;
  GLuint bufferIndex_;

  TextureSetBufferIndex(const ref_ptr<Texture> &tex, GLuint bufferIndex)
  : tex_(tex), bufferIndex_(bufferIndex) { }

  virtual void enable(RenderState *rs) {
    tex_->set_bufferIndex(bufferIndex_);
  }
};

#endif /* TEXTURE_NODE_H_ */
