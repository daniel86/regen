/*
 * volumetric-fog.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/box.h>
#include <ogle/states/atomic-states.h>

#include "volumetric-fog.h"
using namespace ogle;

////////////////
////////////////

VolumetricPointFog::VolumetricPointFog() : State()
{
  mesh_ = ref_ptr<MeshState>::cast( Box::getUnitCube() );
  joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));
  fogShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(fogShader_));
}

void VolumetricPointFog::createShader(ShaderState::Config &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(mesh_.get());
  fogShader_->createShader(_cfg.cfg(), "fog.volumetric.point");

  Shader *s = fogShader_->shader().get();
  posLoc_ = s->uniformLocation("lightPosition");
  radiusLoc_ = s->uniformLocation("lightRadius");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  exposureLoc_ = s->uniformLocation("fogExposure");
  radiusScaleLoc_ = s->uniformLocation("fogRadiusScale");
}

void VolumetricPointFog::addLight(
    const ref_ptr<PointLight> &l,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &radiusScale)
{
  FogLight fl;
  fl.l = l;
  fl.exposure = exposure;
  fl.radiusScale = radiusScale;
  lights_.push_back(fl);

  list<FogLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;
}

void VolumetricPointFog::removeLight(Light *l)
{
  lights_.erase( lightIterators_[l] );
}

void VolumetricPointFog::enable(RenderState *rs)
{
  State::enable(rs);

  for(list<FogLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    FogLight &fl = *it;
    fl.l->position()->enableUniform(posLoc_);
    fl.l->radius()->enableUniform(radiusLoc_);
    fl.l->diffuse()->enableUniform(diffuseLoc_);
    fl.exposure->enableUniform(exposureLoc_);
    fl.radiusScale->enableUniform(radiusScaleLoc_);

    mesh_->draw(1);
  }
}

////////////////
////////////////

VolumetricSpotFog::VolumetricSpotFog() : State()
{
  mesh_ = ref_ptr<MeshState>::cast( ConeClosed::getBaseCone() );
  joinStates(ref_ptr<State>::manage(new CullFaceState(GL_FRONT)));
  fogShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(fogShader_));
}

void VolumetricSpotFog::createShader(ShaderState::Config &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(mesh_.get());
  fogShader_->createShader(_cfg.cfg(), "fog.volumetric.spot");

  Shader *s = fogShader_->shader().get();
  posLoc_ = s->uniformLocation("lightPosition");
  dirLoc_ = s->uniformLocation("lightDirection");
  coneLoc_ = s->uniformLocation("lightConeAngles");
  radiusLoc_ = s->uniformLocation("lightRadius");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  exposureLoc_ = s->uniformLocation("fogExposure");
  radiusScaleLoc_ = s->uniformLocation("fogRadiusScale");
  coneScaleLoc_ = s->uniformLocation("fogConeScale");
  coneMatLoc_ = s->uniformLocation("modelMatrix");
}

void VolumetricSpotFog::addLight(
    const ref_ptr<SpotLight> &l,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &radiusScale,
    const ref_ptr<ShaderInput2f> &coneScale)
{
  FogLight fl;
  fl.l = l;
  fl.exposure = exposure;
  fl.radiusScale = radiusScale;
  fl.coneScale = coneScale;
  lights_.push_back(fl);

  list<FogLight>::iterator it = lights_.end();
  --it;
  lightIterators_[l.get()] = it;
}

void VolumetricSpotFog::removeLight(Light *l)
{
  lights_.erase( lightIterators_[l] );
}

void VolumetricSpotFog::enable(RenderState *rs)
{
  State::enable(rs);

  for(list<FogLight>::iterator it=lights_.begin(); it!=lights_.end(); ++it)
  {
    FogLight &fl = *it;
    fl.l->position()->enableUniform(posLoc_);
    fl.l->spotDirection()->enableUniform(dirLoc_);
    fl.l->coneAngle()->enableUniform(coneLoc_);
    fl.l->radius()->enableUniform(radiusLoc_);
    fl.l->diffuse()->enableUniform(diffuseLoc_);
    fl.l->coneMatrix()->enableUniform(coneMatLoc_);
    fl.exposure->enableUniform(exposureLoc_);
    fl.radiusScale->enableUniform(radiusScaleLoc_);
    fl.coneScale->enableUniform(coneScaleLoc_);
    mesh_->draw(1);
  }
}

////////////////
////////////////

VolumetricFog::VolumetricFog()
: State()
{
  spotFog_ = ref_ptr<VolumetricSpotFog>::manage(new VolumetricSpotFog);
  pointFog_ = ref_ptr<VolumetricPointFog>::manage(new VolumetricPointFog);

  fogStart_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogStart"));
  fogStart_->setUniformData(0.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogStart_));

  fogEnd_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogEnd"));
  fogEnd_->setUniformData(20.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogEnd_));

  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));

  fogSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(ref_ptr<State>::cast(fogSequence_));
}

const ref_ptr<ShaderInput1f>& VolumetricFog::fogStart() const
{
  return fogStart_;
}
const ref_ptr<ShaderInput1f>& VolumetricFog::fogEnd() const
{
  return fogEnd_;
}

void VolumetricFog::createShader(ShaderState::Config &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  spotFog_->createShader(_cfg.cfg());
  pointFog_->createShader(_cfg.cfg());
}

void VolumetricFog::set_gDepthTexture(const ref_ptr<Texture> &t)
{
  if(gDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gDepthTexture_));
  }
  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(t, "gDepthTexture"));
  joinStatesFront(ref_ptr<State>::cast(gDepthTexture_));
}
void VolumetricFog::set_tBuffer(
    const ref_ptr<Texture> &color,
    const ref_ptr<Texture> &depth)
{
  if(tDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(tDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(tColorTexture_));
  }
  if(!color.get()) { return; }
  shaderDefine("USE_TBUFFER", "TRUE");
  tDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depth, "tDepthTexture"));
  joinStatesFront(ref_ptr<State>::cast(tDepthTexture_));

  tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(color, "tColorTexture"));
  joinStatesFront(ref_ptr<State>::cast(tColorTexture_));
}

void VolumetricFog::addLight(
    const ref_ptr<SpotLight> &l,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &x,
    const ref_ptr<ShaderInput2f> &y)
{
  if(spotFog_->lights_.empty()) {
    fogSequence_->joinStates(ref_ptr<State>::cast(spotFog_));
  }
  spotFog_->addLight(l,exposure,x,y);
}
void VolumetricFog::addLight(
    const ref_ptr<PointLight> &l,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &x)
{
  if(pointFog_->lights_.empty()) {
    fogSequence_->joinStates(ref_ptr<State>::cast(pointFog_));
  }
  pointFog_->addLight(l,exposure,x);
}
void VolumetricFog::removeLight(SpotLight *l)
{
  spotFog_->removeLight(l);
  if(spotFog_->lights_.empty()) {
    fogSequence_->disjoinStates(ref_ptr<State>::cast(spotFog_));
  }
}
void VolumetricFog::removeLight(PointLight *l)
{
  pointFog_->removeLight(l);
  if(pointFog_->lights_.empty()) {
    fogSequence_->disjoinStates(ref_ptr<State>::cast(pointFog_));
  }
}

