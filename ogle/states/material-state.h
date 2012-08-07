/*
 * material.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/uniform.h>
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
   * Copy properties from other material.
   */
  void set(Material &other);

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
  const Vec4f& ambient() const;

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
  const Vec4f& diffuse() const;

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
  const Vec4f& specular() const;

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
  const Vec4f& emission() const;

  /**
   * The shininess exponent.
   */
  void set_shininess(float v);
  /**
   * The shininess exponent.
   */
  void set_shininess(GLuint numInstances, GLuint divisor, const float *v);
  /**
   * The shininess exponent.
   */
  float shininess() const;

  /**
   * The shininess strength.
   */
  void set_shininessStrength(float v);
  /**
   * The shininess strength.
   */
  void set_shininessStrength(GLuint numInstances, GLuint divisor, const float *v);
  /**
   * The shininess strength.
   */
  float shininessStrength() const;

  /**
   * The material roughness.
   */
  void set_roughness(float v);
  /**
   * The material roughness.
   */
  void set_roughness(GLuint numInstances, GLuint divisor, const float *v);
  /**
   * The material roughness.
   */
  float roughness() const;

  /**
   * The material darkness.
   */
  void set_darkness(float v);
  /**
   * The material darkness.
   */
  void set_darkness(GLuint numInstances, GLuint divisor, const float *v);
  /**
   * The material darkness.
   */
  float darkness() const;

  /**
   * The material alpha.
   */
  void set_alpha(float v);
  /**
   * The material alpha.
   */
  void set_alpha(GLuint numInstances, GLuint divisor, const float *v);
  /**
   * The material alpha.
   */
  float alpha() const;

  /**
   * The material alpha.
   */
  void set_reflection(float v);
  /**
   * The material alpha.
   */
  void set_reflection(GLuint numInstances, GLuint divisor, const float *v);
  /**
   * The material alpha.
   */
  float reflection() const;

  /**
   * Index of refraction of the material.
   */
  void set_refractionIndex(float v);
  /**
   * Index of refraction of the material.
   */
  void set_refractionIndex(GLuint numInstances, GLuint divisor, const float *v);
  /**
   * Index of refraction of the material. This is used by some shading models,
   * e.g. Cook-Torrance. The value is the ratio of the speed of light in a
   * vacuum to the speed of light in the material (always >= 1.0 in the real world).
   */
  float refractionIndex() const;

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
  void set_twoSided(bool v);
  /**
   * Indicates if the material should be rendered two-sided.
   */
  bool twoSided() const;

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
  void addTexture(ref_ptr<Texture> tex);

  virtual void configureShader(ShaderConfiguration *cfg);
private:
  Shading shading_;
  bool twoSided_;
  GLenum fillMode_;
  GLint lastFillMode_; // used to reset fill mode

  vector< ref_ptr<Texture> > textures_;
  ref_ptr<UniformVec4> diffuseUniform_;
  ref_ptr<UniformVec4> ambientUniform_;
  ref_ptr<UniformVec4> specularUniform_;
  ref_ptr<UniformVec4> emissionUniform_;
  ref_ptr<UniformFloat> shininessUniform_;
  ref_ptr<UniformFloat> shininessStrengthUniform_;
  ref_ptr<UniformFloat> roughnessUniform_;
  ref_ptr<UniformFloat> darknessUniform_;
  ref_ptr<UniformFloat> alphaUniform_;
  ref_ptr<UniformFloat> reflectionUniform_;
  ref_ptr<UniformFloat> refractionIndexUniform_;

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
