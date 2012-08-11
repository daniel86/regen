/*
 * texture-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "texture-state.h"

TextureState::TextureState(ref_ptr<Texture> &texture)
: State(),
  texture_(texture),
  textureUnit_(0u)
{
}

void TextureState::set_transfer(ref_ptr<TexelTransfer> transfer)
{
  transfer_ = transfer;
}
ref_ptr<TexelTransfer> TextureState::transfer()
{
  return transfer_;
}

void TextureState::enable(RenderState *state)
{
  cout << "TextureState::enable" << endl;
  textureUnit_ = state->nextTextureUnit();
  state->pushTexture(textureUnit_, texture_.get());
  State::enable(state);
}

void TextureState::disable(RenderState *state)
{
  cout << "TextureState::disable" << endl;
  State::disable(state);
  state->popTexture(textureUnit_);
  state->releaseTextureUnit();
}

ref_ptr<Texture>& TextureState::texture()
{
  return texture_;
}

void TextureState::configureShader(ShaderConfiguration *shaderCfg)
{
  State::configureShader(shaderCfg);
  shaderCfg->addTexture(this);
  //if(!t->ignoreAlpha() && t->useAlpha()) {
  //  set_useAlpha(true);
  //}
}

/////////

TextureConstantUnitNode::TextureConstantUnitNode(
    ref_ptr<Texture> &texture,
    GLuint textureUnit)
: TextureState(texture)
{
  textureUnit_ = textureUnit;
}

void TextureConstantUnitNode::enable(RenderState *state)
{
  State::enable(state);
  state->pushTexture(textureUnit_, texture_.get());
}

GLuint TextureConstantUnitNode::textureUnit() const
{
  return textureUnit_;
}
