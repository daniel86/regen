/*
 * post-processing.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include "post-processing.h"

#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/rectangle.h>

ShadingPostProcessing::ShadingPostProcessing()
: State(), hasAO_(GL_FALSE)
{
  stateSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(ref_ptr<State>::cast(stateSequence_));

  updateAOState_ = ref_ptr<AmbientOcclusion>::manage(new AmbientOcclusion(0.5));

  ref_ptr<State> drawState = ref_ptr<State>::manage(new State);
  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  drawState->joinStates(ref_ptr<State>::cast(shader_));
  drawState->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
  stateSequence_->joinStates(drawState);
}
void ShadingPostProcessing::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  shader_->createShader(_cfg.cfg(), "shading.postProcessing");
  if(hasAO_) {
    updateAOState_->createResources(_cfg.cfg(), gNorWorldTexture_->texture());
    set_aoBuffer(updateAOState_->aoTexture());
  }
}
void ShadingPostProcessing::resize()
{
  updateAOState_->resize();
}

const ref_ptr<AmbientOcclusion>& ShadingPostProcessing::ambientOcclusionState() const
{
  return updateAOState_;
}

void ShadingPostProcessing::setUseAmbientOcclusion()
{
  if(!hasAO_) {
    stateSequence_->joinStatesFront(ref_ptr<State>::cast(updateAOState_));
    shaderDefine("USE_AMBIENT_OCCLUSION", "TRUE");
    hasAO_ = GL_TRUE;
  }
}

void ShadingPostProcessing::set_gBuffer(
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &norWorldTexture,
    const ref_ptr<Texture> &diffuseTexture)
{
  if(gDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(gDiffuseTexture_));
    disjoinStates(ref_ptr<State>::cast(gNorWorldTexture_));
  }

  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depthTexture));
  gDepthTexture_->set_name("gDepthTexture");
  joinStatesFront(ref_ptr<State>::cast(gDepthTexture_));

  gNorWorldTexture_ = ref_ptr<TextureState>::manage(new TextureState(norWorldTexture));
  gNorWorldTexture_->set_name("gNorWorldTexture");
  joinStatesFront(ref_ptr<State>::cast(gNorWorldTexture_));

  gDiffuseTexture_ = ref_ptr<TextureState>::manage(new TextureState(diffuseTexture));
  gDiffuseTexture_->set_name("gDiffuseTexture");
  joinStatesFront(ref_ptr<State>::cast(gDiffuseTexture_));
}

void ShadingPostProcessing::set_tBuffer(const ref_ptr<Texture> &t)
{
  shaderDefine("USE_TRANSPARENCY","TRUE");
  if(tColorTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(tColorTexture_));
  }
  tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  tColorTexture_->set_name("tColorTexture");
  joinStatesFront(ref_ptr<State>::cast(tColorTexture_));
}

void ShadingPostProcessing::set_aoBuffer(const ref_ptr<Texture> &t)
{
  shaderDefine("USE_AMBIENT_OCCLUSION","TRUE");
  if(aoTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(aoTexture_));
  }
  aoTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  aoTexture_->set_name("aoTexture");
  joinStatesFront(ref_ptr<State>::cast(aoTexture_));
}
