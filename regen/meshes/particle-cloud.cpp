/*
 * precipitation-particles.cpp
 *
 *  Created on: 30.01.2013
 *      Author: daniel
 */

#include <regen/textures/texture-loader.h>

#include "particle-cloud.h"
using namespace regen;

ParticleCloud::ParticleCloud(GLuint numParticles, BlendMode blendMode)
: Particles(numParticles, blendMode)
{
  //// update inputs
  particleMass_ = ref_ptr<ShaderInput2f>::alloc("particleMass");
  particleMass_->setUniformData(Vec2f(0.75,0.25));
  setInput(particleMass_);

  particleSize_ = ref_ptr<ShaderInput2f>::alloc("particleSize");
  particleSize_->setUniformData(Vec2f(0.1,0.05));
  setInput(particleSize_);

  cloudPosition_ = ref_ptr<ShaderInput3f>::alloc("cloudPosition");
  cloudPosition_->setUniformData(Vec3f(0.0,4.0,0.0));
  setInput(cloudPosition_);

  cloudRadius_ = ref_ptr<ShaderInput1f>::alloc("cloudRadius");
  cloudRadius_->setUniformData(20.0);
  setInput(cloudRadius_);

  surfaceHeight_ = ref_ptr<ShaderInput1f>::alloc("surfaceHeight");
  surfaceHeight_->setUniformData(-2.0f);
  setInput(surfaceHeight_);

  //// attributes
  posInput_ = ref_ptr<ShaderInput3f>::alloc("pos");
  posInput_->setVertexData(numParticles, NULL);
  addParticleAttribute(posInput_);

  velocityInput_ = ref_ptr<ShaderInput3f>::alloc("velocity");
  velocityInput_->setVertexData(numParticles, NULL);
  addParticleAttribute(velocityInput_);

  massInput_ = ref_ptr<ShaderInput1f>::alloc("mass");
  massInput_->setVertexData(numParticles, NULL);
  addParticleAttribute(massInput_);

  sizeInput_ = ref_ptr<ShaderInput1f>::alloc("size");
  sizeInput_->setVertexData(numParticles, NULL);
  addParticleAttribute(sizeInput_);

  set_cloudPositionMode(CAMERA_RELATIVE);
}

void ParticleCloud::createShader(StateConfig &shaderCfg, const string &drawShader)
{
  Particles::createShader(shaderCfg, "regen.particles.cloud-particles.update", drawShader);
}

void ParticleCloud::set_particleTexture(const ref_ptr<Texture> &tex)
{
  if(particleTexture_.get()!=NULL) {
    disjoinStates(particleTexture_);
  }
  particleTexture_ = ref_ptr<TextureState>::alloc(tex,"particleTexture");
  joinStatesFront(particleTexture_);
}

void ParticleCloud::set_cloudPositionMode(ParticleCloud::PositionMode v)
{
  cloudPositionMode_ = v;
  switch(v) {
  case ABSOLUTE:
    shaderDefine("CLOUD_POSITION_MODE", "ABSOLUTE");
    break;
  case CAMERA_RELATIVE:
    shaderDefine("CLOUD_POSITION_MODE", "CAMERA_RELATIVE");
    break;
  }
}
ParticleCloud::PositionMode ParticleCloud::cloudPositionMode() const
{ return cloudPositionMode_; }

const ref_ptr<ShaderInput2f>& ParticleCloud::particleSize() const
{ return particleSize_; }
const ref_ptr<ShaderInput2f>& ParticleCloud::particleMass() const
{ return particleMass_; }

const ref_ptr<ShaderInput3f>& ParticleCloud::cloudPosition() const
{ return cloudPosition_; }
const ref_ptr<ShaderInput1f>& ParticleCloud::cloudRadius() const
{ return cloudRadius_; }
const ref_ptr<ShaderInput1f>& ParticleCloud::surfaceHeight() const
{ return surfaceHeight_; }

////////////
////////////

ParticleSnow::ParticleSnow(GLuint numSnowFlakes, BlendMode blendMode)
: ParticleCloud(numSnowFlakes, blendMode)
{
  particleMass_->setVertex(0, Vec2f(0.75,0.25));
  particleSize_->setVertex(0, Vec2f(0.1,0.05));
  cloudPosition_->setVertex(0, Vec3f(0.0,4.0,0.0));
  cloudRadius_->setVertex(0, 20.0);
}

void ParticleSnow::createShader(StateConfig &shaderCfg)
{
  ParticleCloud::createShader(shaderCfg, "regen.particles.snow.draw");
}

////////////
////////////

ParticleRain::ParticleRain(GLuint numRainDrops, BlendMode blendMode)
: ParticleCloud(numRainDrops, blendMode)
{
  particleMass_->setVertex(0, Vec2f(4.0,3.0));
  particleSize_->setVertex(0, Vec2f(1.0,0.1));
  cloudPosition_->setVertex(0, Vec3f(0.0,4.0,0.0));
  cloudRadius_->setVertex(0, 20.0);
  dampingFactor_->setUniformData(1.0);
  noiseFactor_->setUniformData(0.5);

  //// draw inputs
  streakSize_ = ref_ptr<ShaderInput2f>::alloc("streakSize");
  streakSize_->setUniformData(Vec2f(0.02,2.0));
  setInput(streakSize_);
}

/*
void RainParticles::loadRainTextureArray(
    const string &textureDirectory,
    const string &textureNamePattern)
{
  shaderDefine("USE_PARTICLE_SAMPLER2D", "FALSE");
  shaderDefine("USE_PARTICLE_ARRAY_SAMPLER2D", "FALSE");
  if(rainTexture_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(rainTexture_));
  }

  GLenum mipmapFlag=GL_NONE;
  GLenum format=GL_NONE;
  ref_ptr<Texture2DArray> tex = TextureLoader::loadArray(
      textureDirectory, textureNamePattern, mipmapFlag, format);

  rainTexture_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(tex)));
  rainTexture_->set_name("rainTexture");
  joinStates(ref_ptr<State>::cast(rainTexture_));

  shaderDefine("USE_PARTICLE_ARRAY_SAMPLER2D", "TRUE");
}
*/
void ParticleRain::loadIntensityTexture(const string &texturePath)
{
  shaderDefine("USE_PARTICLE_SAMPLER2D", "FALSE");
  shaderDefine("USE_PARTICLE_ARRAY_SAMPLER2D", "FALSE");
  if(particleTexture_.get()!=NULL) {
    disjoinStates(particleTexture_);
  }

  GLenum mipmapFlag=GL_NONE;
  GLenum format=GL_NONE;
  ref_ptr<Texture> tex = textures::load(texturePath, mipmapFlag, format);

  particleTexture_ = ref_ptr<TextureState>::alloc(tex, "particleTexture");
  joinStatesFront(particleTexture_);

  shaderDefine("USE_PARTICLE_SAMPLER2D", "TRUE");
}

void ParticleRain::createShader(StateConfig &shaderCfg)
{
  ParticleCloud::createShader(shaderCfg, "regen.particles.rain.draw");
}

const ref_ptr<ShaderInput2f>& ParticleRain::streakSize() const
{ return streakSize_; }
