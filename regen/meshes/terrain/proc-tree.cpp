#include "regen/meshes/terrain/proc-tree.h"

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
	trunk.mesh = ref_ptr<Mesh>::alloc(GL_TRIANGLES, BufferUpdateFlags::NEVER);
	trunk.indices = ref_ptr<ShaderInput1ui>::alloc("i");
	trunk.pos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	trunk.nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	trunk.tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	trunk.texco = ref_ptr<ShaderInput2f>::alloc("texco0");

	twig.mesh = ref_ptr<Mesh>::alloc(GL_TRIANGLES, BufferUpdateFlags::NEVER);
	twig.indices = ref_ptr<ShaderInput1ui>::alloc("i");
	twig.pos = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	twig.basePos = ref_ptr<ShaderInput3f>::alloc("basePos");
	twig.nor = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	twig.tan = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	twig.texco = ref_ptr<ShaderInput2f>::alloc("texco0");
}

ProcTree::ProcTree(Preset preset) : ProcTree() {
	loadPreset(preset);
}

ProcTree::ProcTree(scene::SceneInputNode &input) : ProcTree() {
	if (input.hasAttribute("tile-counts")) {
		auto lodVec = input.getValue<Vec4ui>(
				"tile-counts", Vec4ui(8, 8, 8, 8));
		silhouetteCfg_.silhouette.tileCounts.resize(4);
		silhouetteCfg_.silhouette.tileCounts[0] = lodVec.x;
		silhouetteCfg_.silhouette.tileCounts[1] = lodVec.y;
		silhouetteCfg_.silhouette.tileCounts[2] = lodVec.z;
		silhouetteCfg_.silhouette.tileCounts[3] = lodVec.w;
	} else {
		silhouetteCfg_.silhouette.tileCounts.push_back(input.getValue<GLuint>("lod", 8));
	}
	if (input.hasAttribute("alpha-cut")) {
		silhouetteCfg_.silhouette.alphaCut = input.getValue<float>("alpha-cut", 0.01f);
	}
	if (input.hasAttribute("coverage-threshold")) {
		silhouetteCfg_.silhouette.coverageThreshold = input.getValue<float>("coverage-threshold", 0.05f);
	}
	if (input.hasAttribute("pad-tiles")) {
		silhouetteCfg_.silhouette.padTiles = input.getValue<uint32_t>("pad-tiles", 1u);
	}
	if (input.hasAttribute("max-quads-per-sprite")) {
		silhouetteCfg_.silhouette.maxQuadsPerSprite = input.getValue<uint32_t>("max-quads-per-sprite", 64u);
	}
	if (input.hasAttribute("texco-scale")) {
		silhouetteCfg_.texcoScale = input.getValue<Vec2f>("texco-scale", Vec2f(1.0f));
	}

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

	trunkMaterial_ = ref_ptr<Material>::alloc();
	trunkMaterial_->set_colorBlendMode(BlendMode::BLEND_MODE_MULTIPLY);
	trunkMaterial_->set_useMipmaps(true);

	twigMaterial_ = ref_ptr<Material>::alloc();
	twigMaterial_->set_colorBlendMode(BlendMode::BLEND_MODE_MULTIPLY);
	twigMaterial_->set_wrapping(GL_CLAMP_TO_EDGE);
	twigMaterial_->set_useMipmaps(true);

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
			trunkMaterial_->set_textures("materials/tree-trunk", "0");
			twigMaterial_->set_textures("materials/tree-twig", "fir");
			twigMaterial_->diffuse()->setVertex(0, Vec3f(0.7f, 0.8f, 0.7f));
			//twigMaterial_->diffuse()->setUniformData(Vec3f(1.7f, 1.8f, 1.7f));
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
			trunkMaterial_->set_textures("materials/tree-trunk", "0");
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
			trunkMaterial_->set_textures("materials/tree-trunk", "0");
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
			trunkMaterial_->set_textures("materials/tree-trunk", "0");
			twigMaterial_->set_textures("materials/tree-twig", "pine");
			break;
	}

	if (useSilhouetteMesh_) {
		// obtain twig diffuse texture, we want to compute silhouette mesh from that
		auto twigTex = twigMaterial_->getColorTexture();
		if (twigTex.get()) {
			auto twigTex2D = ref_ptr<Texture2D>::dynamicCast(twigTex);
			if (twigTex2D.get()) {
				twigSilhouette_ = ref_ptr<SilhouetteMesh>::alloc(twigTex2D, silhouetteCfg_);
				twigSilhouette_->updateSilhouette();
			} else {
				REGEN_WARN("Cannot create silhouette mesh for twig texture.");
			}
		}
	} else {
		twigSilhouette_ = {};
	}
}

ref_ptr<Proctree::Tree> ProcTree::computeMediumDetailTree() {
	auto medTree = ref_ptr<Proctree::Tree>::alloc();
	medTree->mProperties = handle.mProperties;
	auto &medProps = medTree->mProperties;
	auto &highProps = handle.mProperties;
	medProps.mTwigScale = highProps.mTwigScale * 1.25f;
	// reduce number of small branches
	medProps.mLevels = std::max(2, highProps.mLevels - 2);
	return medTree;
}

ref_ptr<Proctree::Tree> ProcTree::computeLowDetailTree() {
	auto lowTree = ref_ptr<Proctree::Tree>::alloc();
	lowTree->mProperties = handle.mProperties;
	auto &lowProps = lowTree->mProperties;
	auto &highProps = handle.mProperties;
	lowProps.mTwigScale = highProps.mTwigScale * 1.75f;
	// reduce number of small branches
	lowProps.mLevels = std::max(1, highProps.mLevels - 5);
	return lowTree;
}

void ProcTree::computeTan(TreeMesh &treeMesh, const ProcMesh &procMesh, int vertexOffset, Vec4f *tanData) {
	for (int i = 0; i < procMesh.mFaceCount; i++) {
		auto &v0 = *((Vec3f *) &procMesh.mVert[procMesh.mFace[i].x]);
		auto &v1 = *((Vec3f *) &procMesh.mVert[procMesh.mFace[i].y]);
		auto &v2 = *((Vec3f *) &procMesh.mVert[procMesh.mFace[i].z]);
		auto &uv0 = *((Vec2f *) &procMesh.mUV[procMesh.mFace[i].x]);
		auto &uv1 = *((Vec2f *) &procMesh.mUV[procMesh.mFace[i].y]);
		auto &uv2 = *((Vec2f *) &procMesh.mUV[procMesh.mFace[i].z]);
		auto e1 = v1 - v0;
		auto e2 = v2 - v0;
		auto deltaUV1 = uv1 - uv0;
		auto deltaUV2 = uv2 - uv0;
		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
		Vec3f tangent3 = (e1 * deltaUV2.y - e2 * deltaUV1.y) * f;
		Vec4f tangent(tangent3.x, tangent3.y, tangent3.z, 1.0f);
		tanData[vertexOffset + procMesh.mFace[i].x] = tangent;
		tanData[vertexOffset + procMesh.mFace[i].y] = tangent;
		tanData[vertexOffset + procMesh.mFace[i].z] = tangent;
	}
}

static int addQuadPoint(Vec3f *quadPos, const Vec3f &pos, const Vec2f &uv) {
	int quadIndex;
	if (uv.y > 0.5 && uv.x < 0.5) {
		quadIndex = 0;
	} else if (uv.y > 0.5 && uv.x > 0.5) {
		quadIndex = 2;
	} else if (uv.x < 0.5) {
		quadIndex = 1;
	} else {
		quadIndex = 3;
	}
	quadPos[quadIndex] = pos;
	return quadIndex;
}

void ProcTree::computeBasePos(TreeMesh &treeMesh, const ProcMesh &procMesh, int vertexOffset, Vec3f *basePosData) {
	for (int i = 0; i < procMesh.mFaceCount; i++) {
		auto &v0 = *((Vec3f *) &procMesh.mVert[procMesh.mFace[i].x]);
		auto &v1 = *((Vec3f *) &procMesh.mVert[procMesh.mFace[i].y]);
		auto &v2 = *((Vec3f *) &procMesh.mVert[procMesh.mFace[i].z]);
		auto &uv0 = *((Vec2f *) &procMesh.mUV[procMesh.mFace[i].x]);
		auto &uv1 = *((Vec2f *) &procMesh.mUV[procMesh.mFace[i].y]);
		auto &uv2 = *((Vec2f *) &procMesh.mUV[procMesh.mFace[i].z]);

		Vec3f quadPos[4] = {Vec3f::zero(), Vec3f::zero(),
							Vec3f::zero(), Vec3f::zero()};
		int vIndex0 = addQuadPoint(quadPos, v0, uv0);
		int vIndex1 = addQuadPoint(quadPos, v1, uv1);
		int vIndex2 = addQuadPoint(quadPos, v2, uv2);
		int missingIndex = 6 - vIndex0 - vIndex1 - vIndex2;
		if (missingIndex == 0) {
			quadPos[0] = quadPos[1] + quadPos[2] - quadPos[3];
		} else if (missingIndex == 1) {
			quadPos[1] = quadPos[0] + quadPos[3] - quadPos[2];
		} else if (missingIndex == 2) {
			quadPos[2] = quadPos[3] + quadPos[0] - quadPos[1];
		} else {
			quadPos[3] = quadPos[2] + quadPos[1] - quadPos[0];
		}
		// TODO: why not use the bottom center as base point for everybody?
		// Vec3f bottomCenter = (quadPos[0] + quadPos[2])*0.5f;
		// first base point
		if (vIndex0 == 0 || vIndex0 == 2) {
			basePosData[vertexOffset + procMesh.mFace[i].x] = v0;
		} else if (vIndex0 == 1) { // quadPos[0] is base point
			basePosData[vertexOffset + procMesh.mFace[i].x] = quadPos[0];
		} else { // quadPos[2] is base point
			basePosData[vertexOffset + procMesh.mFace[i].x] = quadPos[2];
		}
		// second base point
		if (vIndex1 == 0 || vIndex1 == 2) {
			basePosData[vertexOffset + procMesh.mFace[i].y] = v1;
		} else if (vIndex1 == 1) { // quadPos[0] is base point
			basePosData[vertexOffset + procMesh.mFace[i].y] = quadPos[0];
		} else { // quadPos[2] is base point
			basePosData[vertexOffset + procMesh.mFace[i].y] = quadPos[2];
		}
		// third base point
		if (vIndex2 == 0 || vIndex2 == 2) {
			basePosData[vertexOffset + procMesh.mFace[i].z] = v2;
		} else if (vIndex2 == 1) { // quadPos[0] is base point
			basePosData[vertexOffset + procMesh.mFace[i].z] = quadPos[0];
		} else { // quadPos[2] is base point
			basePosData[vertexOffset + procMesh.mFace[i].z] = quadPos[2];
		}
	}
}

void ProcTree::updateAttributes_(
		TreeMesh &treeMesh,
		const ProcMesh &lod0,
		const std::vector<Mesh::MeshLOD> &lodLevels,
		uint32_t maxVIndex) {
	treeMesh.mesh->begin(Mesh::INTERLEAVED);
	auto indexRef = treeMesh.mesh->setIndices(treeMesh.indices, maxVIndex);
	treeMesh.mesh->setInput(treeMesh.pos);
	if (treeMesh.basePos.get()) {
		treeMesh.mesh->setInput(treeMesh.basePos);
	}
	treeMesh.mesh->setInput(treeMesh.nor);
	treeMesh.mesh->setInput(treeMesh.tan);
	treeMesh.mesh->setInput(treeMesh.texco);
	treeMesh.mesh->end();

	Vec3f min = *((Vec3f *) &lod0.mVert[0]);
	Vec3f max = *((Vec3f *) &lod0.mVert[0]);
	// find bounding box of LOD=0
	for (int i = 1; i < lod0.mVertCount; i++) {
		auto &v = *((Vec3f *) &lod0.mVert[i]);
		min.x = std::min(min.x, v.x);
		min.y = std::min(min.y, v.y);
		min.z = std::min(min.z, v.z);
		max.x = std::max(max.x, v.x);
		max.y = std::max(max.y, v.y);
		max.z = std::max(max.z, v.z);
	}
	treeMesh.mesh->set_bounds(min, max);

	if (!lodLevels.empty()) {
		for (auto &x: lodLevels) {
			// add the index buffer offset (in number of bytes)
			x.d->indexOffset = indexRef->address() + x.d->indexOffset * sizeof(GLuint);
		}
		treeMesh.mesh->setMeshLODs(lodLevels);
		treeMesh.mesh->activateLOD(0);
	}
}

void ProcTree::updateTrunkAttributes(TreeMesh &treeMesh, const std::vector<ProcMesh> &procLODs) const {
	std::vector<Mesh::MeshLOD> lodLevels;
	auto &lod0 = procLODs[0];
	int numVertices = 0, numIndices = 0;
	for (auto &lod: procLODs) {
		numVertices += lod.mVertCount;
		numIndices += lod.mFaceCount * 3;
	}

	// allocate memory then copy each LOD into the vertex data array
	treeMesh.indices->setVertexData(numIndices);
	treeMesh.pos->setVertexData(numVertices);
	treeMesh.nor->setVertexData(numVertices);
	treeMesh.texco->setVertexData(numVertices);
	treeMesh.tan->setVertexData(numVertices);
	// map client data for writing
	auto indices = treeMesh.indices->mapClientData<unsigned int>(BUFFER_GPU_WRITE);
	auto v_pos = treeMesh.pos->mapClientData<float>(BUFFER_GPU_WRITE);
	auto v_nor = treeMesh.nor->mapClientData<float>(BUFFER_GPU_WRITE);
	auto v_tan = treeMesh.tan->mapClientData<float>(BUFFER_GPU_WRITE);
	auto v_texco = treeMesh.texco->mapClientData<float>(BUFFER_GPU_WRITE);
	auto *ptr_indices = indices.w.data();
	auto *ptr_pos = v_pos.w.data();
	auto *ptr_nor = v_nor.w.data();
	auto *ptr_texco = v_texco.w.data();
	// copy data from Proctree to Mesh
	// also create LOD descriptions on the way.
	unsigned int vertexOffset = 0u, indexOffset = 0u;
	for (auto &lod: procLODs) {
		// create LOD description
		auto &lodLevel = lodLevels.emplace_back();
		lodLevel.d->numVertices = lod.mVertCount;
		lodLevel.d->numIndices = lod.mFaceCount * 3;
		lodLevel.d->vertexOffset = vertexOffset;
		lodLevel.d->indexOffset = indexOffset;
		// copy data
		for (int i = 0; i < lod.mFaceCount; i++) {
			ptr_indices[i * 3 + 0] = lod.mFace[i].x + vertexOffset;
			ptr_indices[i * 3 + 1] = lod.mFace[i].y + vertexOffset;
			ptr_indices[i * 3 + 2] = lod.mFace[i].z + vertexOffset;
		}
		memcpy(ptr_pos, &lod.mVert[0].x, lod.mVertCount * 3 * sizeof(float));
		memcpy(ptr_nor, &lod.mNormal[0].x, lod.mVertCount * 3 * sizeof(float));
		memcpy(ptr_texco, &lod.mUV[0].u, lod.mVertCount * 2 * sizeof(float));
		// compute tangents
		computeTan(treeMesh, lod, vertexOffset, (Vec4f *) v_tan.w.data());
		// increase offsets
		vertexOffset += lodLevel.d->numVertices;
		indexOffset += lodLevel.d->numIndices;
		ptr_indices += lodLevel.d->numIndices;
		ptr_pos += lod.mVertCount * 3;
		ptr_nor += lod.mVertCount * 3;
		ptr_texco += lod.mVertCount * 2;
	}

	updateAttributes_(treeMesh, lod0, lodLevels, numVertices);
}

ProcTree::ProcMesh ProcTree::trunkProcMesh(Proctree::Tree &x) {
	return {
			x.mVertCount,
			x.mFaceCount,
			x.mVert,
			x.mNormal,
			x.mUV,
			x.mFace
	};
}

ProcTree::ProcMesh ProcTree::twigProcMesh(Proctree::Tree &x) {
	return {
			x.mTwigVertCount,
			x.mTwigFaceCount,
			x.mTwigVert,
			x.mTwigNormal,
			x.mTwigUV,
			x.mTwigFace
	};
}

void ProcTree::updateTrunkAttributes() {
	if (useLODs_) {
		updateTrunkAttributes(trunk, {
				trunkProcMesh(handle),
				trunkProcMesh(*lodMedium_.get()),
				trunkProcMesh(*lodLow_.get())
		});
	} else {
		updateTrunkAttributes(trunk, {trunkProcMesh(handle)});
	}
}

void ProcTree::updateTwigAttributes() {
	const ProcMesh lod0 = twigProcMesh(handle);
	std::vector<Mesh::MeshLOD> lodLevels;

	if (twigSilhouette_.get()) {
		// FIXME: There are some bugs in this block of code that needs to be fixed!
		auto &uvRects = twigSilhouette_->silhouetteUVRects()[0];
		uint32_t nq_in = lod0.mFaceCount / 2;
		uint32_t nv_in = lod0.mVertCount;
		uint32_t ni_in = lod0.mFaceCount * 3;
		uint32_t nq_out = nq_in * uvRects.size();
		uint32_t nv_out = nq_out * 4; // 4 vertices per quad
		uint32_t ni_out = nq_out * 6; // 6 indices per quad

		// Allocate client memory
		twig.indices->setVertexData(ni_out);
		twig.pos->setVertexData(nv_out);
		twig.pos->setVertexData(nv_out);
		twig.nor->setVertexData(nv_out);
		twig.texco->setVertexData(nv_out);
		twig.tan->setVertexData(nv_out);
		if (twig.basePos.get()) {
			twig.basePos->setVertexData(nv_out);
		}
		// map client data for writing
		auto twig_i = (GLuint*)twig.indices->clientBuffer()->clientData(0);
		auto twig_p = (Vec3f*) twig.pos->clientBuffer()->clientData(0);
		auto twig_n = (Vec3f*) twig.nor->clientBuffer()->clientData(0);
		auto twig_t = (Vec4f*) twig.tan->clientBuffer()->clientData(0);
		auto twig_uv = (Vec2f*) twig.texco->clientBuffer()->clientData(0);
		auto twig_bp = (twig.basePos.get() ?
			(Vec3f*) twig.basePos->clientBuffer()->clientData(0) : nullptr);

		uint32_t qOffset = 0u;
		uint32_t vOffset = 0u;
		uint32_t iOffset = 0u;

		for (uint32_t quadIdx_in=0u; quadIdx_in < nq_in; quadIdx_in++) {
			// For each input quad, we create uvRects.size() output quads.
			// For this we need to compute tangents along u/v directions.
			auto &face_in = lod0.mFace[quadIdx_in * 2];
			auto &v0_in = *((Vec3f*)&lod0.mVert[face_in.x].x);
			auto &v1_in = *((Vec3f*)&lod0.mVert[face_in.y].x);
			auto &v2_in = *((Vec3f*)&lod0.mVert[face_in.z].x);
			// base position for the quad
			// TODO: UNSURE ABOUT IT
			auto basePos = (v1_in + v2_in) * 0.5f;
			// compute direction vectors along face plane
			// TODO: UNSURE ABOUT IT
			// length of the vectors should be width/height of the quad.
			Vec3f e2 = v0_in - v1_in; // "v" axis
			Vec3f e1 = v2_in - v1_in; // "u" axis
			// Orthonormal-ish data for TBN (you can keep e1/e2 scaled for positioning)
			Vec3f n = e1.cross(e2); n.normalize();
			// tangent = u-axis
			Vec3f t = e1; t.normalize();
			// Handedness: sign = +1 if (t x b)·n > 0; here we approximate with e2
			Vec3f e2_norm = e2; e2_norm.normalize();
			float handedness = t.cross(e2_norm).dot(n) < 0.0f ? -1.0f : 1.0f;

			for (uint32_t uvRectIdx = 0u; uvRectIdx < uvRects.size(); uvRectIdx++) {
				// For each output quad, we need to compute the position and UVs.
				auto &uvRect = uvRects[uvRectIdx];
				// uvRect = (u0, v0, u1, v1) in normalized 0..1 space
				float u0 = uvRect.x, v0 = uvRect.y, u1 = uvRect.z, v1 = uvRect.w;
				// use (0..1) local coords around center:
				Vec2f local0( (u0+u1)*0.5f, (v0+v1)*0.5f );
				Vec2f localSize( u1 - u0, v1 - v0 );
				float hw = localSize.x * 0.5f;
				float hh = localSize.y * 0.5f;
				// We'll create geometry in UV-space [0..1]
				Vec2f q0(local0.x - hw, local0.y - hh);
				Vec2f q1(local0.x + hw, local0.y - hh);
				Vec2f q2(local0.x + hw, local0.y + hh);
				Vec2f q3(local0.x - hw, local0.y + hh);

				// compute positions in world space
				twig_p[vOffset + 0] = v1_in + e1 * q0.x + e2 * q0.y;
				twig_p[vOffset + 1] = v1_in + e1 * q1.x + e2 * q1.y;
				twig_p[vOffset + 2] = v1_in + e1 * q2.x + e2 * q2.y;
				twig_p[vOffset + 3] = v1_in + e1 * q3.x + e2 * q3.y;

				// compute UVs in [0..1] space
				twig_uv[vOffset + 0] = Vec2f(u0, v0);
				twig_uv[vOffset + 1] = Vec2f(u1, v0);
				twig_uv[vOffset + 2] = Vec2f(u1, v1);
				twig_uv[vOffset + 3] = Vec2f(u0, v1);

				// compute normals
				twig_n[vOffset + 0] = n;
				twig_n[vOffset + 1] = n;
				twig_n[vOffset + 2] = n;
				twig_n[vOffset + 3] = n;

				// compute tangents
				twig_t[vOffset + 0] = Vec4f(t, handedness);
				twig_t[vOffset + 1] = Vec4f(t, handedness);
				twig_t[vOffset + 2] = Vec4f(t, handedness);
				twig_t[vOffset + 3] = Vec4f(t, handedness);

				// set the base position for the quad.
				if (twig.basePos.get()) {
					twig_bp[vOffset + 0] = basePos;
					twig_bp[vOffset + 1] = basePos;
					twig_bp[vOffset + 2] = basePos;
					twig_bp[vOffset + 3] = basePos;
				}

				// Finally, set the indices for the quad.
				twig_i[iOffset + 0] = vOffset + 0;
				twig_i[iOffset + 1] = vOffset + 1;
				twig_i[iOffset + 2] = vOffset + 2;
				twig_i[iOffset + 3] = vOffset + 2;
				twig_i[iOffset + 4] = vOffset + 3;
				twig_i[iOffset + 5] = vOffset + 0;
				// increment offsets
				vOffset += 4u;
				iOffset += 6u;
				qOffset += 1u;
			}
		}

		updateAttributes_(twig, lod0, lodLevels, nv_out);
	} else {
		uint32_t nv = lod0.mVertCount;
		uint32_t ni = lod0.mFaceCount * 3;
		// use proc tree data directly
#define PROC_DATA_PTR_(arg) reinterpret_cast<const unsigned char *>(&(arg))
		twig.indices->setVertexData(ni, PROC_DATA_PTR_(lod0.mFace[0].x));
		twig.pos->setVertexData(nv, PROC_DATA_PTR_(lod0.mVert[0].x));
		twig.nor->setVertexData(nv, PROC_DATA_PTR_(lod0.mNormal[0].x));
		twig.texco->setVertexData(nv, PROC_DATA_PTR_(lod0.mUV[0].u));
#undef PROC_DATA_PTR_

		twig.tan->setVertexData(nv);
		auto v_tan = twig.tan->mapClientData<float>(BUFFER_GPU_WRITE);
		computeTan(twig, lod0, 0, (Vec4f *) v_tan.w.data());
		v_tan.unmap();

		if (twig.basePos.get()) {
			twig.basePos->setVertexData(nv);
			auto v_basePos = twig.basePos->mapClientData<float>(BUFFER_GPU_WRITE);
			computeBasePos(twig, lod0, 0, (Vec3f *) v_basePos.w.data());
			v_basePos.unmap();
		}

		updateAttributes_(twig, lod0, lodLevels, nv);
	}
}

void ProcTree::update() {
	handle.generate();
	if (useLODs_) {
		if (lodMedium_.get() == nullptr) {
			lodMedium_ = computeMediumDetailTree();
		}
		if (lodLow_.get() == nullptr) {
			lodLow_ = computeLowDetailTree();
		}
		lodMedium_->generate();
		lodLow_->generate();
	}
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
	if (useLODs_) {
		lodMedium_ = {};
		lodLow_ = {};
	}
}
