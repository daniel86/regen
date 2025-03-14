#include "mesh-vector.h"
#include "regen/meshes/primitives/sphere.h"
#include "regen/meshes/primitives/rectangle.h"
#include "regen/meshes/primitives/box.h"
#include "regen/meshes/primitives/frame.h"
#include "regen/meshes/primitives/torus.h"
#include "regen/meshes/primitives/disc.h"
#include "regen/sky/lightning-bolt.h"
#include "regen/meshes/primitives/point.h"
#include "assimp-importer.h"
#include "mask-mesh.h"
#include "proc-tree.h"
#include "particles.h"
#include "regen/states/state-node.h"
#include "regen/scene/loading-context.h"

using namespace regen;

static void processMeshChildren(LoadingContext &ctx, scene::SceneInputNode &input, MeshVector &x) {
	auto parser = ctx.scene();

	for (auto &child: input.getChildren()) {
		auto processor = parser->getStateProcessor(child->getCategory());
		if (processor.get() == nullptr) {
			REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
			continue;
		}
		if (x.size() > 1) {
			auto state = ref_ptr<State>::alloc();
			processor->processInput(parser, *child.get(), ctx.parent(), state);
			for (auto &mesh: x) {
				mesh->joinStates(state);
			}
		} else {
			processor->processInput(parser, *child.get(), ctx.parent(), x[0]);
		}
	}
}

ref_ptr<MeshVector> MeshVector::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto parser = ctx.scene();

	const std::string meshType = input.getValue("type");
	auto scaling = input.getValue<Vec3f>("scaling", Vec3f(1.0f));
	auto texcoScaling = input.getValue<Vec2f>("texco-scaling", Vec2f(1.0f));
	auto rotation = input.getValue<Vec3f>("rotation", Vec3f(0.0f));
	bool useNormal = input.getValue<bool>("use-normal", true);
	bool useTexco = input.getValue<bool>("use-texco", true);
	bool useTangent = input.getValue<bool>("use-tangent", false);
	auto vboUsage = input.getValue<VBO::Usage>("usage", VBO::USAGE_DYNAMIC);

	ref_ptr<MeshVector> out_ = ref_ptr<MeshVector>::alloc();
	MeshVector *out = out_.get();

	std::vector<GLuint> lodLevels;
	if (input.hasAttribute("lod-levels")) {
		auto lodVec = input.getValue<Vec3ui>("lod-levels", Vec3ui(0));
		lodLevels.resize(3);
		lodLevels[0] = lodVec.x;
		lodLevels[1] = lodVec.y;
		lodLevels[2] = lodVec.z;
	} else {
		lodLevels.push_back(input.getValue<GLuint>("lod", 0));
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
		meshCfg.usage = vboUsage;

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Sphere>::alloc(meshCfg);
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
		meshCfg.usage = vboUsage;

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Rectangle>::alloc(meshCfg);
	} else if (meshType == "box") {
		Box::Config meshCfg;
		meshCfg.texcoMode = input.getValue<Box::TexcoMode>("texco-mode", Box::TEXCO_MODE_UV);
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.usage = vboUsage;
		meshCfg.levelOfDetails = lodLevels;

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Box>::alloc(meshCfg);
	} else if (meshType == "frame") {
		FrameMesh::Config meshCfg;
		meshCfg.texcoMode = input.getValue<FrameMesh::TexcoMode>("texco-mode", FrameMesh::TEXCO_MODE_NONE);
		meshCfg.borderSize = input.getValue<GLfloat>("border-size", 0.1f);
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.usage = vboUsage;
		meshCfg.levelOfDetail = lodLevels[0];

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<FrameMesh>::alloc(meshCfg);
	} else if (meshType == "torus") {
		Torus::Config meshCfg;
		meshCfg.texcoMode = input.getValue<Torus::TexcoMode>("texco-mode", Torus::TEXCO_MODE_UV);
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.usage = vboUsage;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.ringRadius = input.getValue<GLfloat>("ring-radius", 1.0f);
		meshCfg.tubeRadius = input.getValue<GLfloat>("tube-radius", 0.5f);

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Torus>::alloc(meshCfg);
	} else if (meshType == "disc") {
		Disc::Config meshCfg;
		meshCfg.texcoMode = input.getValue<Disc::TexcoMode>("texco-mode", Disc::TEXCO_MODE_UV);
		meshCfg.posScale = scaling;
		meshCfg.rotation = rotation;
		meshCfg.texcoScale = texcoScaling;
		meshCfg.isNormalRequired = useNormal;
		meshCfg.isTangentRequired = useTangent;
		meshCfg.usage = vboUsage;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.discRadius = input.getValue<GLfloat>("radius", 1.0f);

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Disc>::alloc(meshCfg);
	} else if (meshType == "cone" || meshType == "cone-closed") {
		ConeClosed::Config meshCfg;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.radius = input.getValue<GLfloat>("radius", 1.0f);
		meshCfg.height = input.getValue<GLfloat>("height", 1.0f);
		meshCfg.isBaseRequired = input.getValue<bool>("use-base", true);
		meshCfg.isNormalRequired = useNormal;
		meshCfg.usage = vboUsage;

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<ConeClosed>::alloc(meshCfg);
	} else if (meshType == "cone-opened") {
		ConeOpened::Config meshCfg;
		meshCfg.levelOfDetails = lodLevels;
		meshCfg.cosAngle = input.getValue<GLfloat>("angle", 0.5f);
		meshCfg.height = input.getValue<GLfloat>("height", 1.0f);
		meshCfg.isNormalRequired = useNormal;
		meshCfg.usage = vboUsage;

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<ConeOpened>::alloc(meshCfg);
	}
		// Special meshes
	else if (meshType == "particles") {
		const auto numParticles = input.getValue<GLuint>("num-vertices", 0u);
		if (numParticles == 0u) {
			REGEN_WARN("Ignoring " << input.getDescription() << " with num-vertices=0.");
		} else {
			(*out) = MeshVector(1);
			(*out)[0] = createParticleMesh(ctx, input, numParticles);
			return out_;
		}
	} else if (meshType == "lightning") {
		LightningBolt::Config meshCfg;
		meshCfg.maxBranches_ = input.getValue<GLuint>("max-branches", 10u);
		meshCfg.maxSubDivisions_ = input.getValue<GLuint>("max-subdivisions", 8u);
		auto mesh = ref_ptr<LightningBolt>::alloc(meshCfg);

		if (input.hasAttribute("source-pos")) {
			mesh->setSourcePosition(input.getValue<Vec3f>("source-pos", Vec3f::zero()));
		}
		if (input.hasAttribute("target-pos")) {
			mesh->setTargetPosition(input.getValue<Vec3f>("target-pos", Vec3f::zero()));
		}
		// TODO: support moving targets here
		if (input.hasAttribute("lifetime")) {
			auto lifetime = input.getValue<Vec2f>("lifetime", Vec2f(2.0f, 0.5f));
			mesh->setLifetime(lifetime.x, lifetime.y);
		}
		if (input.hasAttribute("jitter-offset")) {
			mesh->setJitterOffset(input.getValue<GLfloat>("jitter-offset", 8.0f));
		}
		if (input.hasAttribute("frequency")) {
			auto frequency = input.getValue<Vec2f>("frequency", Vec2f(0.0f));
			mesh->setFrequency(frequency.x, frequency.y);
		}
		if (input.hasAttribute("branch-probability")) {
			mesh->setBranchProbability(input.getValue<GLfloat>("branch-probability", 0.5f));
		}
		if (input.hasAttribute("branch-offset")) {
			mesh->setBranchOffset(input.getValue<GLfloat>("branch-offset", 0.5f));
		}
		if (input.hasAttribute("branch-length")) {
			mesh->setBranchLength(input.getValue<GLfloat>("branch-length", 0.5f));
		}
		if (input.hasAttribute("branch-darkening")) {
			mesh->setBranchDarkening(input.getValue<GLfloat>("branch-darkening", 0.5f));
		}
		mesh->startAnimation();

		(*out) = MeshVector(1);
		(*out)[0] = mesh;
	} else if (meshType == "point") {
		const auto numVertices = input.getValue<GLuint>("num-vertices", 1u);
		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Point>::alloc(numVertices);
	} else if (meshType == "asset") {
		ref_ptr<AssetImporter> importer = parser->getResource<AssetImporter>(input.getValue("asset"));
		if (importer.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << " with unknown Asset.");
		} else {
			out_ = createAssetMeshes(ctx, input, importer);
			out = out_.get();
		}
		for (GLuint i = 0u; i < out->size(); ++i) {
			parser->putState(REGEN_STRING(input.getName() << i), (*out)[i]);
		}
	} else if (meshType == "mask-patch") {
		MaskMesh::Config meshCfg;
		meshCfg.quad.centerAtOrigin = true;
		meshCfg.quad.levelOfDetails = lodLevels;
		meshCfg.quad.posScale = scaling;
		meshCfg.quad.rotation = rotation;
		meshCfg.quad.texcoScale = texcoScaling;
		meshCfg.quad.isNormalRequired = input.hasAttribute("use-normal") && useNormal;
		meshCfg.quad.isTangentRequired = input.hasAttribute("use-tangent") && useTangent;
		meshCfg.quad.isTexcoRequired = input.hasAttribute("use-texco") && useTexco;
		meshCfg.quad.usage = vboUsage;
		if (input.hasAttribute("height-map")) {
			meshCfg.heightMap = parser->getResource<Texture2D>(input.getValue("height-map"));
		}
		meshCfg.height = input.getValue<float>("height", 0.0f);
		meshCfg.meshSize = input.getValue<Vec2f>("ground-size", Vec2f(10.0f));
		auto maskTexture = parser->getResource<Texture2D>(input.getValue("mask"));
		if (maskTexture.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load mask texture.");
		} else {
			(*out) = MeshVector(1);
			(*out)[0] = ref_ptr<MaskMesh>::alloc(maskTexture, meshCfg);
		}
	} else if (meshType == "proctree") {
		auto procTree = ref_ptr<ProcTree>::alloc(input);
		procTree->update();
		(*out) = MeshVector(2);
		(*out)[0] = procTree->trunkMesh();
		(*out)[1] = procTree->twigMesh();
	} else if (meshType == "text") {
		(*out) = MeshVector(1);
		(*out)[0] = createTextMesh(ctx, input);
	} else if (meshType == "mesh") {
		GLenum primitive = glenum::primitive(input.getValue<std::string>("primitive", "TRIANGLES"));
		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Mesh>::alloc(primitive, vboUsage);
	} else {
		REGEN_WARN("Ignoring " << input.getDescription() << ", unknown Mesh type.");
	}

	// configure mesh LOD
	if (input.hasAttribute("lod-far")) {
		auto lodFar = input.getValue<GLfloat>("lod-far", 160.0f);
		for (GLuint i = 0u; i < out->size(); ++i) {
			(*out)[i]->setLODFar(lodFar);
		}
	}

	if (out->size() == 1) {
		ref_ptr<Mesh> mesh = (*out)[0];
		parser->putState(input.getName(), mesh);
	}
	parser->putResource<MeshVector>(input.getName(), out_); // TODO check if this is needed

	if (input.hasAttribute("primitive")) {
		GLenum primitive = glenum::primitive(input.getValue("primitive"));
		for (GLuint i = 0u; i < out->size(); ++i) {
			(*out)[i]->set_primitive(primitive);
		}
	}

	// Mesh resources can have State children
	if (!out->empty()) processMeshChildren(ctx, input, *out);

	return out_;
}

ref_ptr<MeshVector> MeshVector::createAssetMeshes(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<AssetImporter> &importer) {
	const auto vboUsage = input.getValue<VBO::Usage>("usage", VBO::USAGE_DYNAMIC);
	const auto scaling = input.getValue<Vec3f>("scaling", Vec3f(1.0f));
	const auto rotation = input.getValue<Vec3f>("rotation", Vec3f(0.0f));
	const auto translation = input.getValue<Vec3f>("translation", Vec3f(0.0f));
	const auto assetIndices = input.getValue<std::string>("asset-indices", "*");
	bool useAnimation = input.getValue<bool>("asset-animation", false);

	ref_ptr<MeshVector> out_ = ref_ptr<MeshVector>::alloc();
	MeshVector &out = *out_.get();

	const vector <ref_ptr<NodeAnimation>> &nodeAnims = importer->getNodeAnimations();
	if (useAnimation && nodeAnims.empty()) {
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
	vector <GLuint> indices(indicesStr.size());
	bool useAllIndices = false;
	for (GLuint i = 0u; i < indices.size(); ++i) {
		if (indicesStr[i] == "*") {
			useAllIndices = true;
			break;
		} else {
			std::stringstream ss(indicesStr[i]);
			ss >> indices[i];
		}
	}

	if (useAllIndices) {
		out = importer->loadAllMeshes(transform, vboUsage);
	} else {
		out = importer->loadMeshes(transform, vboUsage, indices);
	}
	for (GLuint i = 0u; i < out.size(); ++i) {
		ref_ptr<Mesh> mesh = out[i];
		if (mesh.get() == nullptr) continue;

		// Join in material state.
		// Can be set to false to allow overwriting material stuff.
		if (input.getValue<bool>("asset-material", true)) {
			ref_ptr<Material> material =
					importer->getMeshMaterial(mesh.get());
			if (material.get() != nullptr) {
				mesh->joinStates(material);
			}
		}

		if (useAnimation) {
			std::list<ref_ptr<AnimationNode> > meshBones;
			GLuint numBoneWeights = importer->numBoneWeights(mesh.get());
			GLuint numBones = 0u;

			// Find bones influencing this mesh
			for (auto it = nodeAnims.begin(); it != nodeAnims.end(); ++it) {
				std::list<ref_ptr<AnimationNode> > ibonNodes =
						importer->loadMeshBones(mesh.get(), it->get());
				meshBones.insert(meshBones.end(), ibonNodes.begin(), ibonNodes.end());
				numBones = ibonNodes.size();
			}

			// Create Bones state that is responsible for uploading
			// animation data to GL.
			if (!meshBones.empty()) {
				ref_ptr<Bones> bonesState = ref_ptr<Bones>::alloc(numBoneWeights, numBones);
				bonesState->setBones(meshBones);
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
		auto optional = input.getValue<GLfloat>("advance-factor");
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

ref_ptr<Particles> MeshVector::createParticleMesh(LoadingContext &ctx, scene::SceneInputNode &input, const GLuint numParticles) {
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
		particles->setMaxEmits(input.getValue<GLuint>("max-emits", 100u));
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
				configureParticleAttribute<ShaderInput1f, GLfloat>(parser, particles, *child.get());
			} else if (type == "vec2") {
				configureParticleAttribute<ShaderInput2f, Vec2f>(parser, particles, *child.get());
			} else if (type == "vec3") {
				configureParticleAttribute<ShaderInput3f, Vec3f>(parser, particles, *child.get());
			} else if (type == "vec4") {
				configureParticleAttribute<ShaderInput4f, Vec4f>(parser, particles, *child.get());
			} else if (type == "int") {
				configureParticleAttribute<ShaderInput1i, GLint>(parser, particles, *child.get());
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
	auto x = ref_ptr<MeshVector>::alloc(1);
	(*x.get())[0] = particles;
	parser->putResource<MeshVector>(input.getName(), x); // TODO check if this is needed
	processMeshChildren(ctx, input, *x.get());
	particles->end();

	return particles;
}

ref_ptr<TextureMappedText> MeshVector::createTextMesh(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<regen::Font> font =
			ctx.scene()->getResource<Font>(input.getValue("font"));
	if (font.get() == nullptr) {
		REGEN_WARN("Unable to load Font for '" << input.getDescription() << "'.");
		return {};
	}

	auto textHeight = input.getValue<GLfloat>("height", 16.0f);
	auto textColor = input.getValue<Vec4f>("text-color", Vec4f(0.97f, 0.86f, 0.77f, 0.95f));

	auto widget = ref_ptr<TextureMappedText>::alloc(font, textHeight);
	widget->set_color(textColor);
	widget->set_centerAtOrigin(input.getValue<bool>("center", true));

	if (input.hasAttribute("text")) {
		auto maxLineWidth = input.getValue<GLfloat>("max-line-width", 0.0f);
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
