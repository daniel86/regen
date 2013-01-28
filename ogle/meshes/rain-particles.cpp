/*
 * rain-particles.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#include <ogle/textures/texture-loader.h>
#include <ogle/states/blend-state.h>
#include "rain-particles.h"

// TODO: use textures from:
// http://www1.cs.columbia.edu/CAVE/projects/rain_ren/rain_ren.php
// http://developer.download.nvidia.com/SDK/10/direct3d/samples.html#rain

RainParticles::RainParticles(GLuint numRainDrops)
: ParticleState(numRainDrops)
{
  //// update inputs
  dropMass_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("rainDropMass"));
  dropMass_->setUniformData(Vec2f(0.75,0.25));
  setInput(ref_ptr<ShaderInput>::cast(dropMass_));

  cloudPosition_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("rainCloudPosition"));
  cloudPosition_->setUniformData(Vec3f(0.0,10.0,0.0));
  setInput(ref_ptr<ShaderInput>::cast(cloudPosition_));

  cloudRadius_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("rainCloudRadius"));
  cloudRadius_->setUniformData(20.0);
  setInput(ref_ptr<ShaderInput>::cast(cloudRadius_));

  maxNumParticleEmits_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("maxNumParticleEmits"));
  maxNumParticleEmits_->setUniformData(1);
  //setInput(ref_ptr<ShaderInput>::cast(maxNumParticleEmits_));

  //// draw inputs
  strandSize_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("strandSize"));
  strandSize_->setUniformData(Vec2f(0.01,0.015));
  setInput(ref_ptr<ShaderInput>::cast(strandSize_));

  //// attributes
  posInput_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("pos"));
  posInput_->setVertexData(numRainDrops, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(posInput_));

  velocityInput_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("velocity"));
  velocityInput_->setVertexData(numRainDrops, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(velocityInput_));

  massInput_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("mass"));
  massInput_->setVertexData(numRainDrops, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(massInput_));

  // initially set lifetime to zero so that particles
  // get emitted in the first step
  GLfloat zeroLifetimeData[numRainDrops];
  memset(zeroLifetimeData, 0, sizeof(zeroLifetimeData));
  lifetimeInput_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lifetime"));
  lifetimeInput_->setVertexData(numRainDrops, (byte*)zeroLifetimeData);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(lifetimeInput_));

  // get a random seed for each particle
  GLuint initialSeedData[numRainDrops];
  srand(time(0));
  for(GLuint i=0u; i<numRainDrops; ++i) {
    initialSeedData[i] = rand();
  }
  ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::manage(new ShaderInput1ui("randomSeed"));
  randomSeed_->setVertexData(numRainDrops, (byte*)initialSeedData);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(randomSeed_));
}

void RainParticles::createShader(ShaderConfig &shaderCfg)
{
  ParticleState::createShader(shaderCfg, "rain-particles.update", "rain-particles.draw");
}

const ref_ptr<ShaderInput2f>& RainParticles::strandSize() const
{
  return strandSize_;
}
const ref_ptr<ShaderInput3f>& RainParticles::cloudPosition() const
{
  return cloudPosition_;
}
const ref_ptr<ShaderInput2f>& RainParticles::dropMass() const
{
  return dropMass_;
}
const ref_ptr<ShaderInput1i>& RainParticles::maxNumParticleEmits() const
{
  return maxNumParticleEmits_;
}
