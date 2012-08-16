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
  // TODO: Material: use UBO
  // TODO: use constant unis
  ambientUniform_ = ref_ptr< ShaderInput4f >::manage(
      new ShaderInput4f("materialAmbient"));
  ambientUniform_->setUniformData(Vec4f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(ambientUniform_) );

  diffuseUniform_ = ref_ptr< ShaderInput4f >::manage(
      new ShaderInput4f("materialDiffuse"));
  diffuseUniform_->setUniformData(Vec4f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(diffuseUniform_) );

  specularUniform_ = ref_ptr< ShaderInput4f >::manage(
      new ShaderInput4f("materialSpecular"));
  specularUniform_->setUniformData(Vec4f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(specularUniform_) );

  emissionUniform_ = ref_ptr< ShaderInput4f >::manage(
      new ShaderInput4f("materialEmission"));
  emissionUniform_->setUniformData(Vec4f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(emissionUniform_) );

  shininessUniform_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialShininess"));
  shininessUniform_->setUniformData(0.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(shininessUniform_) );

  shininessStrengthUniform_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialShininessStrength"));
  shininessStrengthUniform_->setUniformData(1.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(shininessStrengthUniform_) );

  roughnessUniform_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialRoughness"));
  roughnessUniform_->setUniformData(0.5f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(roughnessUniform_) );

  darknessUniform_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialDarkness"));
  darknessUniform_->setUniformData(1.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(darknessUniform_) );

  alphaUniform_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialAlpha"));
  alphaUniform_->setUniformData(1.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(alphaUniform_) );

  reflectionUniform_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialReflection"));
  reflectionUniform_->setUniformData(0.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(reflectionUniform_) );

  refractionIndexUniform_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialRefractionIndex"));
  refractionIndexUniform_->setUniformData(0.95f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(refractionIndexUniform_) );

  set_jade();

  fillModeSetter_ = ref_ptr<Callable>::manage(new SetFillMode(this));
  fillModeUnsetter_ = ref_ptr<Callable>::manage(new UnsetFillMode(this));
  twoSidedSetter_ = ref_ptr<Callable>::manage(new SetTwoSided());
  twoSidedUnsetter_ = ref_ptr<Callable>::manage(new UnsetTwoSided());
}

string Material::name()
{
  return "Material";
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
  ambientUniform_->setUniformData(v);
}
void Material::set_ambient(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  ambientUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
const Vec4f& Material::ambient() const
{
  return ambientUniform_->getVertex4f(0);
}

void Material::set_diffuse(const Vec4f &v)
{
  diffuseUniform_->setUniformData(v);
}
void Material::set_diffuse(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  diffuseUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
const Vec4f& Material::diffuse() const
{
  return diffuseUniform_->getVertex4f(0);
}

void Material::set_specular(const Vec4f &v)
{
  specularUniform_->setUniformData(v);
}
void Material::set_specular(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  specularUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
const Vec4f& Material::specular() const
{
  return specularUniform_->getVertex4f(0);
}

void Material::set_emission(const Vec4f &v)
{
  emissionUniform_->setUniformData(v);
}
void Material::set_emission(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  emissionUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
const Vec4f& Material::emission() const
{
  return emissionUniform_->getVertex4f(0);
}

void Material::set_shininess(GLfloat shininess)
{
  shininessUniform_->setUniformData(shininess);
}
void Material::set_shininess(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  shininessUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
GLfloat Material::shininess() const
{
  return shininessUniform_->getVertex1f(0);
}

void Material::set_shininessStrength(GLfloat v)
{
  shininessStrengthUniform_->setUniformData(v);
}
void Material::set_shininessStrength(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  shininessStrengthUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
GLfloat Material::shininessStrength() const
{
  return shininessStrengthUniform_->getVertex1f(0);
}

void Material::set_roughness(GLfloat roughness)
{
  roughnessUniform_->setUniformData(roughness);
}
void Material::set_roughness(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  roughnessUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
GLfloat Material::roughness() const
{
  return roughnessUniform_->getVertex1f(0);
}

void Material::set_darkness(float darkness)
{
  darknessUniform_->setUniformData(darkness);
}
void Material::set_darkness(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  darknessUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
GLfloat Material::darkness() const
{
  return darknessUniform_->getVertex1f(0);
}

void Material::set_alpha(GLfloat alpha)
{
  alphaUniform_->setUniformData(alpha);
}
void Material::set_alpha(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  alphaUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
GLfloat Material::alpha() const
{
  return alphaUniform_->getVertex1f(0);
}

void Material::set_reflection(GLfloat reflection)
{
  reflectionUniform_->setUniformData(reflection);
}
void Material::set_reflection(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  reflectionUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
GLfloat Material::reflection() const
{
  return reflectionUniform_->getVertex1f(0);
}

void Material::set_refractionIndex(GLfloat refractionIndex)
{
  refractionIndexUniform_->setUniformData(refractionIndex);
}
void Material::set_refractionIndex(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  refractionIndexUniform_->setInstanceData(numInstance, divisor, (byte*)v);
}
GLfloat Material::refractionIndex() const
{
  return refractionIndexUniform_->getVertex1f(0);
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
GLboolean Material::twoSided() const
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
  ambientUniform_->setUniformData( Vec4f( 0.14, 0.22, 0.16, 0.9 ) );
  diffuseUniform_->setUniformData( Vec4f( 0.54, 0.89, 0.63, 0.9 ) );
  specularUniform_->setUniformData( Vec4f( 0.32, 0.32, 0.32, 0.9 ) );
  emissionUniform_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->setUniformData( 12.8 );
}
void Material::set_ruby()
{
  ambientUniform_->setUniformData( Vec4f( 0.17, 0.01, 0.01, 0.5 ) );
  diffuseUniform_->setUniformData( Vec4f( 0.61, 0.04, 0.04, 0.5 ) );
  specularUniform_->setUniformData( Vec4f( 0.73, 0.63, 0.63, 0.5 ) );
  emissionUniform_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->setUniformData( 76.8 );
}
void Material::set_chrome()
{
  ambientUniform_->setUniformData( Vec4f( 0.25, 0.25, 0.25, 1.0 ) );
  diffuseUniform_->setUniformData( Vec4f( 0.40, 0.40, 0.40, 1.0 ) );
  specularUniform_->setUniformData( Vec4f( 0.77, 0.77, 0.77, 1.0 ) );
  emissionUniform_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->setUniformData( 76.8 );
}
void Material::set_gold()
{
  ambientUniform_->setUniformData( Vec4f( 0.25, 0.20, 0.07, 1.0 ) );
  diffuseUniform_->setUniformData( Vec4f( 0.75, 0.61, 0.23, 1.0 ) );
  specularUniform_->setUniformData( Vec4f( 0.63, 0.65, 0.37, 1.0 ) );
  emissionUniform_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->setUniformData( 51.2 );
}
void Material::set_copper()
{
  ambientUniform_->setUniformData( Vec4f( 0.19, 0.07, 0.02, 1.0 ) );
  diffuseUniform_->setUniformData( Vec4f( 0.70, 0.27, 0.08, 1.0 ) );
  specularUniform_->setUniformData( Vec4f( 0.26, 0.14, 0.09, 1.0 ) );
  emissionUniform_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->setUniformData( 12.8 );
}
void Material::set_silver()
{
  ambientUniform_->setUniformData( Vec4f( 0.19, 0.19, 0.19, 1.0 ) );
  diffuseUniform_->setUniformData( Vec4f( 0.51, 0.51, 0.51, 1.0 ) );
  specularUniform_->setUniformData( Vec4f( 0.51, 0.51, 0.51, 1.0 ) );
  emissionUniform_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->setUniformData( 51.2 );
}
void Material::set_pewter()
{
  ambientUniform_->setUniformData( Vec4f( 0.11, 0.06, 0.11, 1.0 ) );
  diffuseUniform_->setUniformData( Vec4f( 0.43, 0.47, 0.54, 1.0 ) );
  specularUniform_->setUniformData( Vec4f( 0.33, 0.33, 0.52, 1.0 ) );
  emissionUniform_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  shininessUniform_->setUniformData( 9.8 );
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
