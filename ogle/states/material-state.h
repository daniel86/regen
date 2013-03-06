/*
 * material.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <ogle/states/shader-input-state.h>
#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/utility/callable.h>
#include <ogle/utility/ref-ptr.h>

namespace ogle {

/**
 * Provides material related uniforms.
 */
class Material : public ShaderInputState
{
public:
  /**
   * Default constructor.
   */
  Material();

  /**
   * Ambient material color.
   */
  const ref_ptr<ShaderInput3f>& ambient() const;

  /**
   * Diffuse material color.
   */
  const ref_ptr<ShaderInput3f>& diffuse() const;

  /**
   * Specular material color.
   */
  const ref_ptr<ShaderInput3f>& specular() const;

  /**
   * The shininess exponent.
   */
  const ref_ptr<ShaderInput1f>& shininess() const;

  /**
   * The material alpha.
   */
  const ref_ptr<ShaderInput1f>& alpha() const;

  /**
   * Index of refraction of the material. This is used by some shading models,
   * e.g. Cook-Torrance. The value is the ratio of the speed of light in a
   * vacuum to the speed of light in the material (always >= 1.0 in the real world).
   */
  const ref_ptr<ShaderInput1f>& refractionIndex() const;

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
  void addTexture(const ref_ptr<TextureState> &tex);
  void removeTexture(Texture *tex);

private:
  GLenum fillMode_;

  vector< ref_ptr<Texture> > textures_;
  ref_ptr<ShaderInput3f> materialDiffuse_;
  ref_ptr<ShaderInput3f> materialAmbient_;
  ref_ptr<ShaderInput3f> materialSpecular_;
  ref_ptr<ShaderInput1f> materialShininess_;
  ref_ptr<ShaderInput1f> materialRefractionIndex_;
  ref_ptr<ShaderInput1f> materialAlpha_;

  ref_ptr<State> twoSidedState_;
  ref_ptr<State> fillModeState_;

  Material(const Material&);
  Material& operator=(const Material &other);

  friend class FillModeState ;
};

} // end ogle namespace

#endif /* _MATERIAL_H_ */
