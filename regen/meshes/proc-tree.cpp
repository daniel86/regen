#include "regen/meshes/proc-tree.h"

using namespace regen;

namespace regen {
	std::ostream &operator<<(std::ostream &out, const ProcTree::Preset &preset) {
		switch (preset) {
			case ProcTree::PRESET_NONE:
				return out << "none";
			case ProcTree::PRESET_FIR:
				return out << "fir";
			case ProcTree::PRESET_OAK_GREEN:
				return out << "oak-green";
			case ProcTree::PRESET_OAK_RED:
				return out << "oak-red";
			case ProcTree::PRESET_PINE:
				return out << "pine";
			case ProcTree::PRESET_OLIVE:
				return out << "olive";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, ProcTree::Preset &preset) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "none") preset = ProcTree::PRESET_NONE;
		else if (val == "fir") preset = ProcTree::PRESET_FIR;
		else if (val == "oak-green") preset = ProcTree::PRESET_OAK_GREEN;
		else if (val == "oak-red") preset = ProcTree::PRESET_OAK_RED;
		else if (val == "pine") preset = ProcTree::PRESET_PINE;
		else if (val == "olive") preset = ProcTree::PRESET_OLIVE;
		else {
			REGEN_WARN("Unknown Tree Preset '" << val << "'. Using default NONE.");
			preset = ProcTree::PRESET_NONE;
		}
		return in;
	}
}

ProcTree::ProcTree() {
	trunk.mesh = ref_ptr<Mesh>::alloc(GL_TRIANGLES, regen::VBO::USAGE_STATIC);
	trunk.indices = ref_ptr<ShaderInput1ui>::alloc("i");
	trunk.pos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	trunk.nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	trunk.tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	trunk.texco = ref_ptr<ShaderInput2f>::alloc("texco0");

	twig.mesh = ref_ptr<Mesh>::alloc(GL_TRIANGLES, regen::VBO::USAGE_STATIC);
	twig.indices = ref_ptr<ShaderInput1ui>::alloc("i");
	twig.pos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	twig.nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	twig.tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	twig.texco = ref_ptr<ShaderInput2f>::alloc("texco0");
}

ProcTree::ProcTree(Preset preset) : ProcTree() {
	loadPreset(preset);
}

ProcTree::ProcTree(scene::SceneInputNode &input) : ProcTree() {
	if (input.hasAttribute("preset")) {
		loadPreset(input.getValue<Preset>("preset", PRESET_NONE));
	}
	if (input.hasAttribute("seed")) {
		properties().mSeed = input.getValue<int>("seed", 0);
	}
	if (input.hasAttribute("segments")) {
		properties().mSegments = input.getValue<int>("segments", 8);
	}
	if (input.hasAttribute("levels")) {
		properties().mLevels = input.getValue<int>("levels", 5);
	}
	if (input.hasAttribute("v-multiplier")) {
		properties().mVMultiplier = input.getValue<float>("v-multiplier", 1.0f);
	}
	if (input.hasAttribute("branch-length")) {
		properties().mInitialBranchLength = input.getValue<float>("branch-length", 0.5f);
	}
	if (input.hasAttribute("branch-factor")) {
		properties().mBranchFactor = input.getValue<float>("branch-factor", 2.2f);
	}
	if (input.hasAttribute("drop-amount")) {
		properties().mDropAmount = input.getValue<float>("drop-amount", 0.24f);
	}
	if (input.hasAttribute("grow-amount")) {
		properties().mGrowAmount = input.getValue<float>("grow-amount", 0.044f);
	}
	if (input.hasAttribute("sweep-amount")) {
		properties().mSweepAmount = input.getValue<float>("sweep-amount", 0.0f);
	}
	if (input.hasAttribute("max-radius")) {
		properties().mMaxRadius = input.getValue<float>("max-radius", 0.096f);
	}
	if (input.hasAttribute("climb-rate")) {
		properties().mClimbRate = input.getValue<float>("climb-rate", 0.39f);
	}
	if (input.hasAttribute("trunk-kink")) {
		properties().mTrunkKink = input.getValue<float>("trunk-kink", 0.0f);
	}
	if (input.hasAttribute("tree-steps")) {
		properties().mTreeSteps = input.getValue<int>("tree-steps", 5);
	}
	if (input.hasAttribute("taper-rate")) {
		properties().mTaperRate = input.getValue<float>("taper-rate", 0.958f);
	}
	if (input.hasAttribute("radius-falloff-rate")) {
		properties().mRadiusFalloffRate = input.getValue<float>("radius-falloff-rate", 0.71f);
	}
	if (input.hasAttribute("twist-rate")) {
		properties().mTwistRate = input.getValue<float>("twist-rate", 2.97f);
	}
	if (input.hasAttribute("trunk-length")) {
		properties().mTrunkLength = input.getValue<float>("trunk-length", 1.95f);
	}
	if (input.hasAttribute("twig-scale")) {
		properties().mTwigScale = input.getValue<float>("twig-scale", 0.28f);
	}
	if (input.hasAttribute("falloff")) {
		auto falloff = input.getValue<Vec2f>("falloff", Vec2f(0.98f, 1.08f));
		properties().mLengthFalloffFactor = falloff.x;
		properties().mLengthFalloffPower = falloff.y;
	}
	if (input.hasAttribute("clump")) {
		auto clump = input.getValue<Vec2f>("clump", Vec2f(0.414f, 0.282f));
		properties().mClumpMax = clump.x;
		properties().mClumpMin = clump.y;
	}
}

void ProcTree::loadPreset(Preset preset) {
	auto &props = handle.mProperties;
	switch (preset) {
		case PRESET_NONE:
			break;
		case PRESET_FIR:
			props.mSeed = 152;
			props.mSegments = 6;
			props.mLevels = 5;
			props.mVMultiplier = 1.16;
			props.mTwigScale = 0.44;
			props.mInitialBranchLength = 0.49;
			props.mLengthFalloffFactor = 0.85;
			props.mLengthFalloffPower = 0.99;
			props.mClumpMax = 0.454;
			props.mClumpMin = 0.246;
			props.mBranchFactor = 3.2;
			props.mDropAmount = 0.09;
			props.mGrowAmount = 0.235;
			props.mSweepAmount = 0.01;
			props.mMaxRadius = 0.111;
			props.mClimbRate = 0.41;
			props.mTrunkKink = 0;
			props.mTreeSteps = 5;
			props.mTaperRate = 0.835;
			props.mRadiusFalloffRate = 0.73;
			props.mTwistRate = 2.06;
			props.mTrunkLength = 2.45;
			trunkMaterial_ = ref_ptr<Material>::alloc();
			trunkMaterial_->set_textures("materials/tree-trunk", 0);
			twigMaterial_ = ref_ptr<Material>::alloc();
			twigMaterial_->set_wrapping(GL_CLAMP_TO_EDGE);
			twigMaterial_->set_colorBlendMode(BlendMode::BLEND_MODE_SRC);
			twigMaterial_->set_textures("materials/tree-twig", "fir");
			break;
		case PRESET_OAK_GREEN:
		case PRESET_OAK_RED:
			props.mSeed = 152;
			props.mSegments = 8;
			props.mLevels = 5;
			props.mVMultiplier = 1.16;
			props.mTwigScale = 0.39;
			props.mInitialBranchLength = 0.49;
			props.mLengthFalloffFactor = 0.85;
			props.mLengthFalloffPower = 0.99;
			props.mClumpMax = 0.454;
			props.mClumpMin = 0.454;
			props.mBranchFactor = 3.2;
			props.mDropAmount = 0.09;
			props.mGrowAmount = 0.235;
			props.mSweepAmount = 0.051;
			props.mMaxRadius = 0.105;
			props.mClimbRate = 0.322;
			props.mTrunkKink = 0;
			props.mTreeSteps = 5;
			props.mTaperRate = 0.964;
			props.mRadiusFalloffRate = 0.73;
			props.mTwistRate = 1.5;
			props.mTrunkLength = 2.25;
			trunkMaterial_ = ref_ptr<Material>::alloc();
			trunkMaterial_->set_textures("materials/tree-trunk", 0);
			twigMaterial_ = ref_ptr<Material>::alloc();
			twigMaterial_->set_colorBlendMode(BlendMode::BLEND_MODE_SRC);
			twigMaterial_->set_wrapping(GL_CLAMP_TO_EDGE);
			twigMaterial_->set_textures("materials/tree-twig",
				preset == PRESET_OAK_GREEN ? "oak-green" : "oak-red");
			break;
		case PRESET_OLIVE:
			props.mSeed = 861;
			props.mSegments = 10;
			props.mLevels = 5;
			props.mVMultiplier = 0.66;
			props.mTwigScale = 0.47;
			props.mInitialBranchLength = 0.5;
			props.mLengthFalloffFactor = 0.85;
			props.mLengthFalloffPower = 0.99;
			props.mClumpMax = 0.449;
			props.mClumpMin = 0.404;
			props.mBranchFactor = 2.75;
			props.mDropAmount = 0.07;
			props.mGrowAmount = -0.005;
			props.mSweepAmount = 0.01;
			props.mMaxRadius = 0.269;
			props.mClimbRate = 0.626;
			props.mTrunkKink = 0.108;
			props.mTreeSteps = 4;
			props.mTaperRate = 0.876;
			props.mRadiusFalloffRate = 0.66;
			props.mTwistRate = 2.7;
			props.mTrunkLength = 1.55;
			trunkMaterial_ = ref_ptr<Material>::alloc();
			trunkMaterial_->set_textures("materials/tree-trunk", 0);
			twigMaterial_ = ref_ptr<Material>::alloc();
			twigMaterial_->set_colorBlendMode(BlendMode::BLEND_MODE_SRC);
			twigMaterial_->set_wrapping(GL_CLAMP_TO_EDGE);
			twigMaterial_->set_textures("materials/tree-twig", "olive");
			break;
		case PRESET_PINE:
			props.mSeed = 152;
			props.mSegments = 6;
			props.mLevels = 5;
			props.mVMultiplier = 1.16;
			props.mTwigScale = 0.44;
			props.mInitialBranchLength = 0.49;
			props.mLengthFalloffFactor = 0.85;
			props.mLengthFalloffPower = 0.99;
			props.mClumpMax = 0.454;
			props.mClumpMin = 0.246;
			props.mBranchFactor = 3.2;
			props.mDropAmount = 0.09;
			props.mGrowAmount = 0.235;
			props.mSweepAmount = 0.01;
			props.mMaxRadius = 0.111;
			props.mClimbRate = 0.41;
			props.mTrunkKink = 0;
			props.mTreeSteps = 5;
			props.mTaperRate = 0.835;
			props.mRadiusFalloffRate = 0.73;
			props.mTwistRate = 2.06;
			props.mTrunkLength = 2.45;
			trunkMaterial_ = ref_ptr<Material>::alloc();
			trunkMaterial_->set_textures("materials/tree-trunk", 0);
			twigMaterial_ = ref_ptr<Material>::alloc();
			twigMaterial_->set_colorBlendMode(BlendMode::BLEND_MODE_SRC);
			twigMaterial_->set_wrapping(GL_CLAMP_TO_EDGE);
			twigMaterial_->set_textures("materials/tree-twig", "pine");
			break;
	}
}

ref_ptr<ProcTree> ProcTree::computeMediumDetailTree() {
	auto medTree = ref_ptr<ProcTree>::alloc();
	medTree->handle.mProperties = handle.mProperties;
	auto &medProps = medTree->handle.mProperties;
	auto &highProps = handle.mProperties;
	medProps.mSegments = std::max(4, highProps.mSegments / 2);
	medProps.mLevels = std::max(2, highProps.mLevels - 1);
	medProps.mLengthFalloffFactor = highProps.mLengthFalloffFactor * 0.9f;
	medProps.mLengthFalloffPower = highProps.mLengthFalloffPower * 0.9f;
	medProps.mTwigScale = highProps.mTwigScale * 0.75f;
	return medTree;
}

ref_ptr<ProcTree> ProcTree::computeLowDetailTree() {
	auto lowTree = ref_ptr<ProcTree>::alloc();
	lowTree->handle.mProperties = handle.mProperties;
	auto &lowProps = lowTree->handle.mProperties;
	auto &highProps = handle.mProperties;
	lowProps.mSegments = std::max(2, highProps.mSegments / 4);
	lowProps.mLevels = std::max(1, highProps.mLevels - 2);
	lowProps.mLengthFalloffFactor = highProps.mLengthFalloffFactor * 0.8f;
	lowProps.mLengthFalloffPower = highProps.mLengthFalloffPower * 0.8f;
	lowProps.mTwigScale = highProps.mTwigScale * 0.5f;
	return lowTree;
}

void ProcTree::updateAttributes(TreeMesh &treeMesh,
								int numVertices, int numFaces,
								Proctree::fvec3 *vertices,
								Proctree::fvec3 *normals,
								Proctree::fvec2 *uvs,
								Proctree::ivec3 *faces) {
	auto numIndices = numFaces * 3;
	treeMesh.indices->setVertexData(numIndices, reinterpret_cast<const unsigned char *>(&faces[0].x));
	treeMesh.pos->setVertexData(numVertices, reinterpret_cast<const unsigned char *>(&vertices[0].x));
	treeMesh.nor->setVertexData(numVertices, reinterpret_cast<const unsigned char *>(&normals[0].x));
	treeMesh.texco->setVertexData(numVertices, reinterpret_cast<const unsigned char *>(&uvs[0].u));
	treeMesh.tan->setVertexData(numVertices);

	for (int i = 0; i < numFaces; i++) {
		auto &v0 = *((Vec3f *) &vertices[faces[i].x]);
		auto &v1 = *((Vec3f *) &vertices[faces[i].y]);
		auto &v2 = *((Vec3f *) &vertices[faces[i].z]);
		auto &uv0 = *((Vec2f *) &uvs[faces[i].x]);
		auto &uv1 = *((Vec2f *) &uvs[faces[i].y]);
		auto &uv2 = *((Vec2f *) &uvs[faces[i].z]);
		auto e1 = v1 - v0;
		auto e2 = v2 - v0;
		auto deltaUV1 = uv1 - uv0;
		auto deltaUV2 = uv2 - uv0;
		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
		Vec3f tangent3 = (e1 * deltaUV2.y - e2 * deltaUV1.y) * f;
		Vec4f tangent(tangent3.x, tangent3.y, tangent3.z, 1.0f);
		treeMesh.tan->setVertex(faces[i].x, tangent);
		treeMesh.tan->setVertex(faces[i].y, tangent);
		treeMesh.tan->setVertex(faces[i].z, tangent);
	}

	treeMesh.mesh->begin(ShaderInputContainer::INTERLEAVED);
	treeMesh.mesh->setIndices(treeMesh.indices, numVertices);
	treeMesh.mesh->setInput(treeMesh.pos);
	treeMesh.mesh->setInput(treeMesh.nor);
	treeMesh.mesh->setInput(treeMesh.tan);
	treeMesh.mesh->setInput(treeMesh.texco);
	treeMesh.mesh->end();

	Vec3f min = *((Vec3f *) &vertices[0]);
	Vec3f max = *((Vec3f *) &vertices[0]);
	for (int i = 1; i < numVertices; i++) {
		auto &v = *((Vec3f *) &vertices[i]);
		min.x = std::min(min.x, v.x);
		min.y = std::min(min.y, v.y);
		min.z = std::min(min.z, v.z);
		max.x = std::max(max.x, v.x);
		max.y = std::max(max.y, v.y);
		max.z = std::max(max.z, v.z);
	}
	treeMesh.mesh->set_bounds(min, max);
}

void ProcTree::updateTrunkAttributes() {
	updateAttributes(trunk, handle.mVertCount, handle.mFaceCount,
					 handle.mVert, handle.mNormal, handle.mUV, handle.mFace);
}

void ProcTree::updateTwigAttributes() {
	updateAttributes(twig, handle.mTwigVertCount, handle.mTwigFaceCount,
					 handle.mTwigVert, handle.mTwigNormal, handle.mTwigUV, handle.mTwigFace);
}

void ProcTree::update(const std::vector<GLuint> &lodLevels) {
	// TODO: handle LOD levels
	handle.generate();
	// update trunk
	updateTrunkAttributes();
	if (trunkMaterial_.get() != nullptr) {
		trunk.mesh->joinStates(trunkMaterial_);
	}
	// update twig
	updateTwigAttributes();
	if (twigMaterial_.get() != nullptr) {
		twig.mesh->joinStates(twigMaterial_);
	}
}
