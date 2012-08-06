/*
 * material.cpp
 *
 *  Created on: 22.03.2011
 *      Author: daniel
 */

#include "material-state.h"
#include "texture-state.h"

class SetTwoSided : public Callable {
  public: virtual void call() {
    glDisable(GL_CULL_FACE);
  }
};
class UnsetTwoSided : public Callable {
  public: virtual void call() {
    glEnable(GL_CULL_FACE);
  }
};
class SetFillMode : public Callable {
  public: SetFillMode(Material *m) : Callable(), m_(m) {}
  virtual void call() {
    glGetIntegerv(GL_POLYGON_MODE, &m_->lastFillMode_);
    glPolygonMode(GL_FRONT_AND_BACK, m_->fillMode_);
  }
  Material *m_;
};
class UnsetFillMode : public Callable {
  public: UnsetFillMode(Material *m) : Callable(), m_(m) {}
  virtual void call() {
    glPolygonMode(GL_FRONT_AND_BACK, m_->lastFillMode_);
  }
  Material *m_;
};

Material::Material()
: State(),
  textures_(0),
  shading_(PHONG_SHADING),
  fillMode_(GL_FILL),
  twoSided_(false)
{
  ambientUniform_ = ref_ptr< UniformVec4 >::manage(
      new UniformVec4("materialAmbient"));
  joinStates( ambientUniform_ );

  diffuseUniform_ = ref_ptr< UniformVec4 >::manage(
      new UniformVec4("materialDiffuse"));
  joinStates( diffuseUniform_ );

  specularUniform_ = ref_ptr< UniformVec4 >::manage(
      new UniformVec4("materialSpecular"));
  joinStates( specularUniform_ );

  emissionUniform_ = ref_ptr< UniformVec4 >::manage(
      new UniformVec4("materialEmission"));
  joinStates( emissionUniform_ );

  shininessUniform_ = ref_ptr< UniformFloat >::manage(
      new UniformFloat("materialShininess"));
  joinStates( shininessUniform_ );

  shininessStrengthUniform_ = ref_ptr< UniformFloat >::manage(
      new UniformFloat("materialShininessStrength", 1.0f));

  roughnessUniform_ = ref_ptr< UniformFloat >::manage(
      new UniformFloat("materialRoughness", 0.5f));
  joinStates( roughnessUniform_ );

  darknessUniform_ = ref_ptr< UniformFloat >::manage(
      new UniformFloat("materialDarkness", 1.0f));
  joinStates( darknessUniform_ );

  alphaUniform_ = ref_ptr< UniformFloat >::manage(
      new UniformFloat("materialAlpha", 1.0f));
  joinStates( alphaUniform_ );

  reflectionUniform_ = ref_ptr< UniformFloat >::manage(
      new UniformFloat("materialReflection", 0.0f));
  joinStates( reflectionUniform_ );

  refractionIndexUniform_ = ref_ptr< UniformFloat >::manage(
      new UniformFloat("materialRefractionIndex", 0.95f));
  joinStates( refractionIndexUniform_ );

  set_jade();

  fillModeSetter_ = ref_ptr<Callable>::manage(new SetFillMode(this));
  fillModeUnsetter_ = ref_ptr<Callable>::manage(new UnsetFillMode(this));
  twoSidedSetter_ = ref_ptr<Callable>::manage(new SetTwoSided());
  twoSidedUnsetter_ = ref_ptr<Callable>::manage(new UnsetTwoSided());
}

void Material::set(Material &other)
{
  set_ambient( other.ambient() );
  set_diffuse( other.diffuse() );
  set_specular( other.specular() );
  set_emission( other.emission() );
  set_shininess( other.shininess() );
  set_fillMode( other.fillMode() );
  set_twoSided( other.twoSided() );
  set_shininess( other.shininess() );
  set_roughness( other.roughness() );
  set_darkness( other.darkness() );
  set_shading( other.shading() );
  for(vector< ref_ptr<Texture> >::iterator
      it=other.textures_.begin(); it!=other.textures_.end(); ++it)
  {
    addTexture( *it );
  }
}

void Material::set_ambient(const Vec4f &v)
{
  ambientUniform_->set_value(v);
}
void Material::set_ambient(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  ambientUniform_->set_valuesInstanced(numInstance, divisor, v);
}
const Vec4f& Material::ambient() const
{
  return ambientUniform_->value();
}

void Material::set_diffuse(const Vec4f &v)
{
  diffuseUniform_->set_value(v);
}
void Material::set_diffuse(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  diffuseUniform_->set_valuesInstanced(numInstance, divisor, v);
}
const Vec4f& Material::diffuse() const
{
  return diffuseUniform_->value();
}

void Material::set_specular(const Vec4f &v)
{
  specularUniform_->set_value(v);
}
void Material::set_specular(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  specularUniform_->set_valuesInstanced(numInstance, divisor, v);
}
const Vec4f& Material::specular() const
{
  return specularUniform_->value();
}

void Material::set_emission(const Vec4f &v)
{
  emissionUniform_->set_value(v);
}
void Material::set_emission(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  emissionUniform_->set_valuesInstanced(numInstance, divisor, v);
}
const Vec4f& Material::emission() const
{
  return emissionUniform_->value();
}

void Material::set_shininess(float shininess)
{
  shininessUniform_->set_value(shininess);
}
void Material::set_shininess(GLuint numInstance, GLuint divisor, const float *v)
{
  shininessUniform_->set_valuesInstanced(numInstance, divisor, v);
}
float Material::shininess() const
{
  return shininessUniform_->value();
}

void Material::set_shininessStrength(float v)
{
  shininessStrengthUniform_->set_value(v);
}
void Material::set_shininessStrength(GLuint numInstance, GLuint divisor, const float *v)
{
  shininessStrengthUniform_->set_valuesInstanced(numInstance, divisor, v);
}
float Material::shininessStrength() const
{
  return shininessStrengthUniform_->value();
}

void Material::set_roughness(float roughness)
{
  roughnessUniform_->set_value(roughness);
}
void Material::set_roughness(GLuint numInstance, GLuint divisor, const float *v)
{
  roughnessUniform_->set_valuesInstanced(numInstance, divisor, v);
}
float Material::roughness() const
{
  return roughnessUniform_->value();
}

void Material::set_darkness(float darkness)
{
  darknessUniform_->set_value(darkness);
}
void Material::set_darkness(GLuint numInstance, GLuint divisor, const float *v)
{
  darknessUniform_->set_valuesInstanced(numInstance, divisor, v);
}
float Material::darkness() const
{
  return darknessUniform_->value();
}

void Material::set_alpha(float alpha)
{
  alphaUniform_->set_value(alpha);
}
void Material::set_alpha(GLuint numInstance, GLuint divisor, const float *v)
{
  alphaUniform_->set_valuesInstanced(numInstance, divisor, v);
}
float Material::alpha() const
{
  return alphaUniform_->value();
}

void Material::set_reflection(float reflection)
{
  reflectionUniform_->set_value(reflection);
}
void Material::set_reflection(GLuint numInstance, GLuint divisor, const float *v)
{
  reflectionUniform_->set_valuesInstanced(numInstance, divisor, v);
}
float Material::reflection() const
{
  return reflectionUniform_->value();
}

void Material::set_refractionIndex(float refractionIndex)
{
  refractionIndexUniform_->set_value(refractionIndex);
}
void Material::set_refractionIndex(GLuint numInstance, GLuint divisor, const float *v)
{
  refractionIndexUniform_->set_valuesInstanced(numInstance, divisor, v);
}
float Material::refractionIndex() const
{
  return refractionIndexUniform_->value();
}

void Material::set_fillMode(GLenum fillMode)
{
  if(fillMode == fillMode_) return;
  if(fillMode_ == GL_FILL) {
    fillMode_ = fillMode;
    addEnabler(fillModeSetter_);
    addDisabler(fillModeUnsetter_);
  } else if(fillMode == GL_FILL) {
    fillMode_ = fillMode;
    removeEnabler(fillModeSetter_);
    removeDisabler(fillModeUnsetter_);
  } else {
    fillMode_ = fillMode;
  }
}
GLenum Material::fillMode() const
{
  return fillMode_;
}

void Material::set_twoSided(bool twoSided)
{
  if(twoSided_) {
    removeEnabler(twoSidedSetter_);
    removeDisabler(twoSidedUnsetter_);
  }
  twoSided_ = twoSided;
  if(twoSided_) {
    addEnabler(twoSidedSetter_);
    addDisabler(twoSidedUnsetter_);
  }
}
bool Material::twoSided() const
{
  return twoSided_;
}

void Material::set_shading(Shading shading)
{
  shading_ = shading;
}
Material::Shading Material::shading() const
{
  return shading_;
}

void Material::set_jade()
{
  ambientUniform_->set_value( Vec4f( 0.14, 0.22, 0.16, 0.9 ) );
  diffuseUniform_->set_value( Vec4f( 0.54, 0.89, 0.63, 0.9 ) );
  specularUniform_->set_value( Vec4f( 0.32, 0.32, 0.32, 0.9 ) );
  emissionUniform_->set_value( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->set_value( 12.8 );
}
void Material::set_ruby()
{
  ambientUniform_->set_value( Vec4f( 0.17, 0.01, 0.01, 0.5 ) );
  diffuseUniform_->set_value( Vec4f( 0.61, 0.04, 0.04, 0.5 ) );
  specularUniform_->set_value( Vec4f( 0.73, 0.63, 0.63, 0.5 ) );
  emissionUniform_->set_value( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->set_value( 76.8 );
}
void Material::set_chrome()
{
  ambientUniform_->set_value( Vec4f( 0.25, 0.25, 0.25, 1.0 ) );
  diffuseUniform_->set_value( Vec4f( 0.40, 0.40, 0.40, 1.0 ) );
  specularUniform_->set_value( Vec4f( 0.77, 0.77, 0.77, 1.0 ) );
  emissionUniform_->set_value( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->set_value( 76.8 );
}
void Material::set_gold()
{
  ambientUniform_->set_value( Vec4f( 0.25, 0.20, 0.07, 1.0 ) );
  diffuseUniform_->set_value( Vec4f( 0.75, 0.61, 0.23, 1.0 ) );
  specularUniform_->set_value( Vec4f( 0.63, 0.65, 0.37, 1.0 ) );
  emissionUniform_->set_value( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->set_value( 51.2 );
}
void Material::set_copper()
{
  ambientUniform_->set_value( Vec4f( 0.19, 0.07, 0.02, 1.0 ) );
  diffuseUniform_->set_value( Vec4f( 0.70, 0.27, 0.08, 1.0 ) );
  specularUniform_->set_value( Vec4f( 0.26, 0.14, 0.09, 1.0 ) );
  emissionUniform_->set_value( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->set_value( 12.8 );
}
void Material::set_silver()
{
  ambientUniform_->set_value( Vec4f( 0.19, 0.19, 0.19, 1.0 ) );
  diffuseUniform_->set_value( Vec4f( 0.51, 0.51, 0.51, 1.0 ) );
  specularUniform_->set_value( Vec4f( 0.51, 0.51, 0.51, 1.0 ) );
  emissionUniform_->set_value( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->set_value( 51.2 );
}
void Material::set_pewter()
{
  ambientUniform_->set_value( Vec4f( 0.11, 0.06, 0.11, 1.0 ) );
  diffuseUniform_->set_value( Vec4f( 0.43, 0.47, 0.54, 1.0 ) );
  specularUniform_->set_value( Vec4f( 0.33, 0.33, 0.52, 1.0 ) );
  emissionUniform_->set_value( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->set_value( 9.8 );
}

vector< ref_ptr<Texture> >& Material::textures()
{
  return textures_;
}
void Material::addTexture(ref_ptr<Texture> tex)
{
  textures_.push_back(tex);
  ref_ptr<State> texState = ref_ptr<State>::manage(new TextureState(tex));
  joinStates(texState);
}

void Material::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setMaterial(this);
}
