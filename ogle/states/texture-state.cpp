/*
 * texture-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

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

ostream& operator<<(ostream &out, const TextureMapTo &mode)
{
  switch(mode) {
  case MAP_TO_COLOR:            return out << "COLOR";
  case MAP_TO_DIFFUSE:          return out << "DIFFUSE";
  case MAP_TO_AMBIENT:          return out << "AMBIENT";
  case MAP_TO_SPECULAR:         return out << "SPECULAR";
  case MAP_TO_SHININESS:        return out << "SHININESS";
  case MAP_TO_EMISSION:         return out << "EMISSION";
  case MAP_TO_LIGHT:            return out << "LIGHT";
  case MAP_TO_ALPHA:            return out << "ALPHA";
  case MAP_TO_NORMAL:           return out << "NORMAL";
  case MAP_TO_HEIGHT:           return out << "HEIGHT";
  case MAP_TO_DISPLACEMENT:     return out << "DISPLACEMENT";
  case MAP_TO_CUSTOM:
  default:                      return out << "CUSTOM";
  }
  out;
}
istream& operator>>(istream &in, TextureMapTo &mode)
{
  string val;
  in >> val;
  if(val == "COLOR")                    mode = MAP_TO_COLOR;
  else if(val == "DIFFUSE")             mode = MAP_TO_DIFFUSE;
  else if(val == "AMBIENT")             mode = MAP_TO_AMBIENT;
  else if(val == "SPECULAR")            mode = MAP_TO_SPECULAR;
  else if(val == "SHININESS")           mode = MAP_TO_SHININESS;
  else if(val == "EMISSION")            mode = MAP_TO_EMISSION;
  else if(val == "LIGHT")               mode = MAP_TO_LIGHT;
  else if(val == "ALPHA")               mode = MAP_TO_ALPHA;
  else if(val == "NORMAL")              mode = MAP_TO_NORMAL;
  else if(val == "HEIGHT")              mode = MAP_TO_HEIGHT;
  else if(val == "DISPLACEMENT")        mode = MAP_TO_DISPLACEMENT;
  else                                  mode = MAP_TO_CUSTOM;
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
  transferKey_(""),
  transferFunction_(""),
  transferName_(""),
  mappingFunction_(""),
  mappingName_(""),
  blendFunction_(""),
  blendName_(""),
  mapping_(MAPPING_TEXCO),
  mapTo_(MAP_TO_CUSTOM),
  blendFactor_(1.0)
{
  name_ = FORMAT_STRING("Texture" << texture->id());
}

void TextureState::set_name(const string &name)
{
  name_ = name;
}
const string& TextureState::name() const
{
  return name_;
}

const string TextureState::samplerType() const
{
  return texture_->samplerType();
}
const GLint TextureState::id() const
{
  return texture_->id();
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

void TextureState::set_blendFunction(
    const string &blendFunction,
    const string &blendName)
{
  blendFunction_ = blendFunction;
  blendName_ = blendName;
}
const string& TextureState::blendFunction() const
{
  return blendFunction_;
}
const string& TextureState::blendName() const
{
  return blendName_;
}

void TextureState::setMapTo(TextureMapTo id)
{
  mapTo_ = id;
}
TextureMapTo TextureState::mapTo() const
{
  return mapTo_;
}

void TextureState::set_mapping(TextureMapping mapping)
{
  mapping_ = mapping;
}
TextureMapping TextureState::mapping() const
{
  return mapping_;
}

void TextureState::set_mappingFunction(
    const string &mappingFunction,
    const string &mappingName)
{
  mappingFunction_ = mappingFunction;
  mappingName_ = mappingName;
}
const string& TextureState::mappingFunction() const
{
  return mappingFunction_;
}
const string& TextureState::mappingName() const
{
  return mappingName_;
}

const string& TextureState::transferName() const
{
  return transferName_;
}

void TextureState::set_transferFunction(
    const string &transferFunction,
    const string &transferName)
{
  transferKey_ = "";
  transferName_ = transferName;
  transferFunction_ = transferFunction;
}
const string& TextureState::transferFunction() const
{
  return transferFunction_;
}

void TextureState::set_transferKey(
    const string &transferKey,
    const string &transferName)
{
  transferFunction_ = "";
  transferKey_ = transferKey;
  if(transferName.empty()) {
    list<string> path;
    boost::split(path, transferKey, boost::is_any_of("."));
    transferName_ = *path.rbegin();
  } else {
    transferName_ = transferName;
  }
}
const string& TextureState::transferKey() const
{
  return transferKey_;
}

void TextureState::enable(RenderState *state)
{
  textureChannel_ = state->nextTextureUnit();
  state->pushTexture(textureChannel_, this);
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
  state->pushTexture(textureChannel_, this);
}

GLuint TextureConstantUnitNode::textureUnit() const
{
  return textureChannel_;
}
