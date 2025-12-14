#include "composite-mesh.h"
#include "regen/objects/primitives/sphere.h"
#include "regen/objects/primitives/rectangle.h"
#include "regen/objects/primitives/box.h"
#include "regen/objects/primitives/frame.h"
#include "regen/objects/primitives/torus.h"
#include "regen/objects/primitives/disc.h"
#include "regen/objects/sky/lightning-bolt.h"
#include "regen/objects/primitives/point.h"
#include "assimp-importer.h"
#include "mask-mesh.h"
#include "terrain/proc-tree.h"
#include "particles.h"
#include "regen/scene/state-node.h"
#include "regen/scene/loading-context.h"
#include "regen/scene/resource-manager.h"
#include "lod/mesh-simplifier.h"
#include "regen/objects/terrain/ground.h"
#include "regen/objects/lod/impostor-billboard.h"
#include "regen/objects/terrain/grass-patch.h"
#include "silhouette-mesh.h"
#include "primitives/blanket.h"
#include "primitives/cone.h"
#include "terrain/blanket-trail.h"
#include "terrain/ground-path.h"

using namespace regen;

static void processMeshChildren(LoadingContext &ctx, scene::SceneInputNode &input, CompositeMesh &x) {
	auto parser = ctx.scene();

	for (auto &child: input.getChildren()) {
		auto processor = parser->getStateProcessor(child->getCategory());
		if (processor.get() == nullptr) {
			auto res = parser->createResource(*child.get());
			if (!res) {
				REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
			}
			continue;
		}
		if (x.meshes().size() > 1) {
			auto state = ref_ptr<State>::alloc();
			processor->processInput(parser, *child.get(), ctx.parent(), state);
			for (auto &mesh: x.meshes()) {
				mesh->joinStates(state);
			}
		} else {
			processor->processInput(parser, *child.get(), ctx.parent(), x.meshes()[0]);
		}
	}
}

ref_ptr<CompositeMesh> CompositeMesh::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto parser = ctx.scene();

	const std::string meshType = input.getValue("type");
	auto scaling = input.getValue<Vec3f>("scaling", Vec3f::one());
	auto texcoScaling = input.getValue<Vec2f>("texco-scaling", Vec2f::one());
	auto rotation = input.getValue<Vec3f>("rotation", Vec3f::zero());
	bool useNormal = input.getValue<bool>("use-normal", true);
	bool useTexco = input.getValue<bool>("use-texco", true);
	bool useTangent = input.getValue<bool>("use-tangent", false);
	auto accessMode = input.getValue<ClientAccessMode>("access-mode", BUFFER_CPU_WRITE);
	auto mapMode = input.getValue<BufferMapMode>("map-mode", BUFFER_MAP_DISABLED);
	BufferUpdateFlags updateFlags;
	updateFlags.frequency = input.getValue<BufferUpdateFrequency>("update-frequency", BUFFER_UPDATE_NEVER);
	updateFlags.scope = input.getValue<BufferUpdateScope>("update-scope", BUFFER_UPDATE_FULLY);

	ref_ptr<CompositeMesh> out_ = ref_ptr<CompositeMesh>::alloc();
	CompositeMesh *out = out_.get();

	std::vector<uint32_t> lodLevels;
	if (input.hasAttribute("lod-levels")) {
		lodLevels = input.getArray<uint32_t>("lod-levels");
	} else {
		lodLevels.push_back(input.getValue<uint32_t>("lod", 0));
	}

	// Primitives
	if (meshType == "sphere" || meshType == "half-sphere") {
		Sphere::Config meshCfg;
		meshCfg.texcoMode = input.getValue<Sphere::TexcoMode>("texco-mode", Sphere::TEXCO_MODE_UV);
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.posScale = scaling;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.isHalfSphere = (meshType == "half-sphere");
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;

		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<Sphere>::alloc(meshCfg));
	} else if (meshType == "rectangle") {
		Rectangle::Config meshCfg;
		meshCfg.centerAtOrigin = input.getValue<bool>("center", true);
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.isTexcoRequired = useTexco;
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;

		(*out) = CompositeMesh();
		out->addMesh(Rectangle::create(meshCfg));
	} else if (meshType == "blanket") {
		Blanket::BlanketConfig meshCfg;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.blanketSize = input.getValue<Vec2f>("blanket-size", Vec2f(1.0f, 1.0f));
		meshCfg.isInitiallyDead = input.getValue<bool>("initially-dead", false);
		meshCfg.blanketLifetime = input.getValue<float>("blanket-lifetime", 0.0f);
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;
		uint32_t numInstances = input.getValue<uint32_t>("num-instances", 1u);
		auto blanket = ref_ptr<Blanket>::alloc(meshCfg, numInstances, parser->systemTime());
		blanket->updateAttributes();
		(*out) = CompositeMesh();
		out->addMesh(blanket);
	} else if (meshType == "blanket-trail") {
		auto blanket = BlanketTrail::load(ctx, input, lodLevels);
		(*out) = CompositeMesh();
		out->addMesh(blanket);
	} else if (meshType == "box") {
		Box::Config meshCfg;
		meshCfg.texcoMode = input.getValue<Box::TexcoMode>("texco-mode", Box::TEXCO_MODE_UV);
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;
		meshCfg.levelOfDetails = lodLevels;

		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<Box>::alloc(meshCfg));
	} else if (meshType == "frame") {
		FrameMesh::Config meshCfg;
		meshCfg.texcoMode = input.getValue<FrameMesh::TexcoMode>("texco-mode", FrameMesh::TEXCO_MODE_NONE);
		meshCfg.borderSize = input.getValue<float>("border-size", 0.1f);
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;
		meshCfg.levelOfDetails = lodLevels;

		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<FrameMesh>::alloc(meshCfg));
	} else if (meshType == "torus") {
		Torus::Config meshCfg;
		meshCfg.texcoMode = input.getValue<Torus::TexcoMode>("texco-mode", Torus::TEXCO_MODE_UV);
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.updateHints = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.ringRadius = input.getValue<float>("ring-radius", 1.0f);
		meshCfg.tubeRadius = input.getValue<float>("tube-radius", 0.5f);

		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<Torus>::alloc(meshCfg));
	} else if (meshType == "disc") {
		Disc::Config meshCfg;
		meshCfg.texcoMode = input.getValue<Disc::TexcoMode>("texco-mode", Disc::TEXCO_MODE_UV);
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.discRadius = input.getValue<float>("radius", 1.0f);

		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<Disc>::alloc(meshCfg));
	} else if (meshType == "cone" || meshType == "cone-closed") {
		ConeClosed::Config meshCfg;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.radius = input.getValue<float>("radius", 1.0f);
		meshCfg.height = input.getValue<float>("height", 1.0f);
		meshCfg.isBaseRequired = input.getValue<bool>("use-base", true);
		meshCfg.isNormalRequired = useNormal;
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;

		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<ConeClosed>::alloc(meshCfg));
	} else if (meshType == "cone-opened") {
		ConeOpened::Config meshCfg;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.cosAngle = input.getValue<float>("angle", 0.5f);
		meshCfg.height = input.getValue<float>("height", 1.0f);
		meshCfg.isNormalRequired = useNormal;
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;

		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<ConeOpened>::alloc(meshCfg));
	}
	// Special meshes
	else if (meshType == "particles") {
		const auto numParticles = input.getValue<uint32_t>("num-vertices", 0u);
		if (numParticles == 0u) {
			REGEN_WARN("Ignoring " << input.getDescription() << " with num-vertices=0.");
		} else {
			(*out) = CompositeMesh();
			out->addMesh(createParticleMesh(ctx, input, numParticles));
			return out_;
		}
	} else if (meshType == "lightning") {
		auto mesh = ref_ptr<LightningBolt>::alloc();
		mesh->load(ctx, input);
		mesh->startAnimation();

		(*out) = CompositeMesh();
		out->addMesh(mesh);
	} else if (meshType == "point") {
		const auto numVertices = input.getValue<uint32_t>("num-vertices", 1u);
		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<Point>::alloc(numVertices));
	} else if (meshType == "asset") {
		ref_ptr<AssetImporter> importer = parser->getResource<AssetImporter>(input.getValue("asset"));
		if (importer.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << " with unknown Asset.");
		} else {
			out_ = createCompositeMesh(ctx, input, importer);
			out = out_.get();
		}
		for (uint32_t i = 0u; i < out->meshes().size(); ++i) {
			parser->putState(REGEN_STRING(input.getName() << i), out->meshes()[i]);
		}
	} else if (meshType == "impostor-billboard") {
		auto impostor = ImpostorBillboard::load(ctx, input);
		if (impostor.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load impostor billboard.");
		} else {
			(*out) = CompositeMesh();
			out->addMesh(impostor);
			impostor->ensureLOD();

			ref_ptr<CompositeMesh> baseMeshComposite = ctx.scene()->getResource<CompositeMesh>(input.getValue("base-mesh"));
			if (baseMeshComposite.get()) {
				std::queue<std::pair<ref_ptr<Mesh>, uint32_t> > baseMeshQueue;
				loadIndexRange(input, baseMeshComposite, baseMeshQueue, "base-mesh");
				auto [baseMesh,_idx] = baseMeshQueue.front();
				if (baseMeshQueue.size() > 1) {
					REGEN_WARN("multiple base mesh indices in lod-mesh in '" << input.getDescription() << "'.");
				}
				baseMesh->addMeshLOD(Mesh::MeshLOD(impostor));
			} else {
				REGEN_WARN("Ignoring base-mesh in '" << input.getDescription() << "', failed to load base mesh.");
			}
		}
	} else if (meshType == "silhouette") {
		auto silhouette = SilhouetteMesh::load(ctx, input);
		if (silhouette.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load silhouette.");
		} else {
			(*out) = CompositeMesh();
			out->addMesh(silhouette);
		}
	} else if (meshType == "mask-patch" || meshType == "grass-patch") {
		Rectangle::Config meshCfg;
		meshCfg.centerAtOrigin = true;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = input.hasAttribute("use-normal") && useNormal;
		meshCfg.isTangentRequired = input.hasAttribute("use-tangent") && useTangent;
		meshCfg.isTexcoRequired = input.hasAttribute("use-texco") && useTexco;
		meshCfg.updateHint = updateFlags;
		meshCfg.mapMode = mapMode;
		meshCfg.accessMode = accessMode;

		ref_ptr<Mesh> m;
		if (meshType == "grass-patch") {
			m = GrassPatch::load(ctx, input, meshCfg);
		} else {
			m = MaskMesh::load(ctx, input, meshCfg);
		}
		if (m.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load ground mesh.");
		} else {
			(*out) = CompositeMesh();
			out->addMesh(m);
		}
	} else if (meshType == "ground") {
		auto groundMesh = Ground::load(ctx, input);
		if (groundMesh.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load ground mesh.");
		} else if (groundMesh->skirtMesh().get()) {
			(*out) = CompositeMesh();
			out->addMesh(groundMesh);
			out->addMesh(groundMesh->skirtMesh());
		} else {
			(*out) = CompositeMesh();
			out->addMesh(groundMesh);
		}
	} else if (meshType == "ground-path") {
		auto pathMesh = GroundPath::load(ctx, input);
		(*out) = CompositeMesh();
		out->addMesh(pathMesh);
	} else if (meshType == "proctree") {
		auto procTree = ref_ptr<ProcTree>::alloc(input);
		procTree->update();
		(*out) = CompositeMesh();
		out->addMesh(procTree->trunkMesh());
		out->addMesh(procTree->twigMesh());
	} else if (meshType == "text") {
		(*out) = CompositeMesh();
		out->addMesh(createTextMesh(ctx, input));
	} else if (meshType == "mesh") {
		GLenum primitive = glenum::primitive(input.getValue<std::string>("primitive", "TRIANGLES"));
		(*out) = CompositeMesh();
		out->addMesh(ref_ptr<Mesh>::alloc(primitive, updateFlags));
	} else {
		REGEN_WARN("Ignoring " << input.getDescription() << ", unknown Mesh type.");
	}
	if (out->meshes().size() == 0) {
		REGEN_WARN("Ignoring " << input.getDescription() << ", no Mesh created.");
		return {};
	}
	// put resources early such that children can refer to it
	if (out->meshes().size() == 1) {
		parser->putState(input.getName(), out->meshes().front());
	}
	parser->putResource<CompositeMesh>(input.getName(), out_);

	std::vector<ref_ptr<scene::SceneInputNode> > visited;
	// configure shader of meshes.
	// note: the shader is not compiled here, we only store the import keys
	// for the meshes, such that createShader() can be called later.
	for (auto &child: input.getChildren("shader")) {
		std::queue<std::pair<ref_ptr<Mesh>, uint32_t> > meshQueue;
		CompositeMesh::loadIndexRange(*child.get(), out_, meshQueue);
		visited.push_back(child);
		while (!meshQueue.empty()) {
			auto [mesh,_idx] = meshQueue.front();
			meshQueue.pop();
			mesh->loadShaderConfig(ctx, *child.get());
		}
	}

	// Set number of instances if requested
	const uint32_t numInstances = input.getValue<uint32_t>("num-instances", 1u);
	if (numInstances > 1u) {
		for (auto &mesh: out->meshes()) {
			mesh->set_numInstances(numInstances);
			mesh->set_numVisibleInstances(numInstances);
		}
	}

	// generate LOD levels if requested
	if (input.hasAttribute("lod-simplification")) {
		auto thresholds = input.getValue<Vec4f>(
			"lod-simplification",
			Vec4f(1.0f, 0.75f, 0.25f, 1.0f));
		for (auto &mesh: out->meshes()) {
			MeshSimplifier simplifier(mesh);
			simplifier.setThresholds(thresholds);
			if (input.hasAttribute("nor-max-angle")) {
				simplifier.setNormalMaxAngle(input.getValue<float>("nor-max-angle", 0.6f));
			}
			if (input.hasAttribute("nor-penalty")) {
				simplifier.setNormalPenalty(input.getValue<float>("nor-penalty", 0.1f));
			}
			if (input.hasAttribute("valence-penalty")) {
				simplifier.setValencePenalty(input.getValue<float>("valence-penalty", 0.1f));
			}
			if (input.hasAttribute("area-penalty")) {
				simplifier.setAreaPenalty(input.getValue<float>("area-penalty", 0.1f));
			}
			if (input.hasAttribute("lod-strict-boundary")) {
				simplifier.setUseStrictBoundary(input.getValue<bool>("lod-strict-boundary", false));
			}
			simplifier.simplifyMesh();
		}
	}

	// Remember LOD-meshes, we need to process them after the base mesh is fully configured
	std::vector<ref_ptr<scene::SceneInputNode> > lodMeshes;
	auto lodMeshInput = input.getFirstChild("lod-meshes");
	if (lodMeshInput.get() != nullptr) {
		visited.push_back(lodMeshInput);
		for (auto &meshChild: lodMeshInput->getChildren("mesh")) {
			lodMeshes.push_back(meshChild);
		}
	}
	// remove visited children
	for (auto &child: visited) {
		input.removeChild(child);
	}

	// configure mesh LOD
	{
		auto thresholds = input.getValue<Vec3f>(
			"lod-thresholds", Vec3f(10.0, 50.0, 100.0));
		for (const auto &mesh: out->meshes()) {
			auto numLODs = mesh->numLODs();
			if (numLODs == 0) {
				continue;
			}
			mesh->setLODThresholds(thresholds);
		}
	}

	if (input.hasAttribute("primitive")) {
		GLenum primitive = glenum::primitive(input.getValue("primitive"));
		for (auto &mesh : out->meshes()) {
			mesh->set_primitive(primitive);
		}
	}

	// Mesh resources can have State children
	if (!out->meshes().empty()) processMeshChildren(ctx, input, *out);

	for (auto &meshChild: lodMeshes) {
		// First try to find existing CompositeMesh resource
		ref_ptr<CompositeMesh> lodMeshVec = parser->getResources()->getMesh(parser, meshChild->getValue("id"));
		if (lodMeshVec.get() == nullptr || lodMeshVec->meshes().empty()) {
			// Else try to create new one from child node
			lodMeshVec = parser->getResources()->createMesh(parser, *meshChild.get());
		}
		if (lodMeshVec.get() == nullptr || lodMeshVec->meshes().empty()) {
			REGEN_WARN("Ignoring " << meshChild->getDescription() << ", failed to load lod mesh.");
			continue;
		}
		auto lodMesh = lodMeshVec->meshes()[0];
		if (lodMeshVec->meshes().size() > 1) {
			REGEN_WARN("multiple lod mesh indices in '" << meshChild->getDescription() << "'.");
		}
		lodMesh->shaderDefine("HAS_LOD", "TRUE");
		REGEN_DEBUG("Adding LOD mesh '" << meshChild->getName() << "' to base mesh.");

		for (auto &mesh: out->meshes()) {
			std::stack<ref_ptr<State>> stateStack;
			for (auto &state: *mesh->joined().get()) {
				stateStack.push(state);
			}
			if (mesh->material().get()) {
				stateStack.push(mesh->material());
			}
			while (!stateStack.empty()) {
				auto state = stateStack.top();
				stateStack.pop();
				if (auto *textureState = dynamic_cast<TextureState*>(state.get())) {
					if (textureState->texture()->targetType() == GL_TEXTURE_BUFFER) {
						// TBOs must be joined, they could be used for instancing of uniforms
						lodMesh->joinStates(state);
					} else {
						continue; // skip (material) texture states, we will bake them into the snapshot textures
					}
				} else {
					for (auto &stateInput: state->inputs()) {
						if (dynamic_cast<Texture*>(stateInput.in_.get()) != nullptr) {
							continue; // skip texture inputs, we will bake them into the snapshot textures
						}
						if (!stateInput.in_->isVertexAttribute()) {
							lodMesh->setInput(stateInput.in_, stateInput.name_);
						}
					}
				}
				for (auto &joined: *state->joined().get()) {
					stateStack.push(joined);
				}
			}
		}
	}

	return out_;
}

ref_ptr<CompositeMesh> CompositeMesh::createCompositeMesh(
	LoadingContext &ctx,
	scene::SceneInputNode &input,
	const ref_ptr<AssetImporter> &importer) {
	const auto scaling = input.getValue<Vec3f>("scaling", Vec3f::one());
	const auto rotation = input.getValue<Vec3f>("rotation", Vec3f::zero());
	const auto translation = input.getValue<Vec3f>("translation", Vec3f::zero());
	const auto assetIndices = input.getValue<std::string>("asset-indices", "*");
	bool useAnimation = input.getValue<bool>("asset-animation", false);
	BufferUpdateFlags updateFlags;
	updateFlags.frequency = input.getValue<BufferUpdateFrequency>("update-frequency", BUFFER_UPDATE_NEVER);
	updateFlags.scope = input.getValue<BufferUpdateScope>("update-scope", BUFFER_UPDATE_FULLY);

	BufferFlags bufferConfig(ARRAY_BUFFER, updateFlags);
	if (input.hasAttribute("access-mode")) {
		bufferConfig.accessMode = input.getValue<ClientAccessMode>("access-mode", BUFFER_CPU_WRITE);
	}
	if (input.hasAttribute("map-mode")) {
		bufferConfig.mapMode = input.getValue<BufferMapMode>("map-mode", BUFFER_MAP_DISABLED);
	}

	ref_ptr<CompositeMesh> out_ = ref_ptr<CompositeMesh>::alloc();
	CompositeMesh &out = *out_.get();

	ref_ptr<BoneTree> nodeAnim = importer->getNodeAnimation();
	if (useAnimation && !nodeAnim) {
		REGEN_WARN(input.getDescription() << " has use-animation=1 but Asset '" <<
			input.getValue("asset") << "' has not.");
		useAnimation = false;
	}

	// Create import model transform
	Mat4f transform = Mat4f::scaleMatrix(scaling);
	Quaternion q(0.0, 0.0, 0.0, 1.0);
	q.setEuler(rotation.x, rotation.y, rotation.z);
	transform *= q.calculateMatrix();
	transform.translate(translation);

	// Asset can contain multiple meshes, they can be accessed by index.
	// Parse user specified indices ...
	std::vector<std::string> indicesStr;
	boost::split(indicesStr, assetIndices, boost::is_any_of(","));
	std::vector<uint32_t> indices(indicesStr.size());
	bool useAllIndices = false;
	for (uint32_t i = 0u; i < indices.size(); ++i) {
		if (indicesStr[i] == "*") {
			useAllIndices = true;
			break;
		} else {
			std::stringstream ss(indicesStr[i]);
			ss >> indices[i];
		}
	}

	if (useAllIndices) {
		for (auto &mesh : importer->loadAllMeshes(transform, bufferConfig)) {
			out.addMesh(mesh);
		}
	} else {
		for (auto &mesh : importer->loadMeshes(transform, bufferConfig, indices)) {
			out.addMesh(mesh);
		}
	}
	for (uint32_t i = 0u; i < out.meshes().size(); ++i) {
		ref_ptr<Mesh> mesh = out.meshes()[i];
		if (mesh.get() == nullptr) continue;

		// Join in material state.
		// Can be set to false to allow overwriting material stuff.
		if (input.getValue<bool>("asset-material", true)) {
			ref_ptr<Material> material =
					importer->getMeshMaterial(mesh.get());
			if (material.get() != nullptr) {
				mesh->setMaterial(material);
			}
		}

		if (useAnimation) {
			std::vector<ref_ptr<BoneNode>> meshBones;
			uint32_t numBoneWeights = importer->numBoneWeights(mesh.get());

			// Find bones influencing this mesh
			if (nodeAnim.get()) {
				auto ibonNodes = importer->loadMeshBones(mesh.get(), nodeAnim.get());
				meshBones.insert(meshBones.end(), ibonNodes.begin(), ibonNodes.end());
			}

			// Create Bones state that is responsible for uploading
			// animation data to GL.
			if (!meshBones.empty()) {
				ref_ptr<Bones> bonesState = ref_ptr<Bones>::alloc(nodeAnim, meshBones, numBoneWeights);
				bonesState->setAnimationName(REGEN_STRING("bones-" << input.getName()));
				bonesState->startAnimation();
				mesh->joinStates(bonesState);
			}
		}
	}

	return out_;
}

template<class InputType, class ValueType>
static void configureParticleAttribute(
	scene::SceneLoader *parser,
	const ref_ptr<Particles> &particles,
	scene::SceneInputNode &input) {
	const std::string name = input.getValue("name");
	if (input.hasAttribute("default")) {
		auto optional = input.getValue<ValueType>("default");
		if (optional.has_value()) {
			particles->setDefault<InputType, ValueType>(name, optional.value());
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to parse default value.");
		}
	}
	if (input.hasAttribute("variance")) {
		auto optional = input.getValue<ValueType>("variance");
		if (optional.has_value()) {
			particles->setVariance<InputType, ValueType>(name, optional.value());
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to parse variance value.");
		}
	}
	if (input.hasAttribute("advance-mode")) {
		auto optional = input.getValue<Particles::AdvanceMode>("advance-mode");
		if (optional.has_value()) {
			particles->setAdvanceMode(name, optional.value());
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to parse advance mode.");
		}
	}
	if (input.hasAttribute("advance-function")) {
		auto optional = input.getValue<std::string>("advance-function");
		if (optional.has_value()) {
			particles->setAdvanceFunction(name, optional.value());
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to parse advance function.");
		}
	}
	if (input.hasAttribute("advance-factor")) {
		auto optional = input.getValue<float>("advance-factor");
		if (optional.has_value()) {
			particles->setAdvanceFactor(name, optional.value());
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to parse advance factor.");
		}
	}
	if (input.hasAttribute("advance-constant")) {
		auto optional = input.getValue<ValueType>("advance-constant");
		if (optional.has_value()) {
			particles->setAdvanceConstant<InputType, ValueType>(name, optional.value());
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to parse advance constant.");
		}
	}
	if (input.hasAttribute("advance-ramp")) {
		auto rampMode = input.getValue<Particles::RampMode>("advance-ramp-mode", Particles::RAMP_MODE_LIFETIME);
		auto rampTexture = parser->getResource<Texture>(input.getValue("advance-ramp"));
		if (rampTexture.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load advance ramp texture.");
		} else {
			particles->setAdvanceRamp(name, rampTexture, rampMode);
		}
	}
}

ref_ptr<Particles> CompositeMesh::createParticleMesh(LoadingContext &ctx, scene::SceneInputNode &input,
                                                     const uint32_t numParticles) {
	ref_ptr<Particles> particles;
	auto parser = ctx.scene();
	if (input.hasAttribute("update-shader")) {
		const auto updateShader = input.getValue("update-shader");
		particles = ref_ptr<Particles>::alloc(numParticles, updateShader);
	} else {
		particles = ref_ptr<Particles>::alloc(numParticles);
	}
	particles->setAnimationName(input.getName());
	particles->startAnimation();

	if (input.hasAttribute("max-emits")) {
		particles->setMaxEmits(input.getValue<uint32_t>("max-emits", 100u));
	}
	if (input.hasAttribute("animation-state")) {
		auto animNodeName = input.getValue("animation-state");
		auto animState = parser->getState(animNodeName);
		if (animState.get() == nullptr) {
			// Try to load the animation node
			auto animNodeInput = parser->getRoot()->getFirstChild("node", animNodeName);
			if (animNodeInput.get() != nullptr) {
				auto animNode = ref_ptr<StateNode>::alloc();
				animNode->set_name(animNodeName);
				parser->processNode(animNode, animNodeName);
				if (animNode->childs().empty()) {
					animState = animNode->state();
				} else {
					animState = animNode->childs().front()->state();
				}
				parser->putState(animNodeName, animState);
			}
		}
		if (animState.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << ", unknown animation-node.");
		} else {
			particles->joinAnimationState(animState);
		}
	}

	// the attributes of particles may have special attributes in the scene file that determine default, variance, etc.
	// these are ignored by processMeshChildren, so we need to process them here.
	for (auto &child: input.getChildren()) {
		if (child->getCategory() == "input" && child->getValue<bool>("is-attribute", false)) {
			const std::string type = child->getValue("type");
			if (type == "float") {
				configureParticleAttribute<ShaderInput1f, float>(parser, particles, *child.get());
			} else if (type == "vec2") {
				configureParticleAttribute<ShaderInput2f, Vec2f>(parser, particles, *child.get());
			} else if (type == "vec3") {
				configureParticleAttribute<ShaderInput3f, Vec3f>(parser, particles, *child.get());
			} else if (type == "vec4") {
				configureParticleAttribute<ShaderInput4f, Vec4f>(parser, particles, *child.get());
			} else if (type == "int") {
				configureParticleAttribute<ShaderInput1i, int>(parser, particles, *child.get());
			} else if (type == "ivec2") {
				configureParticleAttribute<ShaderInput2i, Vec2i>(parser, particles, *child.get());
			} else if (type == "ivec3") {
				configureParticleAttribute<ShaderInput3i, Vec3i>(parser, particles, *child.get());
			} else if (type == "ivec4") {
				configureParticleAttribute<ShaderInput4i, Vec4i>(parser, particles, *child.get());
			} else {
				REGEN_WARN("Ignoring " << child->getDescription() << ", unknown attribute type '" << type << "'.");
			}
		}
	}

	particles->begin();
	// Mesh resources can have State children
	auto x = ref_ptr<CompositeMesh>::alloc();
	x->addMesh(particles);
	// Make the mesh available for child nodes.
	parser->putResource<CompositeMesh>(input.getName(), x);
	processMeshChildren(ctx, input, *x.get());
	particles->end();

	return particles;
}

ref_ptr<TextureMappedText> CompositeMesh::createTextMesh(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<regen::Font> font =
			ctx.scene()->getResource<Font>(input.getValue("font"));
	if (font.get() == nullptr) {
		REGEN_WARN("Unable to load Font for '" << input.getDescription() << "'.");
		return {};
	}

	auto textHeight = input.getValue<float>("height", 16.0f);
	auto textColor = input.getValue<Vec4f>("text-color", Vec4f(0.97f, 0.86f, 0.77f, 0.95f));

	auto widget = ref_ptr<TextureMappedText>::alloc(font, textHeight);
	widget->set_color(textColor);
	widget->set_centerAtOrigin(input.getValue<bool>("center", true));

	if (input.hasAttribute("text")) {
		auto maxLineWidth = input.getValue<float>("max-line-width", 0.0f);
		TextureMappedText::Alignment alignment = TextureMappedText::ALIGNMENT_LEFT;
		if (input.hasAttribute("alignment")) {
			std::string align = input.getValue("alignment");
			if (align == "center") alignment = TextureMappedText::ALIGNMENT_CENTER;
			else if (align == "right") alignment = TextureMappedText::ALIGNMENT_RIGHT;
		}

		std::string val = input.getValue("text");
		std::wstringstream ss;
		ss << val.c_str();
		widget->set_value(ss.str(), alignment, maxLineWidth);
	}

	return widget;
}

std::vector<uint32_t> CompositeMesh::loadIndexRange(scene::SceneInputNode &input, const std::string &prefix) {
	std::vector<uint32_t> out;
	auto meshIndex = input.getValue<int>(REGEN_STRING(prefix << "-index"), -1);
	if (meshIndex >= 0) {
		out.push_back(meshIndex);
	} else if (input.hasAttribute(REGEN_STRING(prefix << "-indices"))) {
		out = input.getArray<uint32_t>(REGEN_STRING(prefix << "-indices"));
	} else if (input.hasAttribute(REGEN_STRING(prefix << "-index-range"))) {
		auto meshIndexRange = input.getValue(REGEN_STRING(prefix << "-index-range"));
		std::vector<std::string> range;
		boost::split(range, meshIndexRange, boost::is_any_of("-"));
		if (range.size() == 2 && !range[0].empty() && !range[1].empty()) {
			int start = std::stoi(range[0]);
			int end = std::stoi(range[1]);
			for (int i = start; i <= end; ++i) {
				if (i >= 0) {
					out.push_back(i);
				} else {
					REGEN_WARN("Ignoring " << input.getDescription() << ", invalid mesh index '" << i << "'.");
				}
			}
		}
	}

	return out;
}

void CompositeMesh::loadIndexRange(
	scene::SceneInputNode &input,
	const ref_ptr<CompositeMesh> &compositeMesh,
	std::queue<std::pair<ref_ptr<Mesh>, uint32_t> > &meshQueue,
	const std::string &prefix) {
	auto indexRange = CompositeMesh::loadIndexRange(input, prefix);
	if (indexRange.empty()) {
		uint32_t idx = 0u;
		for (auto &it: compositeMesh->meshes()) {
			meshQueue.push({it, idx++});
		}
	} else {
		for (auto &index: indexRange) {
			if (index >= 0 && index < static_cast<uint32_t>(compositeMesh->meshes().size())) {
				meshQueue.push({compositeMesh->meshes()[index], index});
			} else {
				REGEN_WARN("Ignoring " << input.getDescription() << ", invalid mesh index '" << index << "'.");
			}
		}
	}
}
