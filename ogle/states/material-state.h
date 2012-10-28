/*
 * material.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/utility/callable.h>
#include <ogle/utility/ref-ptr.h>

/**
 * Provides material related uniforms.
 */
class Material : public State
{
public:
  /**
   * Defines how light influences the final color.
   */
  enum Shading {
    GOURAD_SHADING,
    PHONG_SHADING,
    BLINN_SHADING,
    TOON_SHADING,
    ORENNAYER_SHADING,
    MINNAERT_SHADING,
    COOKTORRANCE_SHADING,
    NO_SHADING
  };

  /**
   * Default constructor.
   */
  Material();

  /**
   * Ambient material color.
   */
  void set_ambient(const Vec4f &v);
  /**
   * Ambient material color.
   */
  void set_ambient(GLuint numInstances, GLuint divisor, const Vec4f *v);
  /**
   * Ambient material color.
   */
  ref_ptr<ShaderInput4f>& ambient();

  /**
   * Diffuse material color.
   */
  void set_diffuse(const Vec4f &v);
  /**
   * Diffuse material color.
   */
  void set_diffuse(GLuint numInstances, GLuint divisor, const Vec4f *v);
  /**
   * Diffuse material color.
   */
  ref_ptr<ShaderInput4f>& diffuse();

  /**
   * Specular material color.
   */
  void set_specular(const Vec4f &v);
  /**
   * Specular material color.
   */
  void set_specular(GLuint numInstances, GLuint divisor, const Vec4f *v);
  /**
   * Specular material color.
   */
  ref_ptr<ShaderInput4f>& specular();

  /**
   * Emission material color.
   */
  void set_emission(const Vec4f &v);
  /**
   * Emission material color.
   */
  void set_emission(GLuint numInstances, GLuint divisor, const Vec4f *v);
  /**
   * Emission material color.
   */
  ref_ptr<ShaderInput4f>& emission();

  /**
   * The shininess exponent.
   */
  void set_shininess(GLfloat v);
  /**
   * The shininess exponent.
   */
  void set_shininess(GLuint numInstances, GLuint divisor, const GLfloat *v);
  /**
   * The shininess exponent.
   */
  ref_ptr<ShaderInput1f>& shininess();

  /**
   * The shininess strength.
   */
  void set_shininessStrength(GLfloat v);
  /**
   * The shininess strength.
   */
  void set_shininessStrength(GLuint numInstances, GLuint divisor, const GLfloat *v);
  /**
   * The shininess strength.
   */
  ref_ptr<ShaderInput1f>& shininessStrength();

  /**
   * The material roughness.
   */
  void set_roughness(GLfloat v);
  /**
   * The material roughness.
   */
  void set_roughness(GLuint numInstances, GLuint divisor, const GLfloat *v);
  /**
   * The material roughness.
   */
  ref_ptr<ShaderInput1f>& roughness();

  /**
   * The material darkness.
   */
  void set_darkness(GLfloat v);
  /**
   * The material darkness.
   */
  void set_darkness(GLuint numInstances, GLuint divisor, const GLfloat *v);
  /**
   * The material darkness.
   */
  ref_ptr<ShaderInput1f>& darkness();

  GLboolean useAlpha() const;
  void set_useAlpha(GLboolean v);

  /**
   * The material alpha.
   */
  void set_alpha(GLfloat v);
  /**
   * The material alpha.
   */
  void set_alpha(GLuint numInstances, GLuint divisor, const GLfloat *v);
  /**
   * The material alpha.
   */
  ref_ptr<ShaderInput1f>& alpha();

  /**
   * Index of refraction of the material.
   */
  void set_refractionIndex(GLfloat v);
  /**
   * Index of refraction of the material.
   */
  void set_refractionIndex(GLuint numInstances, GLuint divisor, const GLfloat *v);
  /**
   * Index of refraction of the material. This is used by some shading models,
   * e.g. Cook-Torrance. The value is the ratio of the speed of light in a
   * vacuum to the speed of light in the material (always >= 1.0 in the real world).
   */
  ref_ptr<ShaderInput1f>& refractionIndex();

  /**
   * Defines how faces are shaded (FILL/LINE/POINT).
   */
  void set_fillMode(GLenum mode);
  /**
   * Defines how faces are shaded (FILL/LINE/POINT).
   */
  GLenum fillMode() const;

  /**
   * Indicates if the material should be rendered two-sided.
   */
  void set_twoSided(GLboolean v);
  /**
   * Indicates if the material should be rendered two-sided.
   */
  GLboolean twoSided() const;

  /**
   * Defines the shading model to use.
   */
  void set_shading(Shading shading);
  /**
   * Defines the shading model to use.
   */
  Material::Shading shading() const;

  /**
   * Sets default material colors for jade.
   */
  void set_jade();
  /**
   * Sets default material colors for ruby.
   */
  void set_ruby();
  /**
   * Sets default material colors for chrome.
   */
  void set_chrome();
  /**
   * Sets default material colors for gold.
   */
  void set_gold();
  /**
   * Sets default material colors for copper.
   */
  void set_copper();
  /**
   * Sets default material colors for silver.
   */
  void set_silver();
  /**
   * Sets default material colors for pewter.
   */
  void set_pewter();

  /**
   * @return the material textures
   */
  vector< ref_ptr<Texture> >& textures();
  /**
   * Add a tecture to the material.
   */
  void addTexture(ref_ptr<TextureState> &tex);
  void removeTexture(Texture *tex);

  virtual void configureShader(ShaderConfig *cfg);

  virtual string name();
private:
  Shading shading_;
  GLboolean twoSided_;
  GLenum fillMode_;
  GLint lastFillMode_; // used to reset fill mode

  vector< ref_ptr<Texture> > textures_;
  ref_ptr<ShaderInput4f> materialDiffuse_;
  ref_ptr<ShaderInput4f> materialAmbient_;
  ref_ptr<ShaderInput4f> materialSpecular_;
  ref_ptr<ShaderInput4f> materialEmission_;
  ref_ptr<ShaderInput1f> materialShininess_;
  ref_ptr<ShaderInput1f> materialShininessStrength_;
  ref_ptr<ShaderInput1f> materialRoughness_;
  ref_ptr<ShaderInput1f> materialDarkness_;
  ref_ptr<ShaderInput1f> materialRefractionIndex_;

  GLboolean useAlpha_;
  ref_ptr<ShaderInput1f> materialAlpha_;

  ref_ptr<Callable> twoSidedSetter_;
  ref_ptr<Callable> twoSidedUnsetter_;
  ref_ptr<Callable> fillModeSetter_;
  ref_ptr<Callable> fillModeUnsetter_;

  Material(const Material&);
  Material& operator=(const Material &other);

  friend class SetFillMode ;
  friend class UnsetFillMode ;
};

#endif /* _MATERIAL_H_ */
