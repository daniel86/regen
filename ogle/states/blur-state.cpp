/*
 * blur-state.cpp
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#include <ogle/utility/string-util.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/states/shader-configurer.h>

#include "blur-state.h"

BlurState::BlurState(
    const ref_ptr<Texture> &input,
    const Vec2ui &textureSize)
: State(), input_(input)
{
  { // create the render target
    ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
        new FrameBufferObject(textureSize.x, textureSize.y, GL_NONE));
    framebuffer_ = ref_ptr<FBOState>::manage(new FBOState(fbo));
    joinStates(ref_ptr<State>::cast(framebuffer_));

    // handle some special texture formats
    switch(input->targetType()) {
    case GL_TEXTURE_CUBE_MAP:
      createCubeMapTextures();
      break;
    case GL_TEXTURE_2D_ARRAY:
      create2DArrayTextures();
      break;
    default:
      create2DTextures();
      break;
    }
  }

  sigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  sigma_->setUniformData(4.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sigma_));

  numPixels_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("numBlurPixels"));
  numPixels_->setUniformData(4.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(numPixels_));

  ref_ptr<State> blurSequence = ref_ptr<State>::manage(new StateSequence);
  joinStates(blurSequence);

  { // downsample input texture to blur buffer size -> GL_COLOR_ATTACHMENT0
    downsampleShader_ = ref_ptr<ShaderState>::manage(new ShaderState);

    ref_ptr<DrawBufferState> drawBuffer = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
    drawBuffer->colorBuffers.push_back(GL_COLOR_ATTACHMENT0);
    downsampleShader_->joinStates(ref_ptr<State>::cast(drawBuffer));

    ref_ptr<TextureState> texState =
        ref_ptr<TextureState>::manage(new TextureState(input_));
    texState->set_name("inputTexture");
    downsampleShader_->joinStates(ref_ptr<State>::cast(texState));

    downsampleShader_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
    blurSequence->joinStates(ref_ptr<State>::cast(downsampleShader_));
  }
  { // calculate blur in horizontal direction -> GL_COLOR_ATTACHMENT1
    blurHorizontalShader_ = ref_ptr<ShaderState>::manage(new ShaderState);

    ref_ptr<DrawBufferState> drawBuffer = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
    drawBuffer->colorBuffers.push_back(GL_COLOR_ATTACHMENT1);
    blurHorizontalShader_->joinStates(ref_ptr<State>::cast(drawBuffer));

    ref_ptr<TextureState> blurTexState =
        ref_ptr<TextureState>::manage(new TextureState(blurTexture0_));
    blurTexState->set_name("inputTexture");
    blurHorizontalShader_->joinStates(ref_ptr<State>::cast(blurTexState));

    blurHorizontalShader_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
    blurSequence->joinStates(ref_ptr<State>::cast(blurHorizontalShader_));
  }
  { // calculate blur in vertical direction -> GL_COLOR_ATTACHMENT0
    blurVerticalShader_ = ref_ptr<ShaderState>::manage(new ShaderState);

    ref_ptr<DrawBufferState> drawBuffer = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
    drawBuffer->colorBuffers.push_back(GL_COLOR_ATTACHMENT0);
    blurVerticalShader_->joinStates(ref_ptr<State>::cast(drawBuffer));

    ref_ptr<TextureState> blurTexState =
        ref_ptr<TextureState>::manage(new TextureState(blurTexture1_));
    blurTexState->set_name("inputTexture");
    blurVerticalShader_->joinStates(ref_ptr<State>::cast(blurTexState));

    blurVerticalShader_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
    blurSequence->joinStates(ref_ptr<State>::cast(blurVerticalShader_));
  }

  // use layered geometry shader for 3d textures
  if(dynamic_cast<TextureCube*>(input_.get()))
  {
    shaderDefine("HAS_GEOMETRY_SHADER", "TRUE");
    shaderDefine("IS_CUBE_TEXTURE", "TRUE");
  }
  else if(dynamic_cast<Texture3D*>(input_.get()))
  {
    Texture3D *tex3D = (Texture3D*)input_.get();
    shaderDefine("HAS_GEOMETRY_SHADER", "TRUE");
    shaderDefine("NUM_TEXTURE_LAYERS", FORMAT_STRING(tex3D->depth()));
    shaderDefine("IS_ARRAY_TEXTURE", "FALSE");
  }
  else
  {
    shaderDefine("IS_2D_TEXTURE", "TRUE");
  }
}

void BlurState::createCubeMapTextures()
{
  ref_ptr<FrameBufferObject> fbo = framebuffer_->fbo();

  blurTexture0_ = ref_ptr<Texture>::manage(new TextureCube);
  blurTexture0_->set_size(fbo->width(), fbo->height());
  blurTexture0_->set_format(input_->format());
  blurTexture0_->set_internalFormat(input_->internalFormat());
  blurTexture0_->set_pixelType(input_->pixelType());
  blurTexture0_->bind();
  blurTexture0_->set_wrapping(GL_CLAMP_TO_EDGE);
  blurTexture0_->set_filter(GL_LINEAR, GL_LINEAR);
  blurTexture0_->texImage();
  fbo->addColorAttachment(*blurTexture0_.get());

  blurTexture1_ = ref_ptr<Texture>::manage(new TextureCube);
  blurTexture1_->set_size(fbo->width(), fbo->height());
  blurTexture1_->set_format(input_->format());
  blurTexture1_->set_internalFormat(input_->internalFormat());
  blurTexture1_->set_pixelType(input_->pixelType());
  blurTexture1_->bind();
  blurTexture1_->set_wrapping(GL_CLAMP_TO_EDGE);
  blurTexture1_->set_filter(GL_LINEAR, GL_LINEAR);
  blurTexture1_->texImage();
  fbo->addColorAttachment(*blurTexture1_.get());
}
void BlurState::create2DArrayTextures()
{
  // XXX
  ref_ptr<FrameBufferObject> fbo = framebuffer_->fbo();
  blurTexture0_ = fbo->addTexture(1, input_->format(), input_->internalFormat());
  blurTexture1_ = fbo->addTexture(1, input_->format(), input_->internalFormat());
}
void BlurState::create2DTextures()
{
  ref_ptr<FrameBufferObject> fbo = framebuffer_->fbo();
  blurTexture0_ = fbo->addTexture(1, input_->format(), input_->internalFormat());
  blurTexture1_ = fbo->addTexture(1, input_->format(), input_->internalFormat());
}

void BlurState::createShader(ShaderConfig &cfg)
{
  {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(downsampleShader_.get());
    downsampleShader_->createShader(_cfg.cfg(), "blur.downsample");
  }
  {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(blurHorizontalShader_.get());
    _cfg.define("BLUR_HORIZONTAL", "TRUE");
    blurHorizontalShader_->createShader(_cfg.cfg(), "blur");
  }
  {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(blurVerticalShader_.get());
    _cfg.define("BLUR_HORIZONTAL", "FALSE");
    blurVerticalShader_->createShader(_cfg.cfg(), "blur");
  }
}

const ref_ptr<Texture>& BlurState::blurTexture() const
{
  return blurTexture0_;
}
const ref_ptr<FBOState>& BlurState::framebuffer() const
{
  return framebuffer_;
}

void BlurState::set_sigma(GLfloat sigma)
{
  sigma_->setVertex1f(0,sigma);
}
const ref_ptr<ShaderInput1f>& BlurState::sigma() const
{
  return sigma_;
}

void BlurState::set_numPixels(GLfloat numPixels)
{
  numPixels_->setVertex1f(0,numPixels);
}
const ref_ptr<ShaderInput1f>& BlurState::numPixels() const
{
  return numPixels_;
}
