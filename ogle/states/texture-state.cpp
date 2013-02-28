/*
 * texture-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "texture-state.h"
#include <ogle/utility/string-util.h>

ostream& operator<<(ostream &out, const TextureMapping &mode)
{
  switch(mode) {
  case MAPPING_FLAT:            return out << "flat";
  case MAPPING_CUBE:            return out << "cube";
  case MAPPING_TUBE:            return out << "tube";
  case MAPPING_SPHERE:          return out << "sphere";
  case MAPPING_REFLECTION:      return out << "reflection";
  case MAPPING_REFRACTION:      return out << "refraction";
  case MAPPING_CUSTOM:          return out << "custom";
  case MAPPING_TEXCO:
  default:                      return out << "texco";
  }
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
  else if(val == "custom")      mode = MAPPING_CUSTOM;
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

#define __TEX_NAME(x) FORMAT_STRING(x << stateID_)

GLuint TextureState::idCounter_ = 0;

TextureState::TextureState(const ref_ptr<Texture> &texture, const string &name)
: State(),
  stateID_(++idCounter_),
  channelPtr_(new GLint),
  blendFunction_(""),
  blendName_(""),
  mappingFunction_(""),
  mappingName_(""),
  transferKey_(""),
  transferFunction_(""),
  transferName_(""),
  texcoChannel_(0u),
  useAlpha_(GL_FALSE),
  ignoreAlpha_(GL_FALSE),
  mapTo_(MAP_TO_CUSTOM)
{
  *channelPtr_ = -1;
  set_blendMode( BLEND_MODE_SRC );
  set_blendFactor(1.0f);
  set_mapping(MAPPING_TEXCO);
  set_texture(texture);
  if(!name.empty()) {
    set_name(name);
  }
}
TextureState::TextureState()
: State(),
  stateID_(++idCounter_),
  channelPtr_(new GLint),
  blendFunction_(""),
  blendName_(""),
  mappingFunction_(""),
  mappingName_(""),
  transferKey_(""),
  transferFunction_(""),
  transferName_(""),
  texcoChannel_(0u),
  useAlpha_(GL_FALSE),
  ignoreAlpha_(GL_FALSE),
  mapTo_(MAP_TO_CUSTOM)
{
  *channelPtr_ = -1;
  set_blendMode( BLEND_MODE_SRC );
  set_blendFactor(1.0f);
  set_mapping(MAPPING_TEXCO);
}
TextureState::~TextureState()
{
  if(channelPtr_!=NULL) delete channelPtr_;
}

void TextureState::set_texture(const ref_ptr<Texture> &tex)
{
  texture_ = tex;
  if(tex.get()) {
    set_name( FORMAT_STRING("Texture" << tex->id()));
    shaderDefine(__TEX_NAME("TEX_SAMPLER_TYPE"), tex->samplerType());
    shaderDefine(__TEX_NAME("TEX_DIM"), FORMAT_STRING(tex->numComponents()));
  }
}

void TextureState::set_name(const string &name)
{
  name_ = name;
  shaderDefine(__TEX_NAME("TEX_NAME"), name_);
}
const string& TextureState::name() const
{
  return name_;
}

void TextureState::set_samplerType(const string &samplerType)
{
  texture_->set_samplerType(samplerType);
  shaderDefine(__TEX_NAME("TEX_SAMPLER_TYPE"), samplerType);
}
const string& TextureState::samplerType() const
{
  return texture_->samplerType();
}
const GLint TextureState::id() const
{
  return texture_->id();
}
GLuint TextureState::stateID() const
{
  return stateID_;
}

GLuint TextureState::dimension() const
{
  return texture_->numComponents();
}

GLint TextureState::channel() const
{
  return *channelPtr_;
}
GLint* TextureState::channelPtr() const
{
  return channelPtr_;
}

void TextureState::set_texcoChannel(GLuint texcoChannel)
{
  texcoChannel_ = texcoChannel;
  shaderDefine(__TEX_NAME("TEX_TEXCO"), FORMAT_STRING("texco" << texcoChannel_));
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

void TextureState::set_blendFactor(GLfloat blendFactor)
{
  blendFactor_ = blendFactor;
  shaderDefine(__TEX_NAME("TEX_BLEND_FACTOR"), FORMAT_STRING(blendFactor_));
}
GLfloat TextureState::blendFactor() const
{
  return blendFactor_;
}

void TextureState::set_blendMode(BlendMode blendMode)
{
  blendMode_ = blendMode;
  shaderDefine(__TEX_NAME("TEX_BLEND_KEY"),  FORMAT_STRING("blending." << blendMode_));
  shaderDefine(__TEX_NAME("TEX_BLEND_NAME"), FORMAT_STRING("blend_" << blendMode_));
}
BlendMode TextureState::blendMode() const
{
  return blendMode_;
}
void TextureState::set_blendFunction(const string &blendFunction, const string &blendName)
{
  blendFunction_ = blendFunction;
  blendName_ = blendName;

  shaderFunction(blendName_, blendFunction_);
  shaderDefine(__TEX_NAME("TEX_BLEND_KEY"), blendName_);
  shaderDefine(__TEX_NAME("TEX_BLEND_NAME"), blendName_);
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
  shaderDefine(__TEX_NAME("TEX_MAPTO"), FORMAT_STRING(mapTo_));
}
TextureMapTo TextureState::mapTo() const
{
  return mapTo_;
}

void TextureState::set_mapping(TextureMapping mapping)
{
  mapping_ = mapping;
  shaderDefine(__TEX_NAME("TEX_MAPPING_KEY"), FORMAT_STRING("textures.texco_" << mapping));
  shaderDefine(__TEX_NAME("TEX_MAPPING_NAME"), FORMAT_STRING("texco_" << mapping));
  shaderDefine(__TEX_NAME("TEX_TEXCO"), FORMAT_STRING("texco" << texcoChannel_));
}
TextureMapping TextureState::mapping() const
{
  return mapping_;
}

void TextureState::set_mappingFunction(const string &mappingFunction, const string &mappingName)
{
  mappingFunction_ = mappingFunction;
  mappingName_ = mappingName;

  shaderFunction(mappingName_, mappingFunction_);
  shaderDefine(__TEX_NAME("TEX_MAPPING_KEY"), mappingName_);
  shaderDefine(__TEX_NAME("TEX_MAPPING_NAME"), mappingName_);
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

void TextureState::set_transferFunction(const string &transferFunction, const string &transferName)
{
  transferKey_ = "";
  transferName_ = transferName;
  transferFunction_ = transferFunction;

  shaderFunction(transferName_, transferFunction_);
  shaderDefine(__TEX_NAME("TEX_TRANSFER_KEY"), transferName_);
  shaderDefine(__TEX_NAME("TEX_TRANSFER_NAME"), transferName_);
}
const string& TextureState::transferFunction() const
{
  return transferFunction_;
}

void TextureState::set_transferKey(const string &transferKey, const string &transferName)
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
  shaderDefine(__TEX_NAME("TEX_TRANSFER_KEY"), transferKey_);
  shaderDefine(__TEX_NAME("TEX_TRANSFER_NAME"), transferName_);
}
const string& TextureState::transferKey() const
{
  return transferKey_;
}

void TextureState::enable(RenderState *state)
{
  *channelPtr_ = state->reserveTextureChannel();
  state->texture().push(*channelPtr_, texture_.get());
  State::enable(state);
}

void TextureState::disable(RenderState *state)
{
  State::disable(state);
  state->texture().pop(*channelPtr_);
  state->releaseTextureChannel();
}

const ref_ptr<Texture>& TextureState::texture() const
{
  return texture_;
}
