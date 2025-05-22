#include "mesh-processor.h"

using namespace regen::scene;
using namespace regen;

ref_ptr<Mesh> MeshNodeProvider::getMeshCopy(const ref_ptr<Mesh> &originalMesh) {
	if (originalMesh.get() == nullptr) { return originalMesh; }
	ref_ptr<Mesh> meshCopy;
	if (usedMeshes_.count(originalMesh.get()) == 0) {
		// mesh not referenced yet. Take the reference we have to keep
		// reference on special mesh types like Sky.
		meshCopy = originalMesh;
		usedMeshes_.insert(originalMesh.get());
	} else {
		meshCopy = ref_ptr<Mesh>::alloc(originalMesh);
	}
	return meshCopy;
}

void MeshNodeProvider::processInput(
		scene::SceneLoader *scene,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	ref_ptr<MeshVector> meshes = scene->getResource<MeshVector>(input.getName());
	if (meshes.get() == nullptr) {
		REGEN_WARN("Unable to load Mesh for '" << input.getDescription() << "'.");
		return;
	}
	std::queue<ref_ptr<Mesh>> meshQueue;
	MeshVector::loadIndexRange(input, meshes, meshQueue);

	while (!meshQueue.empty()) {
		auto meshOriginal = meshQueue.front();
		meshQueue.pop();
		auto meshCopy = getMeshCopy(meshOriginal);
		if (input.hasAttribute("primitive")) {
			meshCopy->set_primitive(glenum::primitive(input.getValue("primitive")));
		}
		StateConfigurer meshConfigurer;

		auto baseStateInput = input.getFirstChild("base-state");
		if (baseStateInput.get() != nullptr) {
			// parse state from input
			auto baseState = ref_ptr<State>::alloc();
			for (auto &child: baseStateInput->getChildren()) {
				auto processor = scene->getStateProcessor(child->getCategory());
				if (processor.get() == nullptr) {
					REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
					continue;
				}
				processor->processInput(scene, *child.get(), parent, baseState);
			}
			meshConfigurer.addState(baseState.get());
			input.removeChild(baseStateInput);
		}

		LoadingContext ctx(scene,parent);
		meshCopy->loadShaderConfig(ctx, input);
		bool hasShader = false;
		if (!meshCopy->hasShaderKey()) {
			// try to find a shader in the parent node
			// TODO: what about the LOD levels? might be better to refer to the shader by ID instead
			//        of getting it from the parent. there is also the problem with configuration for compilation
			//        if shader is shared.
			auto meshShader = ShaderState::findShader(parent.get());
			if (meshShader.get() != nullptr) {
				StateConfigurer stateConfigurer;
				stateConfigurer.addNode(parent.get());
				stateConfigurer.addState(meshCopy.get());
				meshCopy->updateVAO(stateConfigurer.cfg(), meshShader);
				hasShader = true;
			}
		}
		if (!hasShader) {
			meshCopy->createShader(parent);
		}
		auto meshNode = ref_ptr<StateNode>::alloc(meshCopy);
		meshNode->set_name("base-mesh");
		parent->addChild(meshNode);
	}
}

