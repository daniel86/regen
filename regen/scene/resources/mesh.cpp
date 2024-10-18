/*
 * mesh.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "mesh.h"

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
	for (list<ref_ptr<SceneInputNode> >::const_iterator
				 it = childs.begin(); it != childs.end(); ++it) {
		SceneInputNode *child = it->get();
		ref_ptr<StateProcessor> processor =
				parser->getStateProcessor(child->getCategory());
		if (processor.get() == NULL) {
			REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
			continue;
		}
		processor->processInput(parser, *child, state);
	}

	if (x.size() > 1) {
		for (MeshVector::iterator jt = x.begin(); jt != x.end(); ++jt) {
			ref_ptr<Mesh> mesh = *jt;
			mesh->joinStates(state);
		}
	}
}

MeshResource::MeshResource()
		: ResourceProvider(REGEN_MESH_CATEGORY) {}

ref_ptr<MeshVector> MeshResource::createResource(
		SceneParser *parser, SceneInputNode &input) {
	const string meshType = input.getValue("type");
	GLuint levelOfDetail = input.getValue<GLuint>("lod", 4);
	Vec3f scaling = input.getValue<Vec3f>("scaling", Vec3f(1.0f));
	Vec2f texcoScaling = input.getValue<Vec2f>("texco-scaling", Vec2f(1.0f));
	Vec3f rotation = input.getValue<Vec3f>("rotation", Vec3f(0.0f));
	bool useNormal = input.getValue<bool>("use-normal", true);
	bool useTexco = input.getValue<bool>("use-texco", true);
	bool useTangent = input.getValue<bool>("use-tangent", false);
	VBO::Usage vboUsage = input.getValue<VBO::Usage>("usage", VBO::USAGE_DYNAMIC);

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
		const GLuint numParticles = input.getValue<GLuint>("num-vertices", 0u);
		const string updateShader = input.getValue<string>("update-shader", "");
		if (numParticles == 0u) {
			REGEN_WARN("Ignoring " << input.getDescription() << " with num-vertices=0.");
		} else if (updateShader.empty()) {
			REGEN_WARN("Ignoring " << input.getDescription() << " without update-shader.");
		} else {
			(*out) = MeshVector(1);
			(*out)[0] = createParticleMesh(parser, input, numParticles, updateShader);
			return out_;
		}
	} else if (meshType == "asset") {
		ref_ptr<AssetImporter> importer = parser->getResources()->getAsset(parser, input.getValue("asset"));
		if (importer.get() == NULL) {
			REGEN_WARN("Ignoring " << input.getDescription() << " with unknown Asset.");
		} else {
			out_ = createAssetMeshes(parser, input, importer);
			out = out_.get();
		}
		for (GLuint i = 0u; i < out->size(); ++i) {
			parser->putState(REGEN_STRING(input.getName() << i), (*out)[i]);
		}
	} else if (meshType == "text") {
		(*out) = MeshVector(1);
		(*out)[0] = createTextMesh(parser, input);
	} else if (meshType == "mesh") {
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
		parser->getResources()->putMesh(input.getName(), out_);
	}

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
	const VBO::Usage vboUsage = input.getValue<VBO::Usage>("usage", VBO::USAGE_DYNAMIC);
	const Vec3f scaling = input.getValue<Vec3f>("scaling", Vec3f(1.0f));
	const Vec3f rotation = input.getValue<Vec3f>("rotation", Vec3f(0.0f));
	const Vec3f translation = input.getValue<Vec3f>("translation", Vec3f(0.0f));
	const string assetIndices = input.getValue<string>("asset-indices", "*");
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
		if (mesh.get() == NULL) continue;

		// Join in material state.
		// Can be set to false to allow overwriting material stuff.
		if (input.getValue<bool>("asset-material", true)) {
			ref_ptr<Material> material =
					importer->getMeshMaterial(mesh.get());
			if (material.get() != NULL) {
				mesh->joinStates(material);
			}
		}

		if (useAnimation) {
			list<ref_ptr<AnimationNode> > meshBones;
			GLuint numBoneWeights = importer->numBoneWeights(mesh.get());
			GLuint numBones = 0u;

			// Find bones influencing this mesh
			for (vector<ref_ptr<NodeAnimation> >::const_iterator
						 it = nodeAnims.begin(); it != nodeAnims.end(); ++it) {
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
		const GLuint numParticles,
		const string &updateShader) {
	ref_ptr<Particles> particles = ref_ptr<Particles>::alloc(numParticles, updateShader);

	particles->gravity()->setVertex(0,
									input.getValue<Vec3f>("gravity", Vec3f(0.0, -9.81, 0.0)));
	particles->dampingFactor()->setVertex(0,
										  input.getValue<GLfloat>("damping-factor", 2.0));
	particles->noiseFactor()->setVertex(0,
										input.getValue<GLfloat>("noise-factor", 100.0));

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
	if (font.get() == NULL) {
		REGEN_WARN("Unable to load Font for '" << input.getDescription() << "'.");
		return ref_ptr<TextureMappedText>();
	}

	ref_ptr<TextureMappedText> widget = ref_ptr<TextureMappedText>::alloc(font,
																		  input.getValue<GLfloat>("height", 16.0f));
	widget->set_color(
			input.getValue<Vec4f>("text-color", Vec4f(0.97f, 0.86f, 0.77f, 0.95f)));

	if (input.hasAttribute("text")) {
		string val = input.getValue("text");
		wstringstream ss;
		ss << val.c_str();
		widget->set_value(ss.str());
	}

	return widget;
}

