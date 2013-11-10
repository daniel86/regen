/*
 * shader.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_SHADER_NODE_H_
#define REGEN_SCENE_SHADER_NODE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/filter.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates FilterSequence nodes.
   */
  class ShaderNodeProvider : public NodeProcessor {
  public:
    static ref_ptr<Shader> findShader(State *s);
    static ref_ptr<Shader> findShader(StateNode *n);

    ShaderNodeProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &state);
  };
}}

#endif /* REGEN_SCENE_SHADER_NODE_H_ */
