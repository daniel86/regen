/*
 * scene-parser.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "scene-parser.h"
#include "regen/scene/nodes/deformation.h"
#include "regen/scene/nodes/physics.h"
#include "regen/scene/nodes/motion-blur.h"
#include "regen/scene/nodes/picking.h"
#include "regen/scene/states/polygon.h"
#include "regen/scene/nodes/bloom.h"

using namespace regen::scene;
using namespace regen;
using namespace std;

#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resources.h>
#include <regen/scene/resource-manager.h>

#include <regen/scene/nodes/direct-shading.h>
#include <regen/scene/nodes/filter-sequence.h>
#include <regen/scene/nodes/fullscreen-pass.h>
#include <regen/scene/nodes/light-pass.h>
#include <regen/scene/nodes/mesh.h>
#include <regen/scene/nodes/shader.h>
#include <regen/scene/nodes/sky.h>
#include <regen/scene/nodes/scene-node.h>

#include <regen/scene/states/blend.h>
#include <regen/scene/states/blit.h>
#include <regen/scene/states/camera.h>
#include <regen/scene/states/cull.h>
#include <regen/scene/states/define.h>
#include <regen/scene/states/depth.h>
#include <regen/scene/states/fbo.h>
#include <regen/scene/states/input.h>
#include <regen/scene/states/material.h>
#include <regen/scene/states/texture.h>
#include <regen/scene/states/texture-index.h>
#include <regen/scene/states/toggle.h>
#include <regen/scene/states/transform.h>
#include <regen/scene/states/tesselation.h>
#include <regen/scene/states/state-sequence.h>

namespace regen {
	namespace scene {
		string getResourcePath(const string &relPath) {
			PathChoice texPaths;
			texPaths.choices_.push_back(relPath);
			texPaths.choices_.push_back(filesystemPath(
					".", relPath));
			texPaths.choices_.push_back(filesystemPath(
					REGEN_SOURCE_DIR, relPath));
			texPaths.choices_.push_back(filesystemPath(filesystemPath(
					REGEN_SOURCE_DIR, "regen"), relPath));
			texPaths.choices_.push_back(filesystemPath(filesystemPath(
					REGEN_SOURCE_DIR, "applications"), relPath));
			texPaths.choices_.push_back(filesystemPath(filesystemPath(
					REGEN_INSTALL_PREFIX, "share"), relPath));
			return texPaths.firstValidPath();
		}
	}
}

SceneParser::SceneParser(
		Application *application,
		const ref_ptr<SceneInput> &inputProvider)
		: application_(application),
		  inputProvider_(inputProvider) {
	resources_ = ref_ptr<ResourceManager>::alloc();
	physics_ = ref_ptr<BulletPhysics>::alloc();
	init();
}

SceneParser::SceneParser(
		Application *application,
		const ref_ptr<SceneInput> &inputProvider,
		const ref_ptr<ResourceManager> &resources,
		const ref_ptr<BulletPhysics> &physics)
		: application_(application),
		  inputProvider_(inputProvider),
		  resources_(resources),
		  physics_(physics) {
	init();
}

void SceneParser::init() {
	// add some default node processors
	setNodeProcessor(ref_ptr<DirectShadingNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<FilterSequenceNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<FullscreenPassNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<LightPassNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<MeshNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<SkyNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<ShaderNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<SceneNodeProcessor>::alloc());
	setNodeProcessor(ref_ptr<MotionBlurProvider>::alloc());
	setNodeProcessor(ref_ptr<PickingNodeProvider>::alloc());
	setNodeProcessor(ref_ptr<BulletDebuggerProvider>::alloc());
	setNodeProcessor(ref_ptr<BloomProvider>::alloc());
	// add some default state processors
	setStateProcessor(ref_ptr<ResourceStateProvider>::alloc());
	setStateProcessor(ref_ptr<BlendStateProvider>::alloc());
	setStateProcessor(ref_ptr<BlitStateProvider>::alloc());
	setStateProcessor(ref_ptr<CameraStateProvider>::alloc());
	setStateProcessor(ref_ptr<CullStateProvider>::alloc());
	setStateProcessor(ref_ptr<DefineStateProvider>::alloc());
	setStateProcessor(ref_ptr<DepthStateProvider>::alloc());
	setStateProcessor(ref_ptr<FBOStateProvider>::alloc());
	setStateProcessor(ref_ptr<InputStateProvider>::alloc());
	setStateProcessor(ref_ptr<MaterialStateProvider>::alloc());
	setStateProcessor(ref_ptr<TextureStateProvider>::alloc());
	setStateProcessor(ref_ptr<TextureIndexProvider>::alloc());
	setStateProcessor(ref_ptr<ToggleStateProvider>::alloc());
	setStateProcessor(ref_ptr<TransformStateProvider>::alloc());
	setStateProcessor(ref_ptr<TesselationStateProvider>::alloc());
	setStateProcessor(ref_ptr<StateSequenceNodeProvider>::alloc());
	setStateProcessor(ref_ptr<PhysicsStateProvider>::alloc());
	setStateProcessor(ref_ptr<PolygonStateProvider>::alloc());
	setStateProcessor(ref_ptr<DeformationNodeProvider>::alloc());
}

void SceneParser::addEventHandler(GLuint eventID,
								  const ref_ptr<EventHandler> &eventHandler) {
	application_->connect(eventID, eventHandler);
}

const ref_ptr<ShaderInput2i> &SceneParser::getViewport() const {
	return application_->windowViewport();
}

const ref_ptr<ShaderInput2f> &SceneParser::getMouseTexco() const {
	return application_->mouseTexco();
}

ref_ptr<SceneInputNode> SceneParser::getRoot() const {
	auto root = inputProvider_->getRoot();
	if (root->getChildren().size()==1) {
		return *root->getChildren().begin();
	} else {
		return root;
	}
}

void SceneParser::setNodeProcessor(const ref_ptr<NodeProcessor> &x) {
	nodeProcessors_[x->category()] = x;
}

void SceneParser::setStateProcessor(const ref_ptr<StateProcessor> &x) {
	stateProcessors_[x->category()] = x;
}

ref_ptr<State> SceneParser::getState(const std::string &id) const {
	auto needle = states_.find(id);
	if (needle == states_.end()) {
		return {};
	}
	return needle->second;
}

ref_ptr<NodeProcessor> SceneParser::getNodeProcessor(const string &category) {
	auto needle = nodeProcessors_.find(category);
	if (needle == nodeProcessors_.end()) {
		return {};
	} else {
		return needle->second;
	}
}

ref_ptr<StateProcessor> SceneParser::getStateProcessor(const string &category) {
	auto needle = stateProcessors_.find(category);
	if (needle == stateProcessors_.end()) {
		return {};
	} else {
		return needle->second;
	}
}

void SceneParser::putNode(const std::string &id, const ref_ptr<StateNode> &node) {
	nodes_[id] = node;
}

ref_ptr<StateNode> SceneParser::getNode(const std::string &id) {
	return nodes_[id];
}

void SceneParser::putState(const std::string &id, const ref_ptr<State> &state) {
	states_[id] = state;
}

int SceneParser::putNamedObject(const ref_ptr<StateNode> &obj) {
	return application_->putNamedObject(obj);
}

void SceneParser::processNode(
		const ref_ptr<StateNode> &parent,
		const string &nodeName,
		const string &nodeCategory) {
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

void SceneParser::processState(
		const ref_ptr<State> &parent,
		const string &nodeName,
		const string &nodeCategory) {
	ref_ptr<StateProcessor> processor = getStateProcessor(nodeCategory);
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

vector<AnimRange> SceneParser::getAnimationRanges(const std::string &assetID) {
	ref_ptr<SceneInputNode> root = getRoot();
	ref_ptr<SceneInputNode> importer = root->getFirstChild("asset", assetID);
	if (importer.get() == nullptr) {
		REGEN_WARN("No asset with id '" << assetID << "' known.");
		return {};
	} else {
		const list<ref_ptr<SceneInputNode> > &childs = importer->getChildren();

		GLuint animRangeCount = 0u;
		for (auto it = childs.begin(); it != childs.end(); ++it) {
			if ((*it)->getCategory() == "anim-range") animRangeCount += 1;
		}

		vector<AnimRange> out(animRangeCount);
		animRangeCount = 0u;

		for (auto it = childs.begin(); it != childs.end(); ++it) {
			ref_ptr<SceneInputNode> n = *it;
			if (n->getCategory() != "anim-range") continue;
			out[animRangeCount].name = n->getValue("name");
			out[animRangeCount].range = n->getValue<Vec2d>("range", Vec2d(0.0));
			animRangeCount += 1u;
		}

		return out;
	}
}

