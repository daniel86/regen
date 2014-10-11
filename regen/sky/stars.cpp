/*
 * stars.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "stars.h"

#include <regen/meshes/rectangle.h>
#include <regen/external/osghimmel/earth.h>
#include <regen/textures/texture-loader.h>
#include <regen/states/depth-state.h>

#include <regen/external/osghimmel/brightstars.h>
#include <regen/external/osghimmel/randommapgenerator.h>
#include <regen/external/osghimmel/coords.h>

using namespace regen;

StarsLayer::StarsLayer(const ref_ptr<Sky> &sky)
: SkyLayer(sky)
{
  state()->joinStates(ref_ptr<BlendState>::alloc(GL_SRC_ALPHA, GL_ONE));

  apparentMagnitude_ = ref_ptr<ShaderInput1f>::alloc("apparentMagnitude");
  apparentMagnitude_->setUniformData(defaultApparentMagnitude());
  state()->joinShaderInput(apparentMagnitude_);

  color_ = ref_ptr<ShaderInput3f>::alloc("color");
  color_->setUniformData(defaultColor());
  state()->joinShaderInput(color_);

  colorRatio_ = ref_ptr<ShaderInput1f>::alloc("colorRatio");
  colorRatio_->setUniformData(defaultColorRatio());
  state()->joinShaderInput(colorRatio_);

  glareIntensity_ = ref_ptr<ShaderInput1f>::alloc("glareIntensity");
  glareIntensity_->setUniformData(0.5);
  state()->joinShaderInput(glareIntensity_);

  glareScale_ = ref_ptr<ShaderInput1f>::alloc("glareScale");
  glareScale_->setUniformData(defaultGlareScale());
  state()->joinShaderInput(glareScale_);

  scintillation_ = ref_ptr<ShaderInput1f>::alloc("scintillation");
  scintillation_->setUniformData(defaultScintillation());
  state()->joinShaderInput(scintillation_);

  scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
  scattering_->setUniformData(defaultScattering());
  state()->joinShaderInput(scattering_);

  scale_ = ref_ptr<ShaderInput1f>::alloc("scale");
  scale_->setUniformData(1.0f);
  state()->joinShaderInput(scale_);


  noiseTexState_ = ref_ptr<TextureState>::alloc();
  updateNoiseTexture();
  state()->joinStates(noiseTexState_);

  shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.bright-stars");
  //state()->joinStates(shaderState_->shaderState());

  meshState_ = ref_ptr<Mesh>::alloc(GL_POINTS, VBO::USAGE_STATIC);
  pos_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_POS);
  col_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_COL0);
  //state()->joinStates(meshState_);
}


#define _rightascd(deg, min, sec) \
    (_decimal(deg, min, sec) * 15.0L)

#define _rightasc(deg, min, sec) \
    (_rad(_rightascd(deg, min, sec)))

void StarsLayer::set_brightStarsFile(const string &brightStars)
{
  osgHimmel::BrightStars bs(brightStars.c_str());
  const osgHimmel::BrightStars::s_BrightStar *stars = bs.stars();
  if(bs.numStars()==0) {
    REGEN_WARN("Unable to load bright stars catalog at " << brightStars << ".");
    return;
  }

  pos_->setVertexData(bs.numStars());
  col_->setVertexData(bs.numStars());

  for(unsigned int i = 0; i < bs.numStars(); ++i) {
    osgHimmel::t_equf equ;
    equ.right_ascension = _rightascd(stars[i].RA, 0, 0);
    equ.declination = stars[i].DE;

    pos_->setVertex(i, Vec4f(equ.toEuclidean(),i));
    col_->setVertex(i, Vec4f(
        stars[i].sRGB_R,
        stars[i].sRGB_G,
        stars[i].sRGB_B,
        stars[i].Vmag + 0.4 // the 0.4 accounts for magnitude decrease due to the earth's atmosphere
   ));
  }

  meshState_->begin(ShaderInputContainer::INTERLEAVED);
  meshState_->setInput(pos_);
  meshState_->setInput(col_);
  meshState_->end();
}

void StarsLayer::updateNoiseTexture()
{
  const int noiseN = 256;

  byte *noiseMap = new byte[noiseN];
  osgHimmel::RandomMapGenerator::generate1(noiseN, 1, noiseMap);

  noiseTex_ = ref_ptr<Texture1D>::alloc();
  noiseTex_->set_rectangleSize(noiseN,1);
  noiseTex_->set_format(GL_LUMINANCE);
  noiseTex_->set_internalFormat(GL_LUMINANCE8);
  noiseTex_->set_pixelType(GL_UNSIGNED_BYTE);
  noiseTex_->set_data(noiseMap);
  noiseTex_->begin(RenderState::get());
  noiseTex_->texImage();
  //noiseTex_->filter().push(GL_LINEAR);
  //noiseTex_->wrapping().push(GL_REPEAT);
  noiseTex_->end(RenderState::get());
  noiseTex_->set_data(NULL);

  delete []noiseMap;

  noiseTexState_->set_texture(noiseTex_);
  noiseTexState_->set_name("noiseTexture");
}


GLfloat StarsLayer::defaultApparentMagnitude()
{ return 7.0f; }

Vec3f StarsLayer::defaultColor()
{ return Vec3f(0.66, 0.78, 1.00); }

GLfloat StarsLayer::defaultColorRatio()
{ return 0.66f; }

GLfloat StarsLayer::defaultGlareScale()
{ return 0.04f; }

GLfloat StarsLayer::defaultScintillation()
{ return 20.0f; }

GLfloat StarsLayer::defaultScattering()
{ return 4.0f; }


void StarsLayer::set_apparentMagnitude(const GLfloat vMag)
{ apparentMagnitude_->setVertex(0, vMag); }

const ref_ptr<ShaderInput1f>& StarsLayer::apparentMagnitude() const
{ return apparentMagnitude_; }

void StarsLayer::set_color(const Vec3f color)
{ color_->setVertex(0, color); }

const ref_ptr<ShaderInput3f>& StarsLayer::StarsLayer::color() const
{ return color_; }

void StarsLayer::set_colorRatio(const GLfloat ratio)
{ colorRatio_->setVertex(0, ratio); }

const ref_ptr<ShaderInput1f>& StarsLayer::colorRatio() const
{ return colorRatio_; }

void StarsLayer::set_glareIntensity(const GLfloat intensity)
{ glareIntensity_->setVertex(0, intensity); }

const ref_ptr<ShaderInput1f>& StarsLayer::glareIntensity() const
{ return glareIntensity_; }

void StarsLayer::set_glareScale(const GLfloat scale)
{ glareScale_->setVertex(0, scale); }

const ref_ptr<ShaderInput1f>& StarsLayer::glareScale() const
{ return glareScale_; }

void StarsLayer::set_scintillation(const GLfloat scintillation)
{ scintillation_->setVertex(0, scintillation); }

const ref_ptr<ShaderInput1f>& StarsLayer::scintillation() const
{ return scintillation_; }

void StarsLayer::set_scattering(const GLfloat scattering)
{ scattering_->setVertex(0, scattering); }

const ref_ptr<ShaderInput1f>& StarsLayer::scattering() const
{ return scattering_; }

void StarsLayer::set_scale(const GLfloat scale)
{ scale_->setVertex(0, scale); }

const ref_ptr<ShaderInput1f>& StarsLayer::scale() const
{ return scale_; }


ref_ptr<Mesh> StarsLayer::getMeshState()
{ return meshState_; }

ref_ptr<HasShader> StarsLayer::getShaderState()
{ return shaderState_; }


void StarsLayer::updateSkyLayer(RenderState *rs, GLdouble dt)
{
}


