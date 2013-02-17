/*
 * volumetric-fog.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include "volumetric-fog.h"

#include <ogle/render-tree/shader-configurer.h>
#include <ogle/meshes/attribute-less-mesh.h>

////////////////
////////////////

VolumetricPointFog::VolumetricPointFog(const ref_ptr<MeshState> &mesh)
: State(),
  mesh_(mesh)
{
  fogShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(fogShader_));
}

void VolumetricPointFog::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  fogShader_->createShader(cfg, "volumetric.point");

  Shader *s = fogShader_->shader().get();
  posLoc_ = s->uniformLocation("lightPosition");
  radiusLoc_ = s->uniformLocation("lightRadius");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  exposureLoc_ = s->uniformLocation("fogExposure");
}

void VolumetricPointFog::addLight(
    const ref_ptr<PointLight> &l,
    const ref_ptr<ShaderInput1f> &exposure)
{
  FogLight fl;
  fl.l = l;
  fl.exposure = exposure;
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

    mesh_->draw(1);
  }
}

////////////////
////////////////

VolumetricSpotFog::VolumetricSpotFog(const ref_ptr<MeshState> &mesh)
: State(),
  mesh_(mesh)
{
  fogShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(fogShader_));
}

void VolumetricSpotFog::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  fogShader_->createShader(cfg, "volumetric.spot");

  Shader *s = fogShader_->shader().get();
  posLoc_ = s->uniformLocation("lightPosition");
  dirLoc_ = s->uniformLocation("lightDirection");
  coneLoc_ = s->uniformLocation("lightConeAngles");
  radiusLoc_ = s->uniformLocation("lightRadius");
  diffuseLoc_ = s->uniformLocation("lightDiffuse");
  exposureLoc_ = s->uniformLocation("fogExposure");
}

void VolumetricSpotFog::addLight(
    const ref_ptr<SpotLight> &l,
    const ref_ptr<ShaderInput1f> &exposure)
{
  FogLight fl;
  fl.l = l;
  fl.exposure = exposure;
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
    fl.exposure->enableUniform(exposureLoc_);

    mesh_->draw(1);
  }
}

////////////////
////////////////

VolumetricFog::VolumetricFog()
: State()
{
  mesh_ = ref_ptr<MeshState>::manage(new AttributeLessMesh(1));

  spotFog_ = ref_ptr<VolumetricSpotFog>::manage(new VolumetricSpotFog(mesh_));
  pointFog_ = ref_ptr<VolumetricPointFog>::manage(new VolumetricPointFog(mesh_));

  fogSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(ref_ptr<State>::cast(fogSequence_));
}

void VolumetricFog::createShader(ShaderConfig &cfg)
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
  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  gDepthTexture_->set_name("gDepthTexture");
  joinStates(ref_ptr<State>::cast(gDepthTexture_));
}
void VolumetricFog::set_tBuffer(
    const ref_ptr<Texture> &color,
    const ref_ptr<Texture> &depth)
{
  shaderDefine("USE_TBUFFER", "TRUE");
  if(tDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(tDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(tColorTexture_));
  }
  tDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depth));
  tDepthTexture_->set_name("tDepthTexture");
  joinStates(ref_ptr<State>::cast(tDepthTexture_));

  tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(color));
  tColorTexture_->set_name("tColorTexture");
  joinStates(ref_ptr<State>::cast(tColorTexture_));
}

void VolumetricFog::addLight(
    const ref_ptr<SpotLight> &l,
    const ref_ptr<ShaderInput1f> &exposure)
{
  if(spotFog_->lights_.empty()) {
    fogSequence_->joinStates(ref_ptr<State>::cast(spotFog_));
  }
  spotFog_->addLight(l,exposure);
}
void VolumetricFog::addLight(
    const ref_ptr<PointLight> &l,
    const ref_ptr<ShaderInput1f> &exposure)
{
  if(pointFog_->lights_.empty()) {
    fogSequence_->joinStates(ref_ptr<State>::cast(pointFog_));
  }
  pointFog_->addLight(l,exposure);
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

