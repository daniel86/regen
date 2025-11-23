#include "mesh-processor.h"
#include "regen/shapes/lod-state.h"

using namespace regen::scene;
using namespace regen;

ref_ptr<Mesh> MeshNodeProvider::getMeshCopy(const ref_ptr<Mesh> &originalMesh) {
	if (originalMesh.get() == nullptr) { return originalMesh; }
	ref_ptr<Mesh> meshCopy;
#if 1
	if (usedMeshes_.count(originalMesh.get()) == 0) {
		// mesh not referenced yet. Take the reference we have to keep
		// reference on special mesh types like Sky.
		meshCopy = originalMesh;
		usedMeshes_.insert(originalMesh.get());
	} else {
		meshCopy = ref_ptr<Mesh>::alloc(originalMesh);
	}
#else
	// TODO: Problem with impostor if original is not used.
	if (usedMeshes_.count(originalMesh.get()) == 0) {
		usedMeshes_.insert(originalMesh.get());
	}
	meshCopy = ref_ptr<Mesh>::alloc(originalMesh);
#endif
	return meshCopy;
}

static ref_ptr<LODState> createCullState(
		scene::SceneLoader *parser,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent,
		const ref_ptr<CullShape> &cullShape) {
	// get the parent camera. Note that this will be the light camera in case
	// updating the shadow map.
	auto cam = ref_ptr<Camera>::dynamicCast(parent->getParentCamera());
	if (cam.get() == nullptr) {
		REGEN_WARN("No Camera can be found for '" << input.getDescription() << "'.");
		return {};
	}
	auto spatialIndex = cullShape->spatialIndex();
	if (spatialIndex.get() && !spatialIndex->hasCamera(*cam.get())) {
		// no need to create LOD state if the camera is not used by the spatial index
		return {};
	}
	auto lodState = ref_ptr<LODState>::alloc(cam, cullShape);
	if (input.hasAttribute("sort-mode")) {
		lodState->setInstanceSortMode(input.getValue<SortMode>("sort-mode", SortMode::FRONT_TO_BACK));
	}
	if (input.hasAttribute("use-compaction")) {
		lodState->setUseCompaction(input.getValue<bool>("use-compaction", true));
	}

	parser->putState(input.getName(), lodState);

	return lodState;
}

void MeshNodeProvider::processInput(
		scene::SceneLoader *scene,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	ref_ptr<CompositeMesh> meshes = scene->getResource<CompositeMesh>(input.getName());
	if (meshes.get() == nullptr) {
		REGEN_WARN("Unable to load Mesh for '" << input.getDescription() << "'.");
		return;
	}
	std::queue<std::pair<ref_ptr<Mesh>,uint32_t>> meshQueue;
	CompositeMesh::loadIndexRange(input, meshes, meshQueue);

	while (!meshQueue.empty()) {
		auto [meshOriginal,partIdx] = meshQueue.front();
		meshQueue.pop();
		auto meshCopy = getMeshCopy(meshOriginal);
		if (input.hasAttribute("primitive")) {
			meshCopy->set_primitive(glenum::primitive(input.getValue("primitive")));
		}
		meshCopy->setSortMode(SortMode::FRONT_TO_BACK);
		meshCopy->ensureLOD();
		meshCopy->resetVisibility(false);
		StateConfigurer meshConfigurer;
		auto meshNode = ref_ptr<StateNode>::alloc();
		meshNode->set_name("base-mesh");
		parent->addChild(meshNode);

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

		// load LOD state in case mesh has a cull shape + update-visibility="1"
		if (meshCopy->hasCullShape()) {
			auto updateVisibility = input.getValue<uint32_t>("update-visibility", 1u);
			auto cullShape = ref_ptr<CullShape>::dynamicCast(meshCopy->cullShape());
			if (updateVisibility) {
				if (cullShape.get()) {
					auto lodState = createCullState(scene, input, parent, cullShape);
					if (lodState.get()) {
						meshNode->state()->joinStates(lodState);
						meshCopy->setInstanceBuffer(lodState->instanceBuffer());
						if (lodState->hasIndirectDrawBuffers()) {
							meshCopy->setIndirectDrawBuffer(
								lodState->getIndirectDrawBuffer(meshOriginal),
								0u,
								lodState->numDrawLayers());
						}
						// set sorting mode
						meshCopy->setSortMode(lodState->instanceSortMode());
					} else {
						auto setBaseLOD = ref_ptr<SetBaseLOD>::alloc(meshCopy);
						meshNode->state()->joinStates(setBaseLOD);
					}
				} else {
					REGEN_WARN("Mesh '" << input.getDescription() << "' has no cull shape.");
				}
			} else {
				// try to get an instance buffer
				auto cam = ref_ptr<Camera>::dynamicCast(parent->getParentCamera());
				if (cam.get() && cullShape.get()) {
					if (cullShape->hasIndirectDrawBuffers()) {
						auto dibo = cullShape->getIndirectDrawBuffer(meshOriginal);
						meshCopy->setIndirectDrawBuffer(dibo, 0u, cam->numLayer());
					}
					if (cullShape->hasInstanceBuffer()) {
						meshCopy->setInstanceBuffer(cullShape->instanceBuffer());
					}
					meshCopy->setSortMode(cullShape->instanceSortMode());
				}
			}
		} else if (input.hasAttribute("update-visibility")) {
			REGEN_WARN("Mesh '" << input.getDescription() << "' has no cull shape, but update-visibility is set.");
		}

		LoadingContext ctx(scene, parent);
		meshCopy->loadShaderConfig(ctx, input);
		bool hasShader = false;
		if (!meshCopy->hasShaderKey()) {
			// try to find a shader in the parent node
			// TODO: What about the LOD levels? might be better to refer to the shader by ID instead
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
		meshNode->state()->joinStates(meshCopy);

		// add hidden nodes for LOD meshes to show up in the GUI
		for (auto &lodLevel: meshCopy->meshLODs()) {
			if (lodLevel.impostorMesh.get()) {
				auto lodNode = ref_ptr<StateNode>::alloc(lodLevel.impostorMesh);
				lodNode->set_name("lod-impostor");
				lodNode->set_isHidden(true);
				meshNode->addChild(lodNode);
			}
		}
	}
}

