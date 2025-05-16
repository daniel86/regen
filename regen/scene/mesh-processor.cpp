#include "mesh-processor.h"

using namespace regen::scene;
using namespace regen;

static ref_ptr<Shader> createMeshShader(
		SceneInputNode &input, const ref_ptr<StateNode> &parent,
		const ref_ptr<Mesh> &meshOriginal, const ref_ptr<Mesh> &meshCopy,
		StateConfigurer &stateConfigurer) {
	stateConfigurer.addNode(parent.get());
	stateConfigurer.addState(meshCopy.get());
	ref_ptr<Shader> meshShader;

	// Handle shader
	auto *hasShader0 = dynamic_cast<HasShader *>(meshCopy.get());
	auto *hasShader1 = dynamic_cast<HasShader *>(meshOriginal.get());
	if (hasShader0 != nullptr) {
		// meshCopy has already a shader.
		hasShader0->createShader(stateConfigurer.cfg());
		meshShader = hasShader0->shaderState()->shader();
	} else if (hasShader1 != nullptr) {
		// meshCopy does not, but meshOriginal has a shader -> join into the copy.
		const std::string shaderKey = hasShader1->shaderKey();
		ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
		meshCopy->joinStates(shaderState);

		shaderState->createShader(stateConfigurer.cfg(), shaderKey);
		meshShader = shaderState->shader();
	} else if (input.hasAttribute("shader")) {
		ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
		meshCopy->joinStates(shaderState);

		const std::string shaderKey = input.getValue("shader");
		std::vector<std::string> shaderKeys(glenum::glslStageCount());
		for (GLint i = 0; i < glenum::glslStageCount(); ++i) {
			auto prefix = glenum::glslStagePrefix(glenum::glslStages()[i]);
			if (input.hasAttribute(prefix)) {
				auto overwriteKey = input.getValue(prefix);
				if (overwriteKey != "NONE") {
					shaderKeys[i] = input.getValue(prefix);
				}
			} else {
				shaderKeys[i] = shaderKey;
			}
		}

		shaderState->createShader(stateConfigurer.cfg(), shaderKeys);
		meshShader = shaderState->shader();
	}
	if (meshShader.get() == nullptr) {
		// Try to find parent shader.
		meshShader = ShaderState::findShader(parent.get());
	}

	if (meshShader.get() == nullptr) {
		REGEN_WARN("Unable to find shader for " << input.getDescription() << ".");
	} else {
		// Update VAO
		meshCopy->updateVAO(RenderState::get(), stateConfigurer.cfg(), meshShader);
	}
	return meshShader;
}

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
	auto meshIndex = input.getValue<int>("mesh-index", -1);

	std::queue<ref_ptr<Mesh>> meshQueue;
	if (meshIndex >= 0 && meshIndex < static_cast<int>(meshes->size())) {
		meshQueue.push((*meshes.get())[meshIndex]);
	} else if (input.hasAttribute("mesh-indices")) {
		auto meshIndices = input.getValue("mesh-indices");
		std::vector<std::string> indices;
		boost::split(indices, meshIndices, boost::is_any_of(","));
		for (auto &index: indices) {
			int i = std::stoi(index);
			if (i >= 0 && i < static_cast<int>(meshes->size())) {
				meshQueue.push((*meshes.get())[i]);
			}
		}
	} else if (input.hasAttribute("mesh-index-range")) {
		auto meshIndexRange = input.getValue("mesh-index-range");
		std::vector<std::string> range;
		boost::split(range, meshIndexRange, boost::is_any_of("-"));
		if (range.size() == 2) {
			int start = range[0].empty() ? 0 : std::stoi(range[0]);
			int end = range[1].empty() ? static_cast<int>(meshes->size()) - 1 : std::stoi(range[1]);
			for (int i = start; i <= end; ++i) {
				if (i >= 0 && i < static_cast<int>(meshes->size())) {
					meshQueue.push((*meshes.get())[i]);
				}
			}
		}
	}
	if (meshQueue.empty()) {
		for (auto &it: *meshes.get()) {
			meshQueue.push(it);
		}
	}

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
		}

		createMeshShader(input, parent, meshOriginal, meshCopy, meshConfigurer);

		auto meshLODs = meshOriginal->meshLODs();
		// remove lod entries that have a lod-mesh, as we will add them below
		meshLODs.erase(std::remove_if(meshLODs.begin(), meshLODs.end(),
									  [](const Mesh::MeshLOD &lod) { return lod.impostorMesh.get() != nullptr; }),
					   meshLODs.end());

		for (auto &child: input.getChildren("lod-mesh")) {
			ref_ptr<MeshVector> lodMeshes = scene->getResource<MeshVector>(child->getName());
			if (lodMeshes.get() == nullptr || lodMeshes->empty()) {
				REGEN_WARN("Unable to load Mesh for '" << child->getDescription() << "'.");
				continue;
			}
			auto lodMeshIndex = child->getValue<uint32_t>("mesh-index", 0u);
			if (lodMeshIndex >= lodMeshes->size()) {
				REGEN_WARN("Invalid mesh index '" << lodMeshIndex << "' for '" << child->getDescription() << "'.");
				lodMeshIndex = 0u;
			}
			auto &lodMeshOriginal = (*lodMeshes.get())[lodMeshIndex];
			auto lodMeshCopy = getMeshCopy(lodMeshOriginal);
			StateConfigurer lodConfigurer;
			// also make states of the original mesh available
			for (auto &state: meshCopy->joined()) {
				if(dynamic_cast<Material*>(state.get()) != nullptr) {
					continue;
				}
				lodConfigurer.addState(state.get());
			}
			for (auto &state: lodMeshOriginal->joined()) {
				lodConfigurer.addState(state.get());
			}
			createMeshShader(*child.get(), parent, lodMeshOriginal, lodMeshCopy, lodConfigurer);
			meshLODs.emplace_back(
					lodMeshCopy->inputContainer()->numVertices(),
					lodMeshCopy->inputContainer()->vertexOffset(),
					lodMeshCopy->inputContainer()->numIndices(),
					lodMeshCopy->inputContainer()->indexOffset(),
					lodMeshCopy);
		}

		meshCopy->setMeshLODs(meshLODs);
		auto meshNode = ref_ptr<StateNode>::alloc(meshCopy);
		parent->addChild(meshNode);
	}
}

