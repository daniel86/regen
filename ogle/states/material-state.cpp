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
: ShaderInputState(),
  shading_(DEFERRED_PHONG_SHADING),
  twoSided_(GL_FALSE),
  fillMode_(GL_FILL),
  textures_(0)
{
  materialAmbient_ = ref_ptr< ShaderInput3f >::manage(
      new ShaderInput3f("matAmbient"));
  materialAmbient_->setUniformData(Vec3f(0.0f));
  setInput(ref_ptr<ShaderInput>::cast(materialAmbient_));

  materialDiffuse_ = ref_ptr< ShaderInput3f >::manage(
      new ShaderInput3f("matDiffuse"));
  materialDiffuse_->setUniformData(Vec3f(1.0f));
  setInput(ref_ptr<ShaderInput>::cast(materialDiffuse_));

  materialSpecular_ = ref_ptr< ShaderInput3f >::manage(
      new ShaderInput3f("matSpecular"));
  materialSpecular_->setUniformData(Vec3f(0.0f));
  setInput(ref_ptr<ShaderInput>::cast(materialSpecular_));

  materialShininess_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("matShininess"));
  materialShininess_->setUniformData(0.0f);
  setInput(ref_ptr<ShaderInput>::cast(materialShininess_));

  materialAlpha_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("matAlpha"));
  materialAlpha_->setUniformData(1.0f);
  setInput(ref_ptr<ShaderInput>::cast(materialAlpha_));

  materialRefractionIndex_ = ref_ptr< ShaderInput1f >::manage(
      new ShaderInput1f("matRefractionIndex"));
  materialRefractionIndex_->setUniformData(0.95f);
  setInput(ref_ptr<ShaderInput>::cast(materialRefractionIndex_));

  fillModeState_ = ref_ptr<State>::manage(new FillModeState(this));
  twoSidedState_ = ref_ptr<State>::manage(new TwoSidedState);

  shaderDefine("HAS_MATERIAL", "TRUE");
  set_shading(Material::DEFERRED_PHONG_SHADING);
}

const ref_ptr<ShaderInput3f>& Material::ambient() const
{
  return materialAmbient_;
}
const ref_ptr<ShaderInput3f>& Material::diffuse() const
{
  return materialDiffuse_;
}
const ref_ptr<ShaderInput3f>& Material::specular() const
{
  return materialSpecular_;
}
const ref_ptr<ShaderInput1f>& Material::shininess() const
{
  return materialShininess_;
}

const ref_ptr<ShaderInput1f>& Material::alpha() const
{
  return materialAlpha_;
}

const ref_ptr<ShaderInput1f>& Material::refractionIndex() const
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
  shaderDefine("HAS_TWO_SIDES", twoSided ? "TRUE" : "FALSE");
}
GLboolean Material::twoSided() const
{
  return twoSided_;
}

void Material::set_shading(Shading shading)
{
  shading_ = shading;
  switch(shading) {
  case Material::DEFERRED_PHONG_SHADING:
    shaderDefine("SHADING", "DEFERRED_PHONG");
    break;
  case Material::NO_SHADING:
    shaderDefine("SHADING", "NONE");
    break;
  }
}
Material::Shading Material::shading() const
{
  return shading_;
}

void Material::set_jade()
{
  materialAmbient_->setUniformData( Vec3f( 0.14, 0.22, 0.16 ) );
  materialDiffuse_->setUniformData( Vec3f( 0.54, 0.89, 0.63 ) );
  materialSpecular_->setUniformData( Vec3f( 0.32, 0.32, 0.32 ) );
  materialShininess_->setUniformData( 12.8 );
}
void Material::set_ruby()
{
  materialAmbient_->setUniformData( Vec3f( 0.17, 0.01, 0.01 ) );
  materialDiffuse_->setUniformData( Vec3f( 0.61, 0.04, 0.04 ) );
  materialSpecular_->setUniformData( Vec3f( 0.73, 0.63, 0.63 ) );
  materialShininess_->setUniformData( 76.8 );
}
void Material::set_chrome()
{
  materialAmbient_->setUniformData( Vec3f( 0.25, 0.25, 0.25 ) );
  materialDiffuse_->setUniformData( Vec3f( 0.40, 0.40, 0.40 ) );
  materialSpecular_->setUniformData( Vec3f( 0.77, 0.77, 0.77 ) );
  materialShininess_->setUniformData( 76.8 );
}
void Material::set_gold()
{
  materialAmbient_->setUniformData( Vec3f( 0.25, 0.20, 0.07 ) );
  materialDiffuse_->setUniformData( Vec3f( 0.75, 0.61, 0.23 ) );
  materialSpecular_->setUniformData( Vec3f( 0.63, 0.65, 0.37 ) );
  materialShininess_->setUniformData( 51.2 );
}
void Material::set_copper()
{
  materialAmbient_->setUniformData( Vec3f( 0.19, 0.07, 0.02 ) );
  materialDiffuse_->setUniformData( Vec3f( 0.70, 0.27, 0.08 ) );
  specular()->setUniformData( Vec3f( 0.26, 0.14, 0.09 ) );
  materialShininess_->setUniformData( 12.8 );
}
void Material::set_silver()
{
  materialAmbient_->setUniformData( Vec3f( 0.19, 0.19, 0.19 ) );
  materialDiffuse_->setUniformData( Vec3f( 0.51, 0.51, 0.51 ) );
  materialSpecular_->setUniformData( Vec3f( 0.51, 0.51, 0.51 ) );
  materialShininess_->setUniformData( 51.2 );
}
void Material::set_pewter()
{
  materialAmbient_->setUniformData( Vec3f( 0.11, 0.06, 0.11 ) );
  materialDiffuse_->setUniformData( Vec3f( 0.43, 0.47, 0.54 ) );
  materialSpecular_->setUniformData( Vec3f( 0.33, 0.33, 0.52 ) );
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
