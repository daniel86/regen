/*
 * mesh.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "mesh.h"
#include "regen/meshes/point.h"
#include "regen/meshes/proc-tree.h"
#include "regen/meshes/torus.h"
#include "regen/meshes/disc.h"
#include "regen/meshes/frame.h"

using namespace regen::scene;
using namespace regen;
using namespace std;

#include <regen/meshes/mesh-state.h>
#include <regen/meshes/rectangle.h>
#include <regen/meshes/box.h>
#include <regen/meshes/sphere.h>
#include <regen/meshes/texture-mapped-text.h>
#include <regen/meshes/assimp-importer.h>

#include <regen/scene/resource-manager.h>
#include <regen/scene/input-processors.h>

#define REGEN_MESH_CATEGORY "mesh"

static void processMeshChildren(
		SceneParser *parser, SceneInputNode &input, MeshVector &x) {
	ref_ptr<State> state;
	if (x.size() > 1) state = ref_ptr<State>::alloc();
	else state = x[0];

	const list<ref_ptr<SceneInputNode> > &childs = input.getChildren();
	for (auto it = childs.begin(); it != childs.end(); ++it) {
		SceneInputNode *child = it->get();
		ref_ptr<StateProcessor> processor =
				parser->getStateProcessor(child->getCategory());
		if (processor.get() == nullptr) {
			REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
			continue;
		}
		processor->processInput(parser, *child, state);
	}

	if (x.size() > 1) {
		for (auto jt = x.begin(); jt != x.end(); ++jt) {
			const ref_ptr<Mesh>& mesh = *jt;
			mesh->joinStates(state);
		}
	}
}

MeshResource::MeshResource()
		: ResourceProvider(REGEN_MESH_CATEGORY) {}

ref_ptr<MeshVector> MeshResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	const string meshType = input.getValue("type");
	auto levelOfDetail = input.getValue<GLuint>("lod", 4);
	auto scaling = input.getValue<Vec3f>("scaling", Vec3f(1.0f));
	auto texcoScaling = input.getValue<Vec2f>("texco-scaling", Vec2f(1.0f));
	auto rotation = input.getValue<Vec3f>("rotation", Vec3f(0.0f));
	bool useNormal = input.getValue<bool>("use-normal", true);
	bool useTexco = input.getValue<bool>("use-texco", true);
	bool useTangent = input.getValue<bool>("use-tangent", false);
	auto vboUsage = input.getValue<VBO::Usage>("usage", VBO::USAGE_DYNAMIC);

	ref_ptr<MeshVector> out_ = ref_ptr<MeshVector>::alloc();
	MeshVector *out = out_.get();

	// Primitives
	if (meshType == "sphere" || meshType == "half-sphere") {
		Sphere::Config meshCfg;
		meshCfg.texcoMode = input.getValue<Sphere::TexcoMode>("texco-mode", Sphere::TEXCO_MODE_UV);
		meshCfg.levelOfDetail = levelOfDetail;
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
		meshCfg.levelOfDetail = levelOfDetail;
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
		meshCfg.levelOfDetail = levelOfDetail;

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
		meshCfg.levelOfDetail = levelOfDetail;

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
		meshCfg.levelOfDetail = levelOfDetail;
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
		meshCfg.levelOfDetail = levelOfDetail;
		meshCfg.discRadius = input.getValue<GLfloat>("radius", 1.0f);

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Disc>::alloc(meshCfg);
	} else if (meshType == "cone" || meshType == "cone-closed") {
		ConeClosed::Config meshCfg;
		meshCfg.levelOfDetail = levelOfDetail;
		meshCfg.radius = input.getValue<GLfloat>("radius", 1.0f);
		meshCfg.height = input.getValue<GLfloat>("height", 1.0f);
		meshCfg.isBaseRequired = input.getValue<bool>("use-base", true);
		meshCfg.isNormalRequired = useNormal;
		meshCfg.usage = vboUsage;

		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<ConeClosed>::alloc(meshCfg);
	} else if (meshType == "cone-opened") {
		ConeOpened::Config meshCfg;
		meshCfg.levelOfDetail = levelOfDetail;
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
			(*out)[0] = createParticleMesh(parser, input, numParticles);
			return out_;
		}
	}
	else if (meshType == "point") {
		const auto numVertices = input.getValue<GLuint>("num-vertices", 1u);
		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Point>::alloc(numVertices);
	}
	else if (meshType == "asset") {
		ref_ptr<AssetImporter> importer = parser->getResources()->getAsset(parser, input.getValue("asset"));
		if (importer.get() == nullptr) {
			REGEN_WARN("Ignoring " << input.getDescription() << " with unknown Asset.");
		} else {
			out_ = createAssetMeshes(parser, input, importer);
			out = out_.get();
		}
		for (GLuint i = 0u; i < out->size(); ++i) {
			parser->putState(REGEN_STRING(input.getName() << i), (*out)[i]);
		}
	}
	else if (meshType == "proctree") {
		auto procTree = ref_ptr<ProcTree>::alloc();
		if (input.hasAttribute("seed")) {
			procTree->properties().mSeed = input.getValue<GLint>("seed", 0);
		}
		if (input.hasAttribute("segments")) {
			procTree->properties().mSegments = input.getValue<GLint>("segments", 8);
		}
		if (input.hasAttribute("levels")) {
			procTree->properties().mLevels = input.getValue<GLint>("levels", 5);
		}
		if (input.hasAttribute("v-multiplier")) {
			procTree->properties().mVMultiplier = input.getValue<GLfloat>("v-multiplier", 1.0f);
		}
		if (input.hasAttribute("branch-length")) {
			procTree->properties().mInitialBranchLength = input.getValue<GLfloat>("branch-length", 0.5f);
		}
		if (input.hasAttribute("branch-factor")) {
			procTree->properties().mBranchFactor = input.getValue<GLfloat>("branch-factor", 2.2f);
		}
		if (input.hasAttribute("drop-amount")) {
			procTree->properties().mDropAmount = input.getValue<GLfloat>("drop-amount", 0.24f);
		}
		if (input.hasAttribute("grow-amount")) {
			procTree->properties().mGrowAmount = input.getValue<GLfloat>("grow-amount", 0.044f);
		}
		if (input.hasAttribute("sweep-amount")) {
			procTree->properties().mSweepAmount = input.getValue<GLfloat>("sweep-amount", 0.0f);
		}
		if (input.hasAttribute("max-radius")) {
			procTree->properties().mMaxRadius = input.getValue<GLfloat>("max-radius", 0.096f);
		}
		if (input.hasAttribute("climb-rate")) {
			procTree->properties().mClimbRate = input.getValue<GLfloat>("climb-rate", 0.39f);
		}
		if (input.hasAttribute("trunk-kink")) {
			procTree->properties().mTrunkKink = input.getValue<GLfloat>("trunk-kink", 0.0f);
		}
		if (input.hasAttribute("tree-steps")) {
			procTree->properties().mTreeSteps = input.getValue<GLint>("tree-steps", 5);
		}
		if (input.hasAttribute("taper-rate")) {
			procTree->properties().mTaperRate = input.getValue<GLfloat>("taper-rate", 0.958f);
		}
		if (input.hasAttribute("radius-falloff-rate")) {
			procTree->properties().mRadiusFalloffRate = input.getValue<GLfloat>("radius-falloff-rate", 0.71f);
		}
		if (input.hasAttribute("twist-rate")) {
			procTree->properties().mTwistRate = input.getValue<GLfloat>("twist-rate", 2.97f);
		}
		if (input.hasAttribute("trunk-length")) {
			procTree->properties().mTrunkLength = input.getValue<GLfloat>("trunk-length", 1.95f);
		}
		if (input.hasAttribute("twig-scale")) {
			procTree->properties().mTwigScale = input.getValue<GLfloat>("twig-scale", 0.28f);
		}
		if (input.hasAttribute("falloff")) {
			auto falloff = input.getValue<Vec2f>("falloff", Vec2f(0.98f, 1.08f));
			procTree->properties().mLengthFalloffFactor = falloff.x;
			procTree->properties().mLengthFalloffPower = falloff.y;
		}
		if (input.hasAttribute("clump")) {
			auto clump = input.getValue<Vec2f>("clump", Vec2f(0.414f, 0.282f));
			procTree->properties().mClumpMax = clump.x;
			procTree->properties().mClumpMin = clump.y;
		}
		procTree->update();
		(*out) = MeshVector(2);
		(*out)[0] = procTree->trunkMesh();
		(*out)[1] = procTree->twigMesh();
	}
	else if (meshType == "text") {
		(*out) = MeshVector(1);
		(*out)[0] = createTextMesh(parser, input);
	}
	else if (meshType == "mesh") {
		const VBO::Usage vboUsage = input.getValue<VBO::Usage>("usage", VBO::USAGE_DYNAMIC);
		GLenum primitive = glenum::primitive(input.getValue<string>("primitive", "TRIANGLES"));
		(*out) = MeshVector(1);
		(*out)[0] = ref_ptr<Mesh>::alloc(primitive, vboUsage);
	} else {
		REGEN_WARN("Ignoring " << input.getDescription() << ", unknown Mesh type.");
	}

	if (out->size() == 1) {
		ref_ptr<Mesh> mesh = (*out)[0];
		parser->putState(input.getName(), mesh);
	}
	parser->getResources()->putMesh(input.getName(), out_);

	if (input.hasAttribute("primitive")) {
		GLenum primitive = glenum::primitive(input.getValue("primitive"));
		for (GLuint i = 0u; i < out->size(); ++i) {
			(*out)[i]->set_primitive(primitive);
		}
	}

	// Mesh resources can have State children
	if (!out->empty()) processMeshChildren(parser, input, *out);

	return out_;
}

ref_ptr<MeshVector> MeshResource::createAssetMeshes(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<AssetImporter> &importer) {
	const auto vboUsage = input.getValue<VBO::Usage>("usage", VBO::USAGE_DYNAMIC);
	const auto scaling = input.getValue<Vec3f>("scaling", Vec3f(1.0f));
	const auto rotation = input.getValue<Vec3f>("rotation", Vec3f(0.0f));
	const auto translation = input.getValue<Vec3f>("translation", Vec3f(0.0f));
	const auto assetIndices = input.getValue<string>("asset-indices", "*");
	bool useAnimation = input.getValue<bool>("asset-animation", false);

	ref_ptr<MeshVector> out_ = ref_ptr<MeshVector>::alloc();
	MeshVector &out = *out_.get();

	const vector<ref_ptr<NodeAnimation> > &nodeAnims = importer->getNodeAnimations();
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
	vector<string> indicesStr;
	boost::split(indicesStr, assetIndices, boost::is_any_of(","));
	vector<GLuint> indices(indicesStr.size());
	bool useAllIndices = false;
	for (GLuint i = 0u; i < indices.size(); ++i) {
		if (indicesStr[i] == "*") {
			useAllIndices = true;
			break;
		} else {
			stringstream ss(indicesStr[i]);
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
			list<ref_ptr<AnimationNode> > meshBones;
			GLuint numBoneWeights = importer->numBoneWeights(mesh.get());
			GLuint numBones = 0u;

			// Find bones influencing this mesh
			for (auto it = nodeAnims.begin(); it != nodeAnims.end(); ++it) {
				list<ref_ptr<AnimationNode> > ibonNodes =
						importer->loadMeshBones(mesh.get(), it->get());
				meshBones.insert(meshBones.end(), ibonNodes.begin(), ibonNodes.end());
				numBones = ibonNodes.size();
			}

			// Create Bones state that is responsible for uploading
			// animation data to GL.
			if (!meshBones.empty()) {
				ref_ptr<Bones> bonesState = ref_ptr<Bones>::alloc(numBoneWeights, numBones);
				bonesState->setBones(meshBones);
				mesh->joinStates(bonesState);
			}
		}
	}

	return out_;
}

ref_ptr<Particles> MeshResource::createParticleMesh(
		SceneParser *parser,
		SceneInputNode &input,
		const GLuint numParticles) {
	ref_ptr<Particles> particles;
	if (input.hasAttribute("update-shader")) {
		const auto updateShader = input.getValue("update-shader");
		particles = ref_ptr<Particles>::alloc(numParticles, updateShader);
	} else {
		particles = ref_ptr<Particles>::alloc(numParticles);
	}
	particles->begin();
	// Mesh resources can have State children
	MeshVector x = MeshVector(1);
	x[0] = particles;
	processMeshChildren(parser, input, x);
	particles->end();

	return particles;
}

ref_ptr<TextureMappedText> MeshResource::createTextMesh(
		SceneParser *parser, SceneInputNode &input) {
	ref_ptr<regen::Font> font =
			parser->getResources()->getFont(parser, input.getValue("font"));
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
			string align = input.getValue("alignment");
			if (align == "center") alignment = TextureMappedText::ALIGNMENT_CENTER;
			else if (align == "right") alignment = TextureMappedText::ALIGNMENT_RIGHT;
		}

		string val = input.getValue("text");
		wstringstream ss;
		ss << val.c_str();
		widget->set_value(ss.str(), alignment, maxLineWidth);
	}

	return widget;
}

