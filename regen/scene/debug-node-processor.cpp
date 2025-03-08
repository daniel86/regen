#include "debug-node-processor.h"
#include "regen/physics/bullet-debug-drawer.h"
#include "regen/shapes/spatial-index-debug.h"

using namespace regen::scene;
using namespace regen;

void processBulletDebugger(
		scene::SceneLoader *scene,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	auto physics = scene->getPhysics();
	auto debugDrawer = ref_ptr<BulletDebugDrawer>::alloc(physics);
	debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	physics->dynamicsWorld()->setDebugDrawer(debugDrawer.get());
	parent->addChild(debugDrawer);

	StateConfig shaderConfig = StateConfigurer::configure(debugDrawer.get());
	shaderConfig.setVersion(330);
	debugDrawer->createShader(shaderConfig);
}

void processSpatialIndexDebugger(
		scene::SceneLoader *scene,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	auto indexID = input.getValue("resource");
	auto index = scene->getResource<SpatialIndex>(indexID);
	if (index.get() == nullptr) {
		REGEN_WARN("Skipping spatial index debugger with unknown index ID '" << input.getDescription() << "'.");
		return;
	}

	auto debugDrawer = ref_ptr<SpatialIndexDebug>::alloc(index);
	parent->addChild(debugDrawer);

	StateConfig shaderConfig = StateConfigurer::configure(debugDrawer.get());
	shaderConfig.setVersion(330);
	debugDrawer->createShader(shaderConfig);
}

void DebugNodeProcessor::processInput(
		scene::SceneLoader *scene,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	auto debuggerType = input.getValue("type");
	if (debuggerType == "bullet") {
		processBulletDebugger(scene, input, parent);
	} else if (debuggerType == "spatial-index") {
		processSpatialIndexDebugger(scene, input, parent);
	} else {
		REGEN_WARN("Unknown debugger type '" << debuggerType << "'.");
	}
}

