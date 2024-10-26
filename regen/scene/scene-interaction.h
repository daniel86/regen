#ifndef REGEN_SCENE_INTERACTION_H
#define REGEN_SCENE_INTERACTION_H

#include <regen/scene/nodes/scene-node.h>

namespace regen {
	/**
	 * Interface for interacting with nodes.
	 */
	class SceneInteraction {
	public:
		virtual ~SceneInteraction() = default;
		virtual bool interactWith(const ref_ptr<StateNode> &node) = 0;
	};
} // regen

#endif //REGEN_SCENE_INTERACTION_H
