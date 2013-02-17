/*
 * fog.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <ogle/render-tree/shader-configurer.h>
#include <ogle/meshes/cone.h>
#include <ogle/animations/animation-manager.h>
#include "fog-state.h"

/////////////
/////////////

FogState::FogState()
: State()
{
  useConstFogColor_ = GL_FALSE;
  quad_ = Rectangle::getUnitQuad();

  accumulateShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  dirShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  spotShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  pointShader_ = ref_ptr<ShaderState>::manage(new ShaderState);

  shaderDefine("USE_FOG_DENSITY_TEXTURE", "FALSE");
  shaderDefine("USE_FOG_COLOR_TEXTURE", "FALSE");
  set_useSkyScattering(GL_TRUE);

  /////////////
  /////////////

  fogStart_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogStart"));
  fogStart_->setUniformData(0.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogStart_));

  fogEnd_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogEnd"));
  fogEnd_->setUniformData(20.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogEnd_));

  fogExposure_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogExposure"));
  fogExposure_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogExposure_));

  fogScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogScale"));
  fogScale_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogScale_));

  constFogDensity_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("constFogDensity"));
  constFogDensity_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(constFogDensity_));

  constFogColor_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("constFogColor"));
  constFogColor_->setUniformData(Vec3f(1.0));
  joinShaderInput(ref_ptr<ShaderInput>::cast(constFogColor_));

  /////////////
  /////////////

  sunDiffuseExposure_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("sunDiffuseExposure"));
  sunDiffuseExposure_->setUniformData(0.01);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sunDiffuseExposure_));

  sunScatteringExposure_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("sunScatteringExposure"));
  sunScatteringExposure_->setUniformData(0.6);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sunScatteringExposure_));

  sunScatteringWeight_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("sunScatteringWeight"));
  sunScatteringWeight_->setUniformData(0.1);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sunScatteringWeight_));

  sunScatteringDensity_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("sunScatteringDensity"));
  sunScatteringDensity_->setUniformData(4.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sunScatteringDensity_));

  sunScatteringDecay_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("sunScatteringDecay"));
  sunScatteringDecay_->setUniformData(0.85);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sunScatteringDecay_));

  sunScatteringSamples_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("sunScatteringSamples"));
  sunScatteringSamples_->setUniformData(14.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sunScatteringSamples_));

  /////////////
  /////////////

  lightDiffuseExposure_ = ref_ptr<ShaderInput1f>::manage(
      new ShaderInput1f("lightDiffuseExposure"));
  lightDiffuseExposure_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(lightDiffuseExposure_));

  /////////////
  /////////////

  densityScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("densityScale"));
  densityScale_->setUniformData(1.5);
  joinShaderInput(ref_ptr<ShaderInput>::cast(densityScale_));

  densityBias_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("densityBias"));
  densityBias_->setUniformData(0.5);
  joinShaderInput(ref_ptr<ShaderInput>::cast(densityBias_));

  emitterRadius_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("emitterRadius"));
  emitterRadius_->setUniformData(10.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(emitterRadius_));

  emitVelocity_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("emitVelocity"));
  emitVelocity_->setUniformData(2.2);
  joinShaderInput(ref_ptr<ShaderInput>::cast(emitVelocity_));

  emitRadius_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("emitRadius"));
  emitRadius_->setUniformData(Vec2f(5.0,0.75));
  joinShaderInput(ref_ptr<ShaderInput>::cast(emitRadius_));

  emitDensity_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("emitDensity"));
  emitDensity_->setUniformData(Vec2f(1.0,0.4));
  joinShaderInput(ref_ptr<ShaderInput>::cast(emitDensity_));

  emitLifetime_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("emitLifetime"));
  emitLifetime_->setUniformData(Vec2f(10.0,2.5));
  joinShaderInput(ref_ptr<ShaderInput>::cast(emitLifetime_));
}

void FogState::set_useSkyScattering(GLboolean v)
{
  shaderDefine("USE_SKY_SCATTERING", v?"TRUE":"FALSE");
}

void FogState::createBuffer(const Vec2ui &size)
{
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(size.x,size.y,GL_NONE));
  fogBuffer_ = ref_ptr<FBOState>::manage(new FBOState(fbo));

  if(!useConstFogColor_) {
    if(fogColorTexture_.get()!=NULL) {
      disjoinStates(ref_ptr<State>::cast(fogColorTexture_));
    }
    ref_ptr<Texture> tex = fbo->addTexture(1, GL_RGB, GL_RGB16F);
    fogColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(tex));
    fogColorTexture_->set_name("fogColorTexture");
    joinStates(ref_ptr<State>::cast(fogColorTexture_));
  }

  if(densityParticles_.get()) {
    if(fogDensityTexture_.get()!=NULL) {
      disjoinStates(ref_ptr<State>::cast(fogDensityTexture_));
    }
    ref_ptr<Texture> tex = fbo->addTexture(1, GL_RED, GL_R16F);
    fogDensityTexture_ = ref_ptr<TextureState>::manage(new TextureState(tex));
    fogDensityTexture_->set_name("fogDensityTexture");
    joinStates(ref_ptr<State>::cast(fogDensityTexture_));
  }
}
void FogState::resizeBuffer(const Vec2ui &size)
{
  fogBuffer_->resize(size.x, size.y);
}

void FogState::createShaders(ShaderConfig &cfg)
{
  {
    ShaderConfigurer configurer(cfg);
    configurer.addState(quad_.get());
    accumulateShader_->createShader(configurer.cfg(), "fog.accumulate");
  }

  if(densityParticles_.get()) {
    ShaderConfigurer configurer(cfg);
    configurer.addState(densityParticles_.get());
    configurer.addState(fogBuffer_.get());
    densityParticles_->createShader(configurer.cfg(),
        "fog.density.update", "fog.density.draw");
  }

  if(!dirLights_.empty()) {
    ShaderConfigurer configurer(cfg);
    configurer.addState(quad_.get());
    configurer.addState(fogBuffer_.get());
    dirShader_->createShader(configurer.cfg(), "fog.volumetric.sun");

    sunLightDirLoc_ =
        dirShader_->shader()->uniformLocation("lightDirection");
    sunLightDiffuseLoc_ =
        dirShader_->shader()->uniformLocation("lightColor");
  }

  if(!pointLights_.empty()) {
    ShaderConfigurer configurer(cfg);
    configurer.addState(quad_.get());
    configurer.addState(fogBuffer_.get());
    configurer.define("IS_SPOT_LIGHT", "FALSE");
    pointShader_->createShader(configurer.cfg(), "fog.volumetric.light");

    pointLightPosLoc_ =
        pointShader_->shader()->uniformLocation("lightPosition");
    pointLightDiffuseLoc_ =
        pointShader_->shader()->uniformLocation("lightColor");
    pointLightAttenuationLoc_ =
        pointShader_->shader()->uniformLocation("lightAttenuation");
  }

  if(!spotLights_.empty()) {
    ShaderConfigurer configurer(cfg);
    configurer.addState(fogBuffer_.get());
    configurer.define("IS_SPOT_LIGHT", "TRUE");
    spotShader_->createShader(configurer.cfg(), "fog.volumetric.light");

    spotLightPosLoc_ =
        spotShader_->shader()->uniformLocation("lightPosition");
    spotLightDirLoc_ =
        spotShader_->shader()->uniformLocation("lightDirection");
    spotLightDiffuseLoc_ =
        spotShader_->shader()->uniformLocation("lightColor");
    spotLightAttenuationLoc_ =
        spotShader_->shader()->uniformLocation("lightAttenuation");
    spotConeMatLoc_ =
        spotShader_->shader()->uniformLocation("modelMatrix");
    spotPosLoc_ =
        spotShader_->shader()->attributeLocation("pos");
  }
}

void FogState::enable(RenderState *rs)
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  State::enable(rs);

  fogBuffer_->enable(rs);
  /////////////
  /////////////

  GLenum nextTarget = GL_COLOR_ATTACHMENT0;

  if(!useConstFogColor_) {
    // update fog color texture
    glDrawBuffer(nextTarget); nextTarget+=1;
    glClearColor(0.0,0.0,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if(!dirLights_.empty()) {
      dirShader_->enable(rs);
      for(list< ref_ptr<DirectionalLight> >::iterator
          it=dirLights_.begin(); it!=dirLights_.end(); ++it)
      {
        ref_ptr<DirectionalLight> &l = *it;
        l->direction()->enableUniform(sunLightDirLoc_);
        l->diffuse()->enableUniform(sunLightDiffuseLoc_);
        quad_->draw(1);
      }
      dirShader_->disable(rs);
    }

    if(!pointLights_.empty()) {
      pointShader_->enable(rs);
      for(list< ref_ptr<PointLight> >::iterator
          it=pointLights_.begin(); it!=pointLights_.end(); ++it)
      {
        ref_ptr<PointLight> &l = *it;
        l->position()->enableUniform(pointLightPosLoc_);
        l->diffuse()->enableUniform(pointLightDiffuseLoc_);
        l->radius()->enableUniform(pointLightAttenuationLoc_);
        quad_->draw(1);
      }
      pointShader_->disable(rs);
    }

    if(!spotLights_.empty()) {
      spotShader_->enable(rs);
      for(list< ref_ptr<SpotLight> >::iterator
          it=spotLights_.begin(); it!=spotLights_.end(); ++it)
      {
        ref_ptr<SpotLight> &l = *it;
        l->position()->enableUniform(spotLightPosLoc_);
        l->spotDirection()->enableUniform(spotLightDirLoc_);
        l->diffuse()->enableUniform(spotLightDiffuseLoc_);
        l->radius()->enableUniform(spotLightAttenuationLoc_);

        const ref_ptr<ShaderInput> &pos = *l->coneMesh()->positions();
        l->coneMatrix()->modelMat()->enableUniform(spotConeMatLoc_);

        glBindBuffer(GL_ARRAY_BUFFER, pos->buffer());
        pos->enableAttribute(spotPosLoc_);

        l->coneMesh()->enable(rs);
        l->coneMesh()->disable(rs);
      }
      spotShader_->disable(rs);
    }
  }

  if(densityParticles_.get()){
    // update fog density texture
    glDrawBuffer(nextTarget); nextTarget+=1;
    glClear(GL_COLOR_BUFFER_BIT);
    densityParticles_->enable(rs);
    densityParticles_->disable(rs);
  }

  fogBuffer_->disable(rs);

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //glBlendFunc(GL_ONE, GL_ONE);
  //glDisable(GL_BLEND);

  // draw ontop of scene
  sceneFramebuffer_->bind();
  sceneFramebuffer_->set_viewport();
  glDrawBuffer(GL_COLOR_ATTACHMENT0 + colorTexture_->texture()->bufferIndex());

  accumulateShader_->enable(rs);
  quad_->draw(1);
}
void FogState::disable(RenderState *rs)
{
  accumulateShader_->disable(rs);
  State::disable(rs);
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
}

/////////////
/////////////

void FogState::set_sceneFramebuffer(const ref_ptr<FrameBufferObject> &fbo)
{
  sceneFramebuffer_ = fbo;
}
void FogState::set_colorTexture(const ref_ptr<Texture> &tex)
{
  if(colorTexture_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(colorTexture_));
  }
  colorTexture_ = ref_ptr<TextureState>::manage(new TextureState(tex));
  colorTexture_->set_name("colorTexture");
  joinStates(ref_ptr<State>::cast(colorTexture_));
}
void FogState::set_depthTexture(const ref_ptr<Texture> &tex)
{
  if(depthTexture_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(depthTexture_));
  }
  depthTexture_ = ref_ptr<TextureState>::manage(new TextureState(tex));
  depthTexture_->set_name("depthTexture");
  joinStates(ref_ptr<State>::cast(depthTexture_));
}

const ref_ptr<ShaderInput1f>& FogState::fogStart() const
{
  return fogStart_;
}
const ref_ptr<ShaderInput1f>& FogState::fogEnd() const
{
  return fogEnd_;
}
const ref_ptr<ShaderInput1f>& FogState::fogExposure() const
{
  return fogExposure_;
}
const ref_ptr<ShaderInput1f>& FogState::fogScale() const
{
  return fogScale_;
}

const ref_ptr<ShaderInput3f>& FogState::constFogColor() const
{
  return constFogColor_;
}
const ref_ptr<ShaderInput1f>& FogState::constFogDensity() const
{
  return constFogDensity_;
}

/////////////
/////////////

void FogState::addLight(const ref_ptr<DirectionalLight> &l)
{
  useConstFogColor_ = GL_FALSE;
  shaderDefine("USE_FOG_COLOR_TEXTURE", "TRUE");
  dirLights_.push_back(l);
}
void FogState::addLight(const ref_ptr<PointLight> &l)
{
  shaderDefine("USE_FOG_COLOR_TEXTURE", "TRUE");
  useConstFogColor_ = GL_FALSE;
  pointLights_.push_back(l);
}
void FogState::addLight(const ref_ptr<SpotLight> &l)
{
  shaderDefine("USE_FOG_COLOR_TEXTURE", "TRUE");
  useConstFogColor_ = GL_FALSE;
  spotLights_.push_back(l);

  OpenedCone::Config coneCfg;
  coneCfg.height = 1000.0;
  coneCfg.isNormalRequired = GL_FALSE;
  coneCfg.levelOfDetail = 4;
  l->createConeMesh(coneCfg);
}

const ref_ptr<ShaderInput1f>& FogState::sunDiffuseExposure() const
{
  return sunDiffuseExposure_;
}
const ref_ptr<ShaderInput1f>& FogState::sunScatteringExposure() const
{
  return sunScatteringExposure_;
}
const ref_ptr<ShaderInput1f>& FogState::sunScatteringDecay() const
{
  return sunScatteringDecay_;
}
const ref_ptr<ShaderInput1f>& FogState::sunScatteringWeight() const
{
  return sunScatteringWeight_;
}
const ref_ptr<ShaderInput1f>& FogState::sunScatteringDensity() const
{
  return sunScatteringDensity_;
}
const ref_ptr<ShaderInput1f>& FogState::sunScatteringSamples() const
{
  return sunScatteringSamples_;
}

const ref_ptr<ShaderInput1f>& FogState::lightDiffuseExposure() const
{
  return lightDiffuseExposure_;
}

/////////////
/////////////

void FogState::setDensityParticles(GLuint numParticles)
{
  densityParticles_ = ref_ptr<ParticleState>::manage(
      new ParticleState(numParticles));
  densityParticles_->dampingFactor()->setVertex1f(0, 0.01);

  //// attributes
  const char* particleAttributes1f[] = {"radius", "density"};
  for(GLuint i=0; i<sizeof(particleAttributes1f)/sizeof(char*); ++i) {
    ref_ptr<ShaderInput1f> in = ref_ptr<ShaderInput1f>::manage(
        new ShaderInput1f(particleAttributes1f[i]));
    in->setVertexData(numParticles, NULL);
    densityParticles_->addParticleAttribute(ref_ptr<ShaderInput>::cast(in));
  }
  const char* particleAttributes3f[] = {"pos", "velocity"};
  for(GLuint i=0; i<sizeof(particleAttributes3f)/sizeof(char*); ++i) {
    ref_ptr<ShaderInput3f> in = ref_ptr<ShaderInput3f>::manage(
        new ShaderInput3f(particleAttributes3f[i]));
    in->setVertexData(numParticles, NULL);
    densityParticles_->addParticleAttribute(ref_ptr<ShaderInput>::cast(in));
  }
  densityParticles_->createBuffer();

  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(densityParticles_));

  shaderDefine("USE_FOG_DENSITY_TEXTURE", "TRUE");
}

const ref_ptr<ParticleState>& FogState::densityParticles() const
{
  return densityParticles_;
}
const ref_ptr<ShaderInput1f>& FogState::densityScale() const
{
  return densityScale_;
}
const ref_ptr<ShaderInput1f>& FogState::intensityBias() const
{
  return densityBias_;
}
const ref_ptr<ShaderInput1f>& FogState::emitterRadius() const
{
  return emitterRadius_;
}
const ref_ptr<ShaderInput1f>& FogState::emitVelocity() const
{
  return emitVelocity_;
}
const ref_ptr<ShaderInput2f>& FogState::emitRadius() const
{
  return emitRadius_;
}
const ref_ptr<ShaderInput2f>& FogState::emitDensity() const
{
  return emitDensity_;
}
const ref_ptr<ShaderInput2f>& FogState::emitLifetime() const
{
  return emitLifetime_;
}
