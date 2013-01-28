/*
 * snow-particles.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#include <ogle/textures/texture-loader.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/depth-state.h>
#include "snow-particles.h"

SnowParticles::SnowParticles(GLuint numSnowFlakes)
: ParticleState(numSnowFlakes)
{
  // enable blending
  //joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  //joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_LIGHTEN)));
  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));
  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_useDepthWrite(GL_FALSE);
  joinStates(ref_ptr<State>::cast(depth));

  //// update inputs
  flakeMass_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("snowFlakeMass"));
  flakeMass_->setUniformData(Vec2f(0.75,0.25));
  setInput(ref_ptr<ShaderInput>::cast(flakeMass_));

  cloudPosition_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("cloudPosition"));
  cloudPosition_->setUniformData(Vec3f(0.0,4.0,0.0));
  setInput(ref_ptr<ShaderInput>::cast(cloudPosition_));

  cloudRadius_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("cloudRadius"));
  cloudRadius_->setUniformData(20.0);
  setInput(ref_ptr<ShaderInput>::cast(cloudRadius_));

  snowFlakeSize_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("snowFlakeSize"));
  snowFlakeSize_->setUniformData(Vec2f(0.1,0.05));
  setInput(ref_ptr<ShaderInput>::cast(snowFlakeSize_));

  maxNumParticleEmits_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("maxNumParticleEmits"));
  maxNumParticleEmits_->setUniformData(1);
  //setInput(ref_ptr<ShaderInput>::cast(maxNumParticleEmits_));

  //// draw inputs

  //// attributes
  posInput_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("pos"));
  posInput_->setVertexData(numSnowFlakes, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(posInput_));

  velocityInput_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("velocity"));
  velocityInput_->setVertexData(numSnowFlakes, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(velocityInput_));

  massInput_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("mass"));
  massInput_->setVertexData(numSnowFlakes, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(massInput_));

  ref_ptr<ShaderInput1f> sizeInput_ =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("size"));
  sizeInput_->setVertexData(numSnowFlakes, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(sizeInput_));

  // initially set lifetime to zero so that particles
  // get emitted in the first step
  GLfloat zeroLifetimeData[numSnowFlakes];
  memset(zeroLifetimeData, 0, sizeof(zeroLifetimeData));
  lifetimeInput_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lifetime"));
  lifetimeInput_->setVertexData(numSnowFlakes, (byte*)zeroLifetimeData);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(lifetimeInput_));

  // get a random seed for each particle
  GLuint initialSeedData[numSnowFlakes];
  srand(time(0));
  for(GLuint i=0u; i<numSnowFlakes; ++i) {
    initialSeedData[i] = rand();
  }
  ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::manage(new ShaderInput1ui("randomSeed"));
  randomSeed_->setVertexData(numSnowFlakes, (byte*)initialSeedData);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(randomSeed_));
}

void SnowParticles::createShader(ShaderConfig &shaderCfg)
{
  ParticleState::createShader(shaderCfg, "snow-particles.update", "snow-particles.draw");
}

void SnowParticles::set_depthTexture(const ref_ptr<Texture> &tex)
{
  if(depthTexture_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(depthTexture_));
  }
  depthTexture_ = ref_ptr<TextureState>::manage(new TextureState(tex));
  depthTexture_->set_name("depthTexture");
  joinStates(ref_ptr<State>::cast(depthTexture_));
}

void SnowParticles::set_snowFlakeTexture(const ref_ptr<Texture> &tex)
{
  if(snowFlakeTexture_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(snowFlakeTexture_));
  }
  snowFlakeTexture_ = ref_ptr<TextureState>::manage(new TextureState(tex));
  snowFlakeTexture_->set_name("snowFlakeTexture");
  joinStates(ref_ptr<State>::cast(snowFlakeTexture_));
}

const ref_ptr<ShaderInput2f>& SnowParticles::snowFlakeSize() const
{
  return snowFlakeSize_;
}
const ref_ptr<ShaderInput3f>& SnowParticles::cloudPosition() const
{
  return cloudPosition_;
}
const ref_ptr<ShaderInput2f>& SnowParticles::snowFlakeMass() const
{
  return flakeMass_;
}
const ref_ptr<ShaderInput1i>& SnowParticles::maxNumParticleEmits() const
{
  return maxNumParticleEmits_;
}
