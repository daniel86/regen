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
        const string &attachmentKey="attachment");

    TextureStateProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* REGEN_SCENE_TEXTURE_H_ */
