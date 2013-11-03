/*
 * input-processors.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_INPUT_PROCESSORS_H_
#define REGEN_SCENE_INPUT_PROCESSORS_H_

#include <regen/states/state.h>
#include <regen/states/state-node.h>

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>

namespace regen {
namespace scene {
  /**
   * Processing of Scene input.
   */
  template<class T> class InputProcessor {
  public:
    InputProcessor(const std::string &category)
    : category_(category) {}
    virtual ~InputProcessor() {}

    /**
     * @return The node category of this processor.
     */
    const std::string& category()
    { return category_; }

    /**
     * Process given input node, manipulate parent.
     * @param parser The SceneParser.
     * @param input The SceneInputNode providing input data.
     * @param parent The parent state.
     */
    virtual void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<T> &parent) = 0;

  protected:
    std::string category_;
  };

  /**
   * Processing of Scene input to StateNode instances.
   */
  class NodeProcessor : public InputProcessor<StateNode> {
  public:
    NodeProcessor(const std::string &category)
    : InputProcessor(category) {}
  };
  /**
   * Processing of Scene input to State instances.
   */
  class StateProcessor : public InputProcessor<State> {
  public:
    StateProcessor(const std::string &category)
    : InputProcessor(category) {}
  };
}}


#endif /* REGEN_SCENE_INPUT_PROCESSORS_H_ */
