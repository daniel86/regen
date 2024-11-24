#include "mesh.h"

using namespace regen::scene;
using namespace regen;

void MeshNodeProvider::processInput(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	ref_ptr<MeshVector> meshes =
			parser->getResources()->getMesh(parser, input.getName());
	if (meshes.get() == nullptr) {
		REGEN_WARN("Unable to load Mesh for '" << input.getDescription() << "'.");
		return;
	}
	auto meshIndex = input.getValue<int>("mesh-index", -1);

	std::queue<ref_ptr<Mesh>> meshQueue;
	if (meshIndex>=0 && meshIndex < meshes->size()) {
		meshQueue.push((*meshes.get())[meshIndex]);
	}
	else if(input.hasAttribute("mesh-indices")) {
		auto meshIndices = input.getValue("mesh-indices");
		std::vector<std::string> indices;
		boost::split(indices, meshIndices, boost::is_any_of(","));
		for (auto &index : indices) {
			int i = std::stoi(index);
			if (i >= 0 && i < meshes->size()) {
				meshQueue.push((*meshes.get())[i]);
			}
		}
	}
	else if(input.hasAttribute("mesh-index-range")) {
		auto meshIndexRange = input.getValue("mesh-index-range");
		std::vector<std::string> range;
		boost::split(range, meshIndexRange, boost::is_any_of("-"));
		if (range.size() == 2) {
			int start = range[0].empty() ? 0 : std::stoi(range[0]);
			int end = range[1].empty() ? meshes->size() - 1 : std::stoi(range[1]);
			for (int i = start; i <= end; ++i) {
				if (i >= 0 && i < meshes->size()) {
					meshQueue.push((*meshes.get())[i]);
				}
			}
		}
	}
	if (meshQueue.empty()) {
		for (auto it = meshes->begin(); it != meshes->end(); ++it) {
			meshQueue.push(*it);
		}
	}

	while (!meshQueue.empty()) {
		ref_ptr<Mesh> meshResource = meshQueue.front();
		meshQueue.pop();
		if (meshResource.get() == nullptr) {
			REGEN_WARN("null mesh");
			continue;
		}
		ref_ptr<Mesh> mesh;
		if (usedMeshes_.count(meshResource.get()) == 0) {
			// mesh not referenced yet. Take the reference we have to keep
			// reference on special mesh types like Sky.
			mesh = meshResource;
			usedMeshes_.insert(meshResource.get());
		} else {
			mesh = ref_ptr<Mesh>::alloc(meshResource);
		}
		if (input.hasAttribute("primitive")) {
			mesh->set_primitive(glenum::primitive(input.getValue("primitive")));
		}

		ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
		parent->addChild(meshNode);

		StateConfigurer stateConfigurer;
		stateConfigurer.addNode(meshNode.get());
		ref_ptr<Shader> meshShader;

		// Handle shader
		auto *hasShader0 = dynamic_cast<HasShader *>(mesh.get());
		auto *hasShader1 = dynamic_cast<HasShader *>(meshResource.get());
		if (hasShader0 != nullptr) {
			hasShader0->createShader(stateConfigurer.cfg());
			meshShader = hasShader0->shaderState()->shader();
		} else if (hasShader1 != nullptr) {
			const std::string shaderKey = hasShader1->shaderKey();
			ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
			mesh->joinStates(shaderState);

			shaderState->createShader(stateConfigurer.cfg(), shaderKey);
			meshShader = shaderState->shader();
		} else if (input.hasAttribute("shader")) {
			ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
			mesh->joinStates(shaderState);

			const std::string shaderKey = input.getValue("shader");
			std::vector<std::string> shaderKeys(glenum::glslStageCount());
			for (GLint i = 0; i < glenum::glslStageCount(); ++i) {
				auto prefix = glenum::glslStagePrefix(glenum::glslStages()[i]);
				if(input.hasAttribute(prefix)) {
					shaderKeys[i] = input.getValue(prefix);
				} else {
					shaderKeys[i] = shaderKey;
				}
			}

			shaderState->createShader(stateConfigurer.cfg(), shaderKeys);
			meshShader = shaderState->shader();
		}
		if (meshShader.get() == nullptr) {
			// Try to find parent shader.
			meshShader = ShaderNodeProvider::findShader(parent.get());
		}

		if (meshShader.get() == nullptr) {
			REGEN_WARN("Unable to find shader for " << input.getDescription() << ".");
		} else {
			// Update VAO
			mesh->updateVAO(RenderState::get(),
							stateConfigurer.cfg(), meshShader);
		}
	}
}

