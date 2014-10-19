/*
 * texture-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef TEXTURE_NODE_H_
#define TEXTURE_NODE_H_

#include <regen/states/state.h>
#include <regen/states/blend-state.h>
#include <regen/gl-types/texture.h>

namespace regen {
  /**
   * \brief A State Object that contains one or more images
   * that all have the same image format.
   */
  class TextureState : public State
  {
  public:
    /**
     * \brief Defines what is affected by the texture.
     */
    enum MapTo {
      /** The texture is combined with the fragment color. */
      MAP_TO_COLOR,
      /** The texture is combined with the result of the diffuse lighting equation. */
      MAP_TO_DIFFUSE,
      /** The texture is combined with the result of the ambient lighting equation. */
      MAP_TO_AMBIENT,
      /** The texture is combined with the result of the specular lighting equation. */
      MAP_TO_SPECULAR,
      /** The texture defines the glossiness of the material. */
      MAP_TO_SHININESS,
      /** The texture is added to the result of the lighting calculation. */
      MAP_TO_EMISSION,
      /** Lightmap texture (aka Ambient Occlusion). */
      MAP_TO_LIGHT,
      /** The texture defines per-pixel opacity. */
      MAP_TO_ALPHA,
      /** The texture is a normal map.. */
      MAP_TO_NORMAL,
      /** The texture is a height map.. */
      MAP_TO_HEIGHT,
      /** Displacement texture. The exact purpose and format is application-dependent.. */
      MAP_TO_DISPLACEMENT,
      /** user defined. */
      MAP_TO_CUSTOM
    };
    /**
     * \brief Defines how a texture should be mapped on geometry.
     */
    enum Mapping {
      /** Texture coordinate mapping. */
      MAPPING_TEXCO,
      /** Flat mapping. */
      MAPPING_FLAT,
      /** Cube mapping. */
      MAPPING_CUBE,
      /** Tube mapping. */
      MAPPING_TUBE,
      /** Sphere mapping. */
      MAPPING_SPHERE,
      /** Reflection mapping (for Cube Maps). */
      MAPPING_CUBE_REFLECTION,
      /** Reflection mapping (for Planar reflection).
       *  Requires an uniform named 'reflectionMatrix' that
       *  was used to render the reflected scene (proj*view*reflection). */
      MAPPING_PLANAR_REFLECTION,
      /** Reflection mapping (for Paraboloid Maps). */
      MAPPING_PARABOLOID_REFLECTION,
      /** Refraction mapping. */
      MAPPING_CUBE_REFRACTION,
      /** User defined mapping. */
      MAPPING_CUSTOM
    };
    /**
     * \brief some default transfer functions for texco values
     */
    enum TransferTexco {
      /** parallax mapping. */
      TRANSFER_TEXCO_PARALLAX,
      /** parallax occlusion mapping. */
      TRANSFER_TEXCO_PARALLAX_OCC,
      /** relief mapping. */
      TRANSFER_TEXCO_RELIEF,
      /** fisheye mapping. */
      TRANSFER_TEXCO_FISHEYE
    };

    TextureState();
    /**
     * @param tex the associates texture.
     * @param name the name of the texture in shader programs.
     */
    TextureState(const ref_ptr<Texture> &tex, const std::string &name="");

    /**
     * @return used to get unique names in shaders.
     */
    GLuint stateID() const;

    /**
     * @param tex the associates texture.
     */
    void set_texture(const ref_ptr<Texture> &tex);
    /**
     * @return the associates texture.
     */
    const ref_ptr<Texture>& texture() const;

    /**
     * @param name the name of this texture in shader programs.
     */
    void set_name(const std::string &name);
    /**
     * @return the name of this texture in shader programs.
     */
    const std::string& name() const;

    /**
     * @param samplerType the sampler name of this texture in shader programs.
     */
    void set_samplerType(const std::string &samplerType);
    /**
     * @return the name of this texture in shader programs.
     */
    const std::string& samplerType() const;

    /**
     * @param channel  the texture coordinate channel.
     */
    void set_texcoChannel(GLuint channel);
    /**
     * @return the texture coordinate channel.
     */
    GLuint texcoChannel() const;

    /**
     * @param blendMode Specifies how this texture should be mixed with existing
     * values.
     */
    void set_blendMode(BlendMode blendMode);
    /**
     * @param factor Specifies how this texture should be mixed with existing
     * pixels.
     */
    void set_blendFactor(GLfloat factor);
    /**
     * Specifies how this texture should be mixed with existing
     * values.
     * @param blendFunction user defined GLSL function.
     * @param blendName function name of user defined GLSL function.
     */
    void set_blendFunction(const std::string &blendFunction, const std::string &blendName);

    /**
     * @param mapping Specifies how a texture should be mapped on geometry.
     */
    void set_mapping(Mapping mapping);
    /**
     * Specifies how a texture should be mapped on geometry.
     * @param blendFunction user defined GLSL function.
     * @param blendName name of user defined GLSL function.
     */
    void set_mappingFunction(const std::string &blendFunction, const std::string &blendName);

    /**
     * @param id Defines what is affected by the texture.
     */
    void set_mapTo(MapTo id);

    /**
     * Specifies how a texture should be sampled.
     * @param transferKey GLSL include key for transfer function.
     * @param transferName name of the transfer function.
     */
    void set_texelTransferKey(const std::string &transferKey, const std::string &transferName="");
    /**
     * Specifies how a texture should be sampled.
     * For example to scale each texel by 2.0 you can define following
     * transfer function: 'void transfer(inout vec4 texel) { texel *= 2.0; }'
     * @param transferFunction user defined GLSL function.
     * @param transferName name of user defined GLSL function.
     */
    void set_texelTransferFunction(const std::string &transferFunction, const std::string &transferName);

    /**
     * @param mode Specifies how texture coordinates are transfered before sampling.
     */
    void set_texcoTransfer(TransferTexco mode);
    /**
     * Specifies how texture coordinates are transfered before sampling.
     * @param transferKey GLSL include key for transfer function.
     * @param transferName name of the transfer function.
     */
    void set_texcoTransferKey(const std::string &transferKey, const std::string &transferName="");
    /**
     * Specifies how texture coordinates are transfered before sampling.
     * @param transferFunction user defined GLSL function.
     * @param transferName  name of user defined GLSL function.
     */
    void set_texcoTransferFunction(const std::string &transferFunction, const std::string &transferName);

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

    // override
    void enable(RenderState *rs);
    void disable(RenderState *rs);

  protected:
    static GLuint idCounter_;

    GLuint stateID_;

    ref_ptr<Texture> texture_;
    std::string name_;
    std::string samplerType_;

    BlendMode blendMode_;
    GLfloat blendFactor_;
    std::string blendFunction_;
    std::string blendName_;

    Mapping mapping_;
    std::string mappingFunction_;
    std::string mappingName_;

    MapTo mapTo_;

    std::string transferKey_;
    std::string transferFunction_;
    std::string transferName_;

    std::string transferTexcoKey_;
    std::string transferTexcoFunction_;
    std::string transferTexcoName_;

    GLuint texcoChannel_;
    GLint lastTexChannel_;

    GLboolean ignoreAlpha_;
  };

  std::ostream& operator<<(std::ostream &out, const TextureState::Mapping &v);
  std::istream& operator>>(std::istream &in, TextureState::Mapping &v);
  std::ostream& operator<<(std::ostream &out, const TextureState::MapTo &v);
  std::istream& operator>>(std::istream &in, TextureState::MapTo &v);
  std::ostream& operator<<(std::ostream &out, const TextureState::TransferTexco &v);
  std::istream& operator>>(std::istream &in, TextureState::TransferTexco &v);
} // namespace

namespace regen {
  /**
   * \brief Activates texture image when enabled.
   */
  class TextureSetIndex : public State
  {
  public:
    /**
     * @param tex texture reference.
     * @param objectIndex the buffer index that should be activated.
     */
    TextureSetIndex(const ref_ptr<Texture> &tex, GLuint objectIndex)
    : tex_(tex), objectIndex_(objectIndex) { }
    // override
    void enable(RenderState *rs)
    { tex_->set_objectIndex(objectIndex_); }

  protected:
    ref_ptr<Texture> tex_;
    GLuint objectIndex_;
  };

  /**
   * \brief Activates texture image when enabled.
   */
  class TextureNextIndex : public State
  {
  public:
    /**
     * @param tex texture reference.
     */
    TextureNextIndex(const ref_ptr<Texture> &tex)
    : tex_(tex) { }
    // override
    void enable(RenderState *rs)
    { tex_->nextObject(); }

  protected:
    ref_ptr<Texture> tex_;
  };
} // namespace

#endif /* TEXTURE_NODE_H_ */
