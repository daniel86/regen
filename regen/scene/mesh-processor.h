/*
 * mesh.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_MESH_H_
#define REGEN_SCENE_MESH_H_

#include <set>
#include <regen/scene/scene-loader.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/scene-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>

#define REGEN_MESH_NODE_CATEGORY "mesh"

#include <regen/meshes/mesh-state.h>

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates Mesh nodes.
		 */
		class MeshNodeProvider : public NodeProcessor {
		public:
			MeshNodeProvider()
					: NodeProcessor(REGEN_MESH_NODE_CATEGORY) {}

			// Override
			void processInput(
					scene::SceneLoader *scene,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) override;

		protected:
			std::set<Mesh *> usedMeshes_;

			ref_ptr<Mesh> getMeshCopy(const ref_ptr<Mesh> &originalMesh);
		};
	}
}

#endif /* REGEN_SCENE_MESH_H_ */
