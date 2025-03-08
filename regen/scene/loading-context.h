#ifndef REGEN_LOADING_CONTEXT_H
#define REGEN_LOADING_CONTEXT_H

#include "scene-loader.h"
#include "scene-input.h"

namespace regen {
	/**
	 * The context under which a scene is being loaded.
	 * Each load occurs within a specific parent node which is part of the context.
	 */
	class LoadingContext {
	public:
		LoadingContext(scene::SceneLoader *scene, const ref_ptr<StateNode> &parent)
				: scene_(scene), parent_(parent) {}

		scene::SceneLoader *scene() const { return scene_; }

		ref_ptr<StateNode> parent() const { return parent_; }

	private:
		scene::SceneLoader *scene_;
		ref_ptr<StateNode> parent_;
	};
}

#endif //REGEN_LOADING_CONTEXT_H
