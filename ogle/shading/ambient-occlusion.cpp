/*
 * ambient-occlusion.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include "ambient-occlusion.h"

#include <ogle/textures/texture-loader.h>
#include <ogle/states/shader-configurer.h>

AmbientOcclusion::AmbientOcclusion(GLfloat sizeScale)
: State(), sizeScale_(sizeScale)
{
  blurSigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  blurSigma_->setUniformData(2.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(blurSigma_));

  blurNumPixels_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("numBlurPixels"));
  blurNumPixels_->setUniformData(4.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(blurNumPixels_));

  aoSamplingRadius_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoSamplingRadius"));
  aoSamplingRadius_->setUniformData(30.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoSamplingRadius_));

  aoBias_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoBias"));
  aoBias_->setUniformData(0.05);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoBias_));

  aoAttenuation_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("aoAttenuation"));
  aoAttenuation_->setUniformData( Vec2f(0.5,1.0) );
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoAttenuation_));

  ref_ptr<Texture> noise = TextureLoader::load("res/textures/random_normals.png");
  joinStates(ref_ptr<State>::manage(new TextureState(noise, "aoNoiseTexture")));
}

void AmbientOcclusion::createFilter(const ref_ptr<Texture> &input)
{
  if(filter_.get()) {
    disjoinStates(ref_ptr<State>::cast(filter_));
  }
  filter_ = ref_ptr<FilterSequence>::manage(new FilterSequence(input, GL_FALSE));
  filter_->setClearColor(Vec4f(0.0));
  filter_->set_format(GL_RGBA);
  filter_->set_internalFormat(GL_INTENSITY);
  filter_->set_pixelType(GL_BYTE);
  filter_->addFilter(ref_ptr<Filter>::manage(new Filter("shading.ssao", sizeScale_)));
  filter_->addFilter(ref_ptr<Filter>::manage(new Filter("blur.horizontal")));
  filter_->addFilter(ref_ptr<Filter>::manage(new Filter("blur.vertical")));
  joinStates(ref_ptr<State>::cast(filter_));
}

void AmbientOcclusion::createShader(ShaderConfig &cfg)
{
  if(filter_.get()) {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(this);
    filter_->createShader(_cfg.cfg());
  }
}

void AmbientOcclusion::resize()
{
  filter_->resize();
}

const ref_ptr<Texture>& AmbientOcclusion::aoTexture() const
{
  return filter_->output();
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::aoSamplingRadius() const
{
  return aoSamplingRadius_;
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::aoBias() const
{
  return aoBias_;
}
const ref_ptr<ShaderInput2f>& AmbientOcclusion::aoAttenuation() const
{
  return aoAttenuation_;
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::blurSigma() const
{
  return blurSigma_;
}
const ref_ptr<ShaderInput1f>& AmbientOcclusion::blurNumPixels() const
{
  return blurNumPixels_;
}
