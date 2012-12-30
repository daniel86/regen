/*
 * material.cpp
 *
 *  Created on: 22.03.2011
 *      Author: daniel
 */

#include "material-state.h"
#include "texture-state.h"

class TwoSidedState : public State {
public:
  virtual void enable(RenderState *state) {
    glDisable(GL_CULL_FACE);
  }
  virtual void disable(RenderState *state) {
    glEnable(GL_CULL_FACE);
  }
};
class FillModeState : public State {
public:
  FillModeState(Material *m)
  : State(), m_(m) {}
  virtual void enable(RenderState *state) {
    glGetIntegerv(GL_POLYGON_MODE, &m_->lastFillMode_);
    glPolygonMode(GL_FRONT_AND_BACK, m_->fillMode_);
  }
  virtual void disable(RenderState *state) {
    glPolygonMode(GL_FRONT_AND_BACK, m_->lastFillMode_);
  }
  Material *m_;
};

Material::Material()
: State(),
  shading_(DEFERRED_PHONG_SHADING),
  twoSided_(GL_FALSE),
  fillMode_(GL_FILL),
  textures_(0)
{
  materialAmbient_ = ref_ptr< ShaderInput3f >::manage(
      new ShaderInput3f("matAmbient"));
  materialAmbient_->setUniformData(Vec3f(0.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialAmbient_) );

  materialDiffuse_ = ref_ptr< ShaderInput3f >::manage(
      new ShaderInput3f("matDiffuse"));
  materialDiffuse_->setUniformData(Vec3f(1.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialDiffuse_) );

  materialSpecular_ = ref_ptr< ShaderInput3f >::manage(
      new ShaderInput3f("matSpecular"));
  materialSpecular_->setUniformData(Vec3f(0.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialSpecular_) );

  materialShininess_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("matShininess"));
  materialShininess_->setUniformData(0.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialShininess_) );

  materialAlpha_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("matAlpha"));
  materialAlpha_->setUniformData(1.0f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialAlpha_) );

  materialRefractionIndex_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("matRefractionIndex"));
  materialRefractionIndex_->setUniformData(0.95f);
  joinShaderInput( ref_ptr<ShaderInput>::cast(materialRefractionIndex_) );

  fillModeState_ = ref_ptr<State>::manage(new FillModeState(this));
  twoSidedState_ = ref_ptr<State>::manage(new TwoSidedState);
}

void Material::set_ambient(const Vec3f &v)
{
  materialAmbient_->setVertex3f(0,v);
}
void Material::set_ambient(GLuint numInstance, GLuint divisor, const Vec3f *v)
{
  materialAmbient_->setInstanceData(numInstance, divisor, (byte*)v);
}
const ref_ptr<ShaderInput3f>& Material::ambient()
{
  return materialAmbient_;
}

void Material::set_diffuse(const Vec3f &v)
{
  materialDiffuse_->setVertex3f(0,v);
}
void Material::set_diffuse(GLuint numInstance, GLuint divisor, const Vec3f *v)
{
  materialDiffuse_->setInstanceData(numInstance, divisor, (byte*)v);
}
const ref_ptr<ShaderInput3f>& Material::diffuse()
{
  return materialDiffuse_;
}

void Material::set_specular(const Vec3f &v)
{
  materialSpecular_->setVertex3f(0,v);
}
void Material::set_specular(GLuint numInstance, GLuint divisor, const Vec3f *v)
{
  materialSpecular_->setInstanceData(numInstance, divisor, (byte*)v);
}
const ref_ptr<ShaderInput3f>& Material::specular()
{
  return materialSpecular_;
}

void Material::set_shininess(GLfloat shininess)
{
  materialShininess_->setVertex1f(0,shininess);
}
void Material::set_shininess(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialShininess_->setInstanceData(numInstance, divisor, (byte*)v);
}
const ref_ptr<ShaderInput1f>& Material::shininess()
{
  return materialShininess_;
}

void Material::set_alpha(GLfloat alpha)
{
  materialAlpha_->setUniformData(alpha);
}
void Material::set_alpha(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialAlpha_->setInstanceData(numInstance, divisor, (byte*)v);
}
const ref_ptr<ShaderInput1f>& Material::alpha()
{
  return materialAlpha_;
}

void Material::set_refractionIndex(GLfloat refractionIndex)
{
  materialRefractionIndex_->setUniformData(refractionIndex);
}
void Material::set_refractionIndex(GLuint numInstance, GLuint divisor, const GLfloat *v)
{
  materialRefractionIndex_->setInstanceData(numInstance, divisor, (byte*)v);
}
const ref_ptr<ShaderInput1f>& Material::refractionIndex()
{
  return materialRefractionIndex_;
}

void Material::set_fillMode(GLenum fillMode)
{
  if(fillMode == fillMode_) return;
  if(fillMode_ == GL_FILL) {
    fillMode_ = fillMode;
    joinStates(fillModeState_);
  } else if(fillMode == GL_FILL) {
    fillMode_ = fillMode;
    disjoinStates(fillModeState_);
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
    disjoinStates(twoSidedState_);
  }
  twoSided_ = twoSided;
  if(twoSided_) {
    joinStates(twoSidedState_);
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
  set_ambient( Vec3f( 0.14, 0.22, 0.16 ) );
  set_diffuse( Vec3f( 0.54, 0.89, 0.63 ) );
  set_specular( Vec3f( 0.32, 0.32, 0.32 ) );
  materialShininess_->setUniformData( 12.8 );
}
void Material::set_ruby()
{
  set_ambient( Vec3f( 0.17, 0.01, 0.01 ) );
  set_diffuse( Vec3f( 0.61, 0.04, 0.04 ) );
  set_specular( Vec3f( 0.73, 0.63, 0.63 ) );
  materialShininess_->setUniformData( 76.8 );
}
void Material::set_chrome()
{
  set_ambient( Vec3f( 0.25, 0.25, 0.25 ) );
  set_diffuse( Vec3f( 0.40, 0.40, 0.40 ) );
  set_specular( Vec3f( 0.77, 0.77, 0.77 ) );
  materialShininess_->setUniformData( 76.8 );
}
void Material::set_gold()
{
  set_ambient( Vec3f( 0.25, 0.20, 0.07 ) );
  set_diffuse( Vec3f( 0.75, 0.61, 0.23 ) );
  set_specular( Vec3f( 0.63, 0.65, 0.37 ) );
  materialShininess_->setUniformData( 51.2 );
}
void Material::set_copper()
{
  set_ambient( Vec3f( 0.19, 0.07, 0.02 ) );
  set_diffuse( Vec3f( 0.70, 0.27, 0.08 ) );
  set_specular( Vec3f( 0.26, 0.14, 0.09 ) );
  materialShininess_->setUniformData( 12.8 );
}
void Material::set_silver()
{
  set_ambient( Vec3f( 0.19, 0.19, 0.19 ) );
  set_diffuse( Vec3f( 0.51, 0.51, 0.51 ) );
  set_specular( Vec3f( 0.51, 0.51, 0.51 ) );
  materialShininess_->setUniformData( 51.2 );
}
void Material::set_pewter()
{
  set_ambient( Vec3f( 0.11, 0.06, 0.11 ) );
  set_diffuse( Vec3f( 0.43, 0.47, 0.54 ) );
  set_specular( Vec3f( 0.33, 0.33, 0.52 ) );
  materialShininess_->setUniformData( 9.8 );
}

vector< ref_ptr<Texture> >& Material::textures()
{
  return textures_;
}
void Material::addTexture(ref_ptr<TextureState> &tex)
{
  textures_.push_back(tex->texture());
  joinStates(ref_ptr<State>::cast(tex));
}
void Material::removeTexture(Texture *tex)
{
  for(vector< ref_ptr<Texture> >::iterator
      it=textures_.begin(); it!=textures_.end(); ++it)
  {
    if(it->get()==tex) {
      textures_.erase(it);
      break;
    }
  }
  for(list< ref_ptr<State> >::iterator
      it=joined_.begin(); it!=joined_.end(); ++it)
  {
    State *s = it->get();
    TextureState *ts = dynamic_cast<TextureState*>(s);
    if(ts && ts->texture().get()==tex) {
      disjoinStates(*it);
      break;
    }
  }
}

void Material::configureShader(ShaderConfig *cfg)
{
  State::configureShader(cfg);
  cfg->setMaterial(this);
}
