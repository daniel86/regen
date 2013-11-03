/*
 * mesh.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_MESH_H_
#define REGEN_SCENE_MESH_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/meshes/mesh-state.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates Mesh nodes.
   */
  class MeshNodeProvider : public NodeProcessor {
  public:
    MeshNodeProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &state);
  protected:
    set< Mesh* > usedMeshes_;
  };
}}

#endif /* REGEN_SCENE_MESH_H_ */
