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
  materialAmbient_ = ref_ptr< ShaderInput4f >::manage(
      new ShaderInput4f("materialAmbient"));
  materialAmbient_->setUniformData(Vec4f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialAmbient_) );

  materialDiffuse_ = ref_ptr< ShaderInput4f >::manage(
      new ShaderInput4f("materialDiffuse"));
  materialDiffuse_->setUniformData(Vec4f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialDiffuse_) );

  materialSpecular_ = ref_ptr< ShaderInput4f >::manage(
      new ShaderInput4f("materialSpecular"));
  materialSpecular_->setUniformData(Vec4f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialSpecular_) );

  materialEmission_ = ref_ptr< ShaderInput4f >::manage(
      new ShaderInput4f("materialEmission"));
  materialEmission_->setUniformData(Vec4f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialEmission_) );

  materialShininess_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialShininess"));
  materialShininess_->setUniformData(0.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialShininess_) );

  materialShininessStrength_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialShininessStrength"));
  materialShininessStrength_->setUniformData(1.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialShininessStrength_) );

  materialRoughness_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialRoughness"));
  materialRoughness_->setUniformData(0.5f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialRoughness_) );

  materialDarkness_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialDarkness"));
  materialDarkness_->setUniformData(1.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialDarkness_) );

  materialAlpha_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialAlpha"));
  materialAlpha_->setUniformData(1.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialAlpha_) );

  materialReflection_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialReflection"));
  materialReflection_->setUniformData(0.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialReflection_) );

  materialRefractionIndex_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("materialRefractionIndex"));
  materialRefractionIndex_->setUniformData(0.95f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialRefractionIndex_) );

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

void Material::set_ambient(const Vec4f &v)
{
  materialAmbient_->setUniformData(v);
}
void Material::set_ambient(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  materialAmbient_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput4f>& Material::ambient()
{
  return materialAmbient_;
}

void Material::set_diffuse(const Vec4f &v)
{
  materialDiffuse_->setUniformData(v);
}
void Material::set_diffuse(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  materialDiffuse_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput4f>& Material::diffuse()
{
  return materialDiffuse_;
}

void Material::set_specular(const Vec4f &v)
{
  materialSpecular_->setUniformData(v);
}
void Material::set_specular(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  materialSpecular_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput4f>& Material::specular()
{
  return materialSpecular_;
}

void Material::set_emission(const Vec4f &v)
{
  materialEmission_->setUniformData(v);
}
void Material::set_emission(GLuint numInstance, GLuint divisor, const Vec4f *v)
{
  materialEmission_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput4f>& Material::emission()
{
  return materialEmission_;
}

void Material::set_shininess(GLfloat shininess)
{
  materialShininess_->setUniformData(shininess);
}
void Material::set_shininess(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialShininess_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput1f>& Material::shininess()
{
  return materialShininess_;
}

void Material::set_shininessStrength(GLfloat v)
{
  materialShininessStrength_->setUniformData(v);
}
void Material::set_shininessStrength(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialShininessStrength_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput1f>& Material::shininessStrength()
{
  return materialShininessStrength_;
}

void Material::set_roughness(GLfloat roughness)
{
  materialRoughness_->setUniformData(roughness);
}
void Material::set_roughness(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialRoughness_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput1f>& Material::roughness()
{
  return materialRoughness_;
}

void Material::set_darkness(GLfloat darkness)
{
  materialDarkness_->setUniformData(darkness);
}
void Material::set_darkness(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialDarkness_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput1f>& Material::darkness()
{
  return materialDarkness_;
}

void Material::set_alpha(GLfloat alpha)
{
  materialAlpha_->setUniformData(alpha);
}
void Material::set_alpha(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialAlpha_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput1f>& Material::alpha()
{
  return materialAlpha_;
}

void Material::set_reflection(GLfloat reflection)
{
  materialReflection_->setUniformData(reflection);
}
void Material::set_reflection(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialReflection_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput1f>& Material::reflection()
{
  return materialReflection_;
}

void Material::set_refractionIndex(GLfloat refractionIndex)
{
  materialRefractionIndex_->setUniformData(refractionIndex);
}
void Material::set_refractionIndex(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialRefractionIndex_->setInstanceData(numInstance, divisor, (byte*)v);
}
ref_ptr<ShaderInput1f>& Material::refractionIndex()
{
  return materialRefractionIndex_;
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

void Material::set_twoSided(GLboolean twoSided)
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
  materialAmbient_->setUniformData( Vec4f( 0.14, 0.22, 0.16, 0.9 ) );
  materialDiffuse_->setUniformData( Vec4f( 0.54, 0.89, 0.63, 0.9 ) );
  materialSpecular_->setUniformData( Vec4f( 0.32, 0.32, 0.32, 0.9 ) );
  materialEmission_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  materialShininess_->setUniformData( 12.8 );
}
void Material::set_ruby()
{
  materialAmbient_->setUniformData( Vec4f( 0.17, 0.01, 0.01, 0.5 ) );
  materialDiffuse_->setUniformData( Vec4f( 0.61, 0.04, 0.04, 0.5 ) );
  materialSpecular_->setUniformData( Vec4f( 0.73, 0.63, 0.63, 0.5 ) );
  materialEmission_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  materialShininess_->setUniformData( 76.8 );
}
void Material::set_chrome()
{
  materialAmbient_->setUniformData( Vec4f( 0.25, 0.25, 0.25, 1.0 ) );
  materialDiffuse_->setUniformData( Vec4f( 0.40, 0.40, 0.40, 1.0 ) );
  materialSpecular_->setUniformData( Vec4f( 0.77, 0.77, 0.77, 1.0 ) );
  materialEmission_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  materialShininess_->setUniformData( 76.8 );
}
void Material::set_gold()
{
  materialAmbient_->setUniformData( Vec4f( 0.25, 0.20, 0.07, 1.0 ) );
  materialDiffuse_->setUniformData( Vec4f( 0.75, 0.61, 0.23, 1.0 ) );
  materialSpecular_->setUniformData( Vec4f( 0.63, 0.65, 0.37, 1.0 ) );
  materialEmission_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  materialShininess_->setUniformData( 51.2 );
}
void Material::set_copper()
{
  materialAmbient_->setUniformData( Vec4f( 0.19, 0.07, 0.02, 1.0 ) );
  materialDiffuse_->setUniformData( Vec4f( 0.70, 0.27, 0.08, 1.0 ) );
  materialSpecular_->setUniformData( Vec4f( 0.26, 0.14, 0.09, 1.0 ) );
  materialEmission_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  materialShininess_->setUniformData( 12.8 );
}
void Material::set_silver()
{
  materialAmbient_->setUniformData( Vec4f( 0.19, 0.19, 0.19, 1.0 ) );
  materialDiffuse_->setUniformData( Vec4f( 0.51, 0.51, 0.51, 1.0 ) );
  materialSpecular_->setUniformData( Vec4f( 0.51, 0.51, 0.51, 1.0 ) );
  materialEmission_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  materialShininess_->setUniformData( 51.2 );
}
void Material::set_pewter()
{
  materialAmbient_->setUniformData( Vec4f( 0.11, 0.06, 0.11, 1.0 ) );
  materialDiffuse_->setUniformData( Vec4f( 0.43, 0.47, 0.54, 1.0 ) );
  materialSpecular_->setUniformData( Vec4f( 0.33, 0.33, 0.52, 1.0 ) );
  materialEmission_->setUniformData( Vec4f( 0.0, 0.0, 0.0, 0.0 ) );
  materialShininess_->setUniformData( 9.8 );
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
