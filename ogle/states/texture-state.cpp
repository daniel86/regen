/*
 * texture-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "texture-state.h"
#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>

ostream& operator<<(ostream &out, const TextureMapping &mode)
{
  switch(mode) {
  case MAPPING_FLAT:            return out << "flat";
  case MAPPING_CUBE:            return out << "cube";
  case MAPPING_TUBE:            return out << "tube";
  case MAPPING_SPHERE:          return out << "sphere";
  case MAPPING_REFLECTION:      return out << "reflection";
  case MAPPING_REFRACTION:      return out << "refraction";
  case MAPPING_TEXCO:
  default:                      return out << "texco";
  }
  out;
}
istream& operator>>(istream &in, TextureMapping &mode)
{
  string val;
  in >> val;
  if(val == "flat")             mode = MAPPING_FLAT;
  else if(val == "cube")        mode = MAPPING_CUBE;
  else if(val == "tube")        mode = MAPPING_TUBE;
  else if(val == "sphere")      mode = MAPPING_SPHERE;
  else if(val == "reflection")  mode = MAPPING_REFLECTION;
  else if(val == "refraction")  mode = MAPPING_REFRACTION;
  else                          mode = MAPPING_TEXCO;
  return in;
}

TextureState::TextureState(ref_ptr<Texture> texture)
: State(),
  texture_(texture),
  textureChannel_(0u),
  texcoChannel_(0u),
  blendMode_(BLEND_MODE_SRC),
  useAlpha_(false),
  ignoreAlpha_(false),
  invert_(false),
  transferKey_(""),
  mapping_(MAPPING_TEXCO)
{
}

string TextureState::name()
{
  return FORMAT_STRING("TextureState(" << texture_->name() << ")");
}
const string& TextureState::textureName() const
{
  return texture_->name();
}
const string TextureState::samplerType() const
{
  return texture_->samplerType();
}

void TextureState::set_texcoChannel(GLuint texcoChannel)
{
  texcoChannel_ = texcoChannel;
}
GLuint TextureState::texcoChannel() const
{
  return texcoChannel_;
}

void TextureState::set_ignoreAlpha(GLboolean v)
{
  ignoreAlpha_ = v;
}
GLboolean TextureState::ignoreAlpha() const
{
  return ignoreAlpha_;
}

void TextureState::set_useAlpha(GLboolean v)
{
  useAlpha_ = v;
}
GLboolean TextureState::useAlpha() const
{
  return useAlpha_;
}

void TextureState::set_invert(GLboolean invert)
{
  invert_ = invert;
}
GLboolean TextureState::invert() const
{
  return invert_;
}

void TextureState::set_blendMode(BlendMode blendMode)
{
  blendMode_ = blendMode;
}
BlendMode TextureState::blendMode() const
{
  return blendMode_;
}

void TextureState::set_blendFactor(GLfloat blendFactor)
{
  blendFactor_ = blendFactor;
}
GLfloat TextureState::blendFactor() const
{
  return blendFactor_;
}

void TextureState::set_texelFactor(GLfloat texelFactor)
{
  texelFactor_ = texelFactor;
}
GLfloat TextureState::texelFactor() const
{
  return texelFactor_;
}

void TextureState::addMapTo(TextureMapTo id)
{
  mapTo_.insert( id );
}
const set<TextureMapTo>& TextureState::mapTo() const
{
  return mapTo_;
}
GLboolean TextureState::mapTo(TextureMapTo mapTo) const
{
  return mapTo_.count(mapTo)>0;
}

void TextureState::set_mapping(TextureMapping mapping)
{
  mapping_ = mapping;
}
TextureMapping TextureState::mapping() const
{
  return mapping_;
}


void TextureState::set_transferKey(const string &transferKey)
{
  transferKey_ = transferKey;
}
const string& TextureState::transferKey() const
{
  return transferKey_;
}

void TextureState::enable(RenderState *state)
{
  textureChannel_ = state->nextTextureUnit();
  state->pushTexture(textureChannel_, texture_.get());
  State::enable(state);
}

void TextureState::disable(RenderState *state)
{
  State::disable(state);
  state->popTexture(textureChannel_);
  state->releaseTextureUnit();
}

ref_ptr<Texture>& TextureState::texture()
{
  return texture_;
}

void TextureState::configureShader(ShaderConfig *shaderCfg)
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
  textureChannel_ = textureUnit;
}

void TextureConstantUnitNode::enable(RenderState *state)
{
  State::enable(state);
  state->pushTexture(textureChannel_, texture_.get());
}

GLuint TextureConstantUnitNode::textureUnit() const
{
  return textureChannel_;
}
