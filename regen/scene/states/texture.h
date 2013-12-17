/*
 * texture.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_TEXTURE_H_
#define REGEN_SCENE_TEXTURE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resource-manager.h>

#define REGEN_TEXTURE_STATE_CATEGORY "texture"

#include <regen/states/fbo-state.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates TextureState.
   */
  class TextureStateProvider : public StateProcessor {
  public:
    /**
     * Find texture identified by SceneInputNode.
     * @param parser The scene parser.
     * @param input The scene input node.
     * @param idKey ID input key.
     * @param bufferKey FBO input key.
     * @param attachmentKey FBO attachment input key.
     * @return A texture or a null reference.
     */
    static ref_ptr<Texture> getTexture(
        SceneParser *parser, SceneInputNode &input,
        const string &idKey="id",
        const string &bufferKey="fbo",
        const string &attachmentKey="attachment")
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

    TextureStateProvider()
    : StateProcessor(REGEN_TEXTURE_STATE_CATEGORY)
    {}

    // Override
    void processInput(
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
      if(input.hasAttribute("sampler-type")) {
        texState->set_samplerType(input.getValue("sampler-type"));
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
  };
}}

#endif /* REGEN_SCENE_TEXTURE_H_ */
