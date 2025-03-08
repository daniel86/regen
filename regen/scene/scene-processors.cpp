#include "scene-processors.h"
#include <regen/scene/scene-loader.h>

using namespace regen::scene;
using namespace regen;

void ResourceStateProvider::processInput(
		scene::SceneLoader *scene,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent,
		const ref_ptr<State> &state) {
	scene->loadResources(input.getName());
}
