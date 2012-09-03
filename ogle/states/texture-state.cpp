/*
 * texture-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "texture-state.h"
#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>

TextureState::TextureState(ref_ptr<Texture> texture)
: State(),
  texture_(texture),
  textureUnit_(0u)
{
}

string TextureState::name()
{
  return FORMAT_STRING("TextureState(" << texture_->name() << ")");
}

void TextureState::set_transfer(ref_ptr<TexelTransfer> transfer)
{
  if(transfer_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(transfer));
  }
  transfer_ = transfer;
  if(transfer_.get()!=NULL) {
    joinStates(ref_ptr<State>::cast(transfer));
  }
}
ref_ptr<TexelTransfer> TextureState::transfer()
{
  return transfer_;
}

void TextureState::enable(RenderState *state)
{
  textureUnit_ = state->nextTextureUnit();
  state->pushTexture(textureUnit_, texture_.get());
  State::enable(state);
}

void TextureState::disable(RenderState *state)
{
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
