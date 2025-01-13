#include "debugger.h"
#include "regen/physics/bullet-debug-drawer.h"
#include "regen/shapes/spatial-index-debug.h"

using namespace regen::scene;
using namespace regen;

void processBulletDebugger(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	auto &physics = parser->getPhysics();
	auto debugDrawer = ref_ptr<BulletDebugDrawer>::alloc(physics);
	debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	physics->dynamicsWorld()->setDebugDrawer(debugDrawer.get());
	parent->addChild(debugDrawer);

	StateConfig shaderConfig = StateConfigurer::configure(debugDrawer.get());
	shaderConfig.setVersion(330);
	debugDrawer->createShader(shaderConfig);
}

void processSpatialIndexDebugger(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	auto indexID = input.getValue("resource");
	auto index = parser->getResources()->getIndex(parser, indexID);
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

void DebuggerProvider::processInput(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	auto debuggerType = input.getValue("type");
	if (debuggerType == "bullet") {
		processBulletDebugger(parser, input, parent);
	} else if (debuggerType == "spatial-index") {
		processSpatialIndexDebugger(parser, input, parent);
	} else {
		REGEN_WARN("Unknown debugger type '" << debuggerType << "'.");
	}
}

