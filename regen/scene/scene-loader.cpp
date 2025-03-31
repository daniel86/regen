#include "scene-loader.h"
#include "regen/states/blit-state.h"
#include "regen/states/depth-state.h"
#include "regen/states/tesselation-state.h"
#include "regen/scene/animation-processor.h"
#include "regen/scene/shape-processor.h"
#include "regen/scene/debug-node-processor.h"
#include "regen/effects/filter.h"
#include "regen/effects/motion-blur.h"
#include "regen/states/light-pass.h"
#include "shader-input-processor.h"
#include "regen/states/geometric-picking.h"
#include "regen/effects/bloom-pass.h"
#include "regen/scene/node-processor.h"
#include "regen/states/direct-shading.h"
#include "regen/states/stencil-state.h"
#include <regen/scene/mesh-processor.h>
#include <regen/scene/loadable-input.h>
#include <regen/scene/shader-define-processor.h>

using namespace regen::scene;
using namespace regen;

SceneLoader::SceneLoader(
		Application *application,
		const ref_ptr<SceneInput> &inputProvider)
		: application_(application),
		  inputProvider_(inputProvider) {
	resources_ = ref_ptr<ResourceManager>::alloc();
	physics_ = ref_ptr<BulletPhysics>::alloc();
	physics_->setAnimationName("BulletPhysics");
	physics_->startAnimation();
	init();
	loadShapes();
}

SceneLoader::SceneLoader(
		Application *application,
		const ref_ptr<SceneInput> &inputProvider,
		const ref_ptr<ResourceManager> &resources,
		const ref_ptr<BulletPhysics> &physics)
		: application_(application),
		  inputProvider_(inputProvider),
		  resources_(resources),
		  physics_(physics) {
	init();
	loadShapes();
}

void SceneLoader::init() {
	// add some default node processors
	setNodeProcessor(ref_ptr<SceneNodeProcessor>::alloc());
	setNodeProcessor(ref_ptr<DebugNodeProcessor>::alloc());
	setNodeProcessor(ref_ptr<MeshNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<LoadableNode<BloomPass>>::alloc("bloom"));
	setNodeProcessor(ref_ptr<LoadableNode<GeomPicking>>::alloc("picking"));
	setNodeProcessor(ref_ptr<LoadableNode<DirectShading>>::alloc("direct-shading"));
	setNodeProcessor(ref_ptr<LoadableNode<SkyView>>::alloc("sky"));
	// add some default state processors
	setStateProcessor(ref_ptr<ShaderInputProcessor>::alloc());
	setStateProcessor(ref_ptr<ShaderDefineProcessor>::alloc());
	setStateProcessor(ref_ptr<ResourceStateProvider>::alloc());
	setStateProcessor(ref_ptr<ShapeProcessor>::alloc());
	setStateProcessor(ref_ptr<AnimationProcessor>::alloc());
	setStateProcessor(ref_ptr<LoadableState<BlendState>>::alloc("blend"));
	setStateProcessor(ref_ptr<LoadableState<BlitState>>::alloc("blit"));
	setStateProcessor(ref_ptr<LoadableState<FBOState>>::alloc("fbo"));
	setStateProcessor(ref_ptr<LoadableState<Material>>::alloc("material"));
	setStateProcessor(ref_ptr<LoadableState<DepthState>>::alloc("depth"));
	setStateProcessor(ref_ptr<LoadableState<StateSequence>>::alloc("state-sequence"));
	setStateProcessor(ref_ptr<LoadableState<TesselationState>>::alloc("tesselation"));
	setStateProcessor(ref_ptr<LoadableState<TextureIndexState>>::alloc("texture-index"));
	setStateProcessor(ref_ptr<LoadableState<TextureState>>::alloc("texture"));
	setStateProcessor(ref_ptr<LoadableState<ToggleState>>::alloc("toggle"));
	setStateProcessor(ref_ptr<LoadableState<PolygonState>>::alloc("polygon"));
	setStateProcessor(ref_ptr<LoadableState<CullState>>::alloc("cull"));
	setStateProcessor(ref_ptr<LoadableState<FullscreenPass>>::alloc("fullscreen-pass"));
	setStateProcessor(ref_ptr<LoadableState<FilterSequence>>::alloc("filter-sequence"));
	setStateProcessor(ref_ptr<LoadableState<LightPass>>::alloc("light-pass"));
	setStateProcessor(ref_ptr<LoadableState<MotionBlur>>::alloc("motion-blur"));
	setStateProcessor(ref_ptr<LoadableState<ShaderState>>::alloc("shader"));
	setStateProcessor(ref_ptr<LoadableState<StencilState>>::alloc("stencil"));
	setStateProcessor(ref_ptr<LoadableState2<ModelTransformation>>::alloc("transform"));
	setStateProcessor(ref_ptr<StateResource<Camera>>::alloc("camera"));
}

void SceneLoader::loadShapes() {
	auto root = getRoot();
	auto dummy = ref_ptr<State>::alloc();

	// load shapes that are globally defined
	auto shapeNodes = root->getChildren("shape");
	for (auto &shapeNode: shapeNodes) {
		processState({}, dummy, "shape", shapeNode);
	}
	// load meshes and associated shapes
	auto meshNodes = root->getChildren("mesh");
	for (auto &meshNode: meshNodes) {
		auto meshID = meshNode->getName();
		auto meshVec = resources_->getMesh(this, meshID);
		if (!meshVec.get()) {
			REGEN_WARN("Could not load mesh with id '" << meshID << "'.");
		}
	}
}

void SceneLoader::addEventHandler(GLuint eventID,
								  const ref_ptr<EventHandler> &eventHandler) {
	application_->connect(eventID, eventHandler);
}

const ref_ptr<ShaderInput2i> &SceneLoader::getViewport() const {
	return application_->windowViewport();
}

const ref_ptr<ShaderInput2f> &SceneLoader::getMouseTexco() const {
	return application_->mouseTexco();
}

ref_ptr<SceneInputNode> SceneLoader::getRoot() const {
	auto root = inputProvider_->getRoot();
	if (root->getChildren().size() == 1) {
		return *root->getChildren().begin();
	} else {
		return root;
	}
}

void SceneLoader::setNodeProcessor(const ref_ptr<NodeProcessor> &x) {
	nodeProcessors_[x->category()] = x;
}

void SceneLoader::setStateProcessor(const ref_ptr<StateProcessor> &x) {
	stateProcessors_[x->category()] = x;
}

ref_ptr<State> SceneLoader::getState(const std::string &id) const {
	auto needle = states_.find(id);
	if (needle == states_.end()) {
		return {};
	}
	return needle->second;
}

ref_ptr<NodeProcessor> SceneLoader::getNodeProcessor(const std::string &category) {
	auto needle = nodeProcessors_.find(category);
	if (needle == nodeProcessors_.end()) {
		return {};
	} else {
		return needle->second;
	}
}

ref_ptr<StateProcessor> SceneLoader::getStateProcessor(const std::string &category) {
	auto needle = stateProcessors_.find(category);
	if (needle == stateProcessors_.end()) {
		return {};
	} else {
		return needle->second;
	}
}

void SceneLoader::putNode(const std::string &id, const ref_ptr<StateNode> &node) {
	nodes_[id] = node;
}

ref_ptr<StateNode> SceneLoader::getNode(const std::string &id) {
	return nodes_[id];
}

void SceneLoader::putState(const std::string &id, const ref_ptr<State> &state) {
	states_[id] = state;
}

int SceneLoader::putNamedObject(const ref_ptr<StateNode> &obj) {
	return application_->putNamedObject(obj);
}

ref_ptr<Resource> SceneLoader::getResource(const std::string &category, const std::string &name) {
	if (category == "FBO") {
		return resources_->getFBO(this, name);
	} else if (category == "UBO" || category == "SSBO" || category == "BufferBlock") {
		return resources_->getBufferBlock(this, name);
	} else if (category == "Texture") {
		return resources_->getTexture(this, name);
	} else if (category == "Texture2D") {
		return resources_->getTexture2D(this, name);
	} else if (category == "ModelTransformation") {
		return resources_->getTransform(this, name);
	} else if (category == "AssetImporter") {
		return resources_->getAsset(this, name);
	} else if (category == "Camera") {
		return resources_->getCamera(this, name);
	} else if (category == "Light") {
		return resources_->getLight(this, name);
	} else if (category == "Font") {
		return resources_->getFont(this, name);
	} else if (category == "SpatialIndex") {
		return resources_->getIndex(this, name);
	} else if (category == "Mesh") {
		return resources_->getMesh(this, name);
	} else if (category == "Sky") {
		return resources_->getSky(this, name);
	} else if (category == "State") {
		return getState(name);
	} else {
		REGEN_WARN("Unknown resource category '" << category << "'.");
		return {};
	}
}

void SceneLoader::putResource(const std::string &category, const std::string &name, const ref_ptr<Resource> &v) {
	if (category == "FBO") {
		resources_->putFBO(name, ref_ptr<FBO>::dynamicCast(v));
	} else if (category == "UBO" || category == "SSBO" || category == "BufferBlock") {
		resources_->putBufferBlock(name, ref_ptr<BufferBlock>::dynamicCast(v));
	} else if (category == "Texture") {
		resources_->putTexture(name, ref_ptr<Texture>::dynamicCast(v));
	} else if (category == "ModelTransformation") {
		resources_->putTransform(name, ref_ptr<ModelTransformation>::dynamicCast(v));
	} else if (category == "AssetImporter") {
		resources_->putAsset(name, ref_ptr<AssetImporter>::dynamicCast(v));
	} else if (category == "Camera") {
		resources_->putCamera(name, ref_ptr<Camera>::dynamicCast(v));
	} else if (category == "Light") {
		resources_->putLight(name, ref_ptr<Light>::dynamicCast(v));
	} else if (category == "Font") {
		resources_->putFont(name, ref_ptr<Font>::dynamicCast(v));
	} else if (category == "SpatialIndex") {
		resources_->putIndex(name, ref_ptr<SpatialIndex>::dynamicCast(v));
	} else if (category == "Mesh") {
		resources_->putMesh(name, ref_ptr<MeshVector>::dynamicCast(v));
	} else if (category == "State") {
		putState(name, ref_ptr<State>::dynamicCast(v));
	} else {
		REGEN_WARN("Unknown resource category '" << category << "'.");
	}
}

void SceneLoader::loadResources(const std::string &name) {
	resources_->loadResources(this, name);
}

void SceneLoader::processNode(
		const ref_ptr<StateNode> &parent,
		const std::string &nodeName,
		const std::string &nodeCategory) {
	ref_ptr<NodeProcessor> processor = getNodeProcessor(nodeCategory);
	if (processor.get() == nullptr) {
		REGEN_WARN("No Processor registered for node category '" << nodeCategory << "'.");
		return;
	}
	ref_ptr<SceneInputNode> input = getRoot()->getFirstChild(nodeCategory, nodeName);
	if (input.get() == nullptr) {
		REGEN_WARN("No input for node category '" <<
												  nodeCategory << "' and node name '" << nodeName << "'.");
		return;
	}
	processor->processInput(this, *input.get(), parent);
}

void SceneLoader::processState(
		const ref_ptr<StateNode> &parentNode,
		const ref_ptr<State> &parent,
		const std::string &nodeCategory,
		const ref_ptr<SceneInputNode> &input) {
	ref_ptr<StateProcessor> processor = getStateProcessor(nodeCategory);
	if (processor.get() == nullptr) {
		REGEN_WARN("No Processor registered for node category '" << nodeCategory << "'.");
		return;
	}
	processor->processInput(this, *input.get(), parentNode, parent);
}

std::vector<AnimRange> SceneLoader::getAnimationRanges(const std::string &assetID) {
	ref_ptr<SceneInputNode> root = getRoot();
	ref_ptr<SceneInputNode> importer = root->getFirstChild("asset", assetID);
	if (importer.get() == nullptr) {
		REGEN_WARN("No asset with id '" << assetID << "' known.");
		return {};
	} else {
		const std::list<ref_ptr<SceneInputNode> > &childs = importer->getChildren();

		GLuint animRangeCount = 0u;
		for (const auto & child : childs) {
			if (child->getCategory() == "anim-range") animRangeCount += 1;
		}

		std::vector<AnimRange> out(animRangeCount);
		animRangeCount = 0u;

		for (const auto& n : childs) {
			if (n->getCategory() != "anim-range") continue;
			out[animRangeCount].name = n->getValue("name");
			out[animRangeCount].range = n->getValue<Vec2d>("range", Vec2d(0.0));
			out[animRangeCount].channelName = n->getValue("channel");
			out[animRangeCount].channelIndex = n->getValue<GLuint>("index", 0u);
			animRangeCount += 1u;
		}

		return out;
	}
}

