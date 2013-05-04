/*
 * texture-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include <regen/utility/string-util.h>

#include "texture-state.h"
namespace regen {

ostream& operator<<(ostream &out, const TextureState::Mapping &mode)
{
  switch(mode) {
  case TextureState::MAPPING_FLAT:            return out << "flat";
  case TextureState::MAPPING_CUBE:            return out << "cube";
  case TextureState::MAPPING_TUBE:            return out << "tube";
  case TextureState::MAPPING_SPHERE:          return out << "sphere";
  case TextureState::MAPPING_REFLECTION:      return out << "reflection";
  case TextureState::MAPPING_REFRACTION:      return out << "refraction";
  case TextureState::MAPPING_CUSTOM:          return out << "custom";
  case TextureState::MAPPING_TEXCO:           return out << "texco";
  }
  return out;
}
istream& operator>>(istream &in, TextureState::Mapping &mode)
{
  string val;
  in >> val;
  if(val == "flat")             mode = TextureState::MAPPING_FLAT;
  else if(val == "cube")        mode = TextureState::MAPPING_CUBE;
  else if(val == "tube")        mode = TextureState::MAPPING_TUBE;
  else if(val == "sphere")      mode = TextureState::MAPPING_SPHERE;
  else if(val == "reflection")  mode = TextureState::MAPPING_REFLECTION;
  else if(val == "refraction")  mode = TextureState::MAPPING_REFRACTION;
  else if(val == "custom")      mode = TextureState::MAPPING_CUSTOM;
  else                          mode = TextureState::MAPPING_TEXCO;
  return in;
}

ostream& operator<<(ostream &out, const TextureState::MapTo &mode)
{
  switch(mode) {
  case TextureState::MAP_TO_COLOR:            return out << "COLOR";
  case TextureState::MAP_TO_DIFFUSE:          return out << "DIFFUSE";
  case TextureState::MAP_TO_AMBIENT:          return out << "AMBIENT";
  case TextureState::MAP_TO_SPECULAR:         return out << "SPECULAR";
  case TextureState::MAP_TO_SHININESS:        return out << "SHININESS";
  case TextureState::MAP_TO_EMISSION:         return out << "EMISSION";
  case TextureState::MAP_TO_LIGHT:            return out << "LIGHT";
  case TextureState::MAP_TO_ALPHA:            return out << "ALPHA";
  case TextureState::MAP_TO_NORMAL:           return out << "NORMAL";
  case TextureState::MAP_TO_HEIGHT:           return out << "HEIGHT";
  case TextureState::MAP_TO_DISPLACEMENT:     return out << "DISPLACEMENT";
  case TextureState::MAP_TO_CUSTOM:           return out << "CUSTOM";
  }
  return out;
}
istream& operator>>(istream &in, TextureState::MapTo &mode)
{
  string val;
  in >> val;
  if(val == "COLOR")              mode = TextureState::MAP_TO_COLOR;
  else if(val == "DIFFUSE")       mode = TextureState::MAP_TO_DIFFUSE;
  else if(val == "AMBIENT")       mode = TextureState::MAP_TO_AMBIENT;
  else if(val == "SPECULAR")      mode = TextureState::MAP_TO_SPECULAR;
  else if(val == "SHININESS")     mode = TextureState::MAP_TO_SHININESS;
  else if(val == "EMISSION")      mode = TextureState::MAP_TO_EMISSION;
  else if(val == "LIGHT")         mode = TextureState::MAP_TO_LIGHT;
  else if(val == "ALPHA")         mode = TextureState::MAP_TO_ALPHA;
  else if(val == "NORMAL")        mode = TextureState::MAP_TO_NORMAL;
  else if(val == "HEIGHT")        mode = TextureState::MAP_TO_HEIGHT;
  else if(val == "DISPLACEMENT")  mode = TextureState::MAP_TO_DISPLACEMENT;
  else                            mode = TextureState::MAP_TO_CUSTOM;
  return in;
}

#define __TEX_NAME(x) REGEN_STRING(x << stateID_)

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
  ignoreAlpha_(GL_FALSE)
{
  *channelPtr_ = -1;
  set_blendMode( BLEND_MODE_SRC );
  set_blendFactor(1.0f);
  set_mapping(MAPPING_TEXCO);
  set_mapTo(MAP_TO_CUSTOM);
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
  ignoreAlpha_(GL_FALSE)
{
  *channelPtr_ = -1;
  set_blendMode( BLEND_MODE_SRC );
  set_blendFactor(1.0f);
  set_mapping(MAPPING_TEXCO);
  set_mapTo(MAP_TO_CUSTOM);
}
TextureState::~TextureState()
{
  if(channelPtr_!=NULL) delete channelPtr_;
}

void TextureState::set_texture(const ref_ptr<Texture> &tex)
{
  texture_ = tex;
  if(tex.get()) {
    set_name( REGEN_STRING("Texture" << tex->id()));
    shaderDefine(__TEX_NAME("TEX_SAMPLER_TYPE"), tex->samplerType());
    shaderDefine(__TEX_NAME("TEX_DIM"), REGEN_STRING(tex->numComponents()));
  }
}
const ref_ptr<Texture>& TextureState::texture() const
{
  return texture_;
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

GLuint TextureState::stateID() const
{
  return stateID_;
}

GLint* TextureState::channel() const
{
  return channelPtr_;
}

void TextureState::set_texcoChannel(GLuint texcoChannel)
{
  texcoChannel_ = texcoChannel;
  shaderDefine(__TEX_NAME("TEX_TEXCO"), REGEN_STRING("texco" << texcoChannel_));
}
GLuint TextureState::texcoChannel() const
{
  return texcoChannel_;
}

void TextureState::set_ignoreAlpha(GLboolean v)
{
  ignoreAlpha_ = v;
  shaderDefine(__TEX_NAME("TEX_IGNORE_ALPHA"), v?"TRUE":"FALSE");
}
GLboolean TextureState::ignoreAlpha() const
{
  return ignoreAlpha_;
}

void TextureState::set_blendFactor(GLfloat blendFactor)
{
  blendFactor_ = blendFactor;
  shaderDefine(__TEX_NAME("TEX_BLEND_FACTOR"), REGEN_STRING(blendFactor_));
}
void TextureState::set_blendMode(BlendMode blendMode)
{
  blendMode_ = blendMode;
  shaderDefine(__TEX_NAME("TEX_BLEND_KEY"),  REGEN_STRING("blending." << blendMode_));
  shaderDefine(__TEX_NAME("TEX_BLEND_NAME"), REGEN_STRING("blend_" << blendMode_));
}
void TextureState::set_blendFunction(const string &blendFunction, const string &blendName)
{
  blendFunction_ = blendFunction;
  blendName_ = blendName;

  shaderFunction(blendName_, blendFunction_);
  shaderDefine(__TEX_NAME("TEX_BLEND_KEY"), blendName_);
  shaderDefine(__TEX_NAME("TEX_BLEND_NAME"), blendName_);
}

void TextureState::set_mapTo(MapTo id)
{
  mapTo_ = id;
  shaderDefine(__TEX_NAME("TEX_MAPTO"), REGEN_STRING(mapTo_));
}

void TextureState::set_mapping(TextureState::Mapping mapping)
{
  mapping_ = mapping;
  shaderDefine(__TEX_NAME("TEX_MAPPING_KEY"), REGEN_STRING("textures.texco_" << mapping));
  shaderDefine(__TEX_NAME("TEX_MAPPING_NAME"), REGEN_STRING("texco_" << mapping));
  shaderDefine(__TEX_NAME("TEX_TEXCO"), REGEN_STRING("texco" << texcoChannel_));
}
void TextureState::set_mappingFunction(const string &mappingFunction, const string &mappingName)
{
  mappingFunction_ = mappingFunction;
  mappingName_ = mappingName;

  shaderFunction(mappingName_, mappingFunction_);
  shaderDefine(__TEX_NAME("TEX_MAPPING_KEY"), mappingName_);
  shaderDefine(__TEX_NAME("TEX_MAPPING_NAME"), mappingName_);
}

///////
///////


void TextureState::set_texelTransferFunction(const string &transferFunction, const string &transferName)
{
  transferKey_ = "";
  transferName_ = transferName;
  transferFunction_ = transferFunction;

  shaderFunction(transferName_, transferFunction_);
  shaderDefine(__TEX_NAME("TEX_TRANSFER_KEY"), transferName_);
  shaderDefine(__TEX_NAME("TEX_TRANSFER_NAME"), transferName_);
}
void TextureState::set_texelTransferKey(const string &transferKey, const string &transferName)
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

///////
///////

void TextureState::set_texcoTransferFunction(const string &transferFunction, const string &transferName)
{
  transferTexcoKey_ = "";
  transferTexcoName_ = transferName;
  transferTexcoFunction_ = transferFunction;

  shaderFunction(transferTexcoName_, transferTexcoFunction_);
  shaderDefine(__TEX_NAME("TEXCO_TRANSFER_KEY"), transferTexcoName_);
  shaderDefine(__TEX_NAME("TEXCO_TRANSFER_NAME"), transferTexcoName_);
}

void TextureState::set_texcoTransfer(TransferTexco mode)
{
  switch(mode) {
  case TRANSFER_TEXCO_FISHEYE:
    set_texcoTransferKey("textures.fisheyeTransfer");
    break;
  case TRANSFER_TEXCO_PARALLAX:
    set_texcoTransferKey("textures.parallaxTransfer");
    break;
  case TRANSFER_TEXCO_PARALLAX_OCC:
    set_texcoTransferKey("textures.parallaxOcclusionTransfer");
    break;
  case TRANSFER_TEXCO_RELIEF:
    set_texcoTransferKey("textures.reliefTransfer");
    break;
  }
}

void TextureState::set_texcoTransferKey(const string &transferKey, const string &transferName)
{
  transferTexcoFunction_ = "";
  transferTexcoKey_ = transferKey;
  if(transferName.empty()) {
    list<string> path;
    boost::split(path, transferKey, boost::is_any_of("."));
    transferTexcoName_ = *path.rbegin();
  } else {
    transferTexcoName_ = transferName;
  }
  shaderDefine(__TEX_NAME("TEXCO_TRANSFER_KEY"), transferTexcoKey_);
  shaderDefine(__TEX_NAME("TEXCO_TRANSFER_NAME"), transferTexcoName_);
}

///////
///////

void TextureState::enable(RenderState *rs)
{
  // TODO: avoid texture bound multiple times
  *channelPtr_ = rs->reserveTextureChannel();
  texture_->begin(rs, *channelPtr_);
  State::enable(rs);
}

void TextureState::disable(RenderState *rs)
{
  State::disable(rs);
  texture_->end(rs, *channelPtr_);
  rs->releaseTextureChannel();
}

}
