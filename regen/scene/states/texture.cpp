/*
 * texture.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "texture.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/resource-manager.h>

#define REGEN_TEXTURE_STATE_CATEGORY "texture"

TextureStateProvider::TextureStateProvider()
: StateProcessor(REGEN_TEXTURE_STATE_CATEGORY)
{}

ref_ptr<Texture> TextureStateProvider::getTexture(
    SceneParser *parser, SceneInputNode &input,
    const string &idKey,
    const string &bufferKey,
    const string &attachmentKey)
{
  ref_ptr<Texture> tex;
  // Find the texture resource
  if(input.hasAttribute(idKey)) {
    tex = parser->getResources()->getTexture(parser,input.getValue(idKey));
  }
  else if(input.hasAttribute(bufferKey)) {
    ref_ptr<FBO> fbo = parser->getResources()->getFBO(parser,input.getValue(bufferKey));
    if(fbo.get()==NULL) {
      REGEN_WARN("Unable to find FBO '" << input.getValue(bufferKey) <<
          "' for " << input.getDescription() << ".");
      return tex;
    }
    const string val = input.getValue<string>(attachmentKey, "0");
    if(val == "depth") {
      tex = fbo->depthTexture();
    }
    else {
      vector< ref_ptr<Texture> > &textures = fbo->colorTextures();

      unsigned int attachment;
      stringstream ss(val);
      ss >> attachment;

      if(attachment < textures.size()) {
        tex = textures[attachment];
      }
      else {
        REGEN_WARN("Invalid attachment '" << val <<
            "' for " << input.getDescription() << ".");
      }
    }
  }
  return tex;
}

void TextureStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  ref_ptr<Texture> tex = getTexture(parser,input);
  if(tex.get()==NULL) {
    REGEN_WARN("Skipping unidentified texture node for " << input.getDescription() << ".");
    return;
  }

  // Set-Up the texture state
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::alloc(
      tex, input.getValue("name"));

  texState->set_ignoreAlpha(
      input.getValue<bool>("ignore-alpha", false));
  texState->set_mapTo(input.getValue<TextureState::MapTo>(
      "map-to", TextureState::MAP_TO_CUSTOM));

  // Describes how a texture will be mixed with existing pixels.
  texState->set_blendMode(
      input.getValue<BlendMode>("blend-mode", BLEND_MODE_SRC));
  texState->set_blendFactor(
      input.getValue<GLfloat>("blend-factor", 1.0f));

  const string blendFunctionName = input.getValue("blend-function-name");
  if(input.hasAttribute("blend-function")) {
    texState->set_blendFunction(
        input.getValue("blend-function"),
        blendFunctionName);
  }

  // Defines how a texture should be mapped on geometry.
  texState->set_mapping(input.getValue<TextureState::Mapping>(
      "mapping", TextureState::MAPPING_TEXCO));

  const string mappingFunctionName = input.getValue("mapping-function-name");
  if(input.hasAttribute("mapping-function")) {
    texState->set_mappingFunction(
        input.getValue("mapping-function"),
        mappingFunctionName);
  }

  // texel transfer wraps sampled texels before returning them.
  const string texelTransferName = input.getValue("texel-transfer-name");
  if(input.hasAttribute("texel-transfer-key")) {
    texState->set_texelTransferKey(
        input.getValue("texel-transfer-key"),
        texelTransferName);
  }
  else if(input.hasAttribute("texel-transfer-function")) {
    texState->set_texelTransferFunction(
        input.getValue("texel-transfer-function"),
        texelTransferName);
  }

  // texel transfer wraps computed texture coordinates before returning them.
  if(input.hasAttribute("texco-transfer")) {
    texState->set_texcoTransfer(input.getValue<TextureState::TransferTexco>(
        "texco-transfer", TextureState::TRANSFER_TEXCO_RELIEF));
  }
  const string texcoTransferName = input.getValue("texco-transfer-name");
  if(input.hasAttribute("texco-transfer-key")) {
    texState->set_texcoTransferKey(
        input.getValue("texco-transfer-key"),
        texcoTransferName);
  }
  else if(input.hasAttribute("texco-transfer-function")) {
    texState->set_texcoTransferFunction(
        input.getValue("texco-transfer-function"),
        texcoTransferName);
  }

  state->joinStates(texState);
}
