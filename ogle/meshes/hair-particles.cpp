/*
 * snow-particles.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#include <ogle/textures/texture-loader.h>
#include <ogle/states/blend-state.h>
#include "hair-particles.h"

HairParticles::HairParticles(
    const ref_ptr<ShaderInput> &hairRoots,
    const ref_ptr<ShaderInput> &normals)
: ParticleState(hairRoots->numVertices())
{
  //// update inputs
  hairLengthFuzzy_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("hairLengthFuzzy"));
  hairLengthFuzzy_->setUniformData(Vec2f(0.1,0.05));
  setInput(ref_ptr<ShaderInput>::cast(hairLengthFuzzy_));

  hairRigidity_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("hairRigidity"));
  hairRigidity_->setUniformData(2.5);
  setInput(ref_ptr<ShaderInput>::cast(hairRigidity_));

  hairMass_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("hairMass"));
  hairMass_->setUniformData(0.5);
  setInput(ref_ptr<ShaderInput>::cast(hairMass_));

  //// draw inputs

  //// attributes
  hairRootInput_ = hairRoots;
  setInput(hairRootInput_);
  normalInput_ = normals;
  setInput(normalInput_);

  hairLength_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("hairLength"));
  hairLength_->setVertexData(numVertices_, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(hairLength_));

  controlPointInput_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("controlPoint"));
  controlPointInput_->setVertexData(numVertices_, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(controlPointInput_));

  endPointInput_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("endPoint"));
  endPointInput_->setVertexData(numVertices_, NULL);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(endPointInput_));

  // initially set lifetime to zero so that particles
  // get emitted in the first step
  GLfloat zeroLifetimeData[numVertices_];
  memset(zeroLifetimeData, 0, sizeof(zeroLifetimeData));
  lifetimeInput_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("lifetime"));
  lifetimeInput_->setVertexData(numVertices_, (byte*)zeroLifetimeData);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(lifetimeInput_));

  // get a random seed for each particle
  GLuint initialSeedData[numVertices_];
  srand(time(0));
  for(GLuint i=0u; i<numVertices_; ++i) {
    initialSeedData[i] = rand();
  }
  ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::manage(new ShaderInput1ui("randomSeed"));
  randomSeed_->setVertexData(numVertices_, (byte*)initialSeedData);
  addParticleAttribute(ref_ptr<ShaderInput>::cast(randomSeed_));
}

void HairParticles::createShader(ShaderConfig &shaderCfg)
{
  ParticleState::createShader(shaderCfg, "hair-particles.update", "hair-particles.draw");
}

const ref_ptr<ShaderInput2f>& HairParticles::hairLength() const
{
  return hairLengthFuzzy_;
}
const ref_ptr<ShaderInput1f>& HairParticles::hairRigidity() const
{
  return hairRigidity_;
}
const ref_ptr<ShaderInput1f>& HairParticles::hairMass() const
{
  return hairMass_;
}
