#include "ubo-resource.h"
#include "regen/scene/states/input.h"

using namespace regen::scene;
using namespace regen;

#define REGEN_UBO_CATEGORY "ubo"

UBOResource::UBOResource()
		: ResourceProvider(REGEN_UBO_CATEGORY) {}

ref_ptr<UBO> UBOResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	auto ubo = ref_ptr<UBO>::alloc();
	auto dummyState = ref_ptr<State>::alloc();

	for (auto &n : input.getChildren()) {
		if (n->getCategory() == "uniform") {
			auto uniform = InputStateProvider::createShaderInput(
					parser, *n.get(), dummyState);
			auto name = n->getValue("name");
			ubo->addUniform(uniform, name);
		} else {
			REGEN_WARN("Unknown UBO child category '" << n->getCategory() << "'.");
		}
	}

	GL_ERROR_LOG();

	return ubo;
}
