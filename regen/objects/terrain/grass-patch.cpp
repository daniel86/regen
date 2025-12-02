#include "grass-patch.h"
#include "regen/textures/texture-loader.h"
#include "regen/objects/composite-mesh.h"
#include "regen/gl/states/alpha-state.h"

using namespace regen;

GrassPatch::GrassPatch(const ref_ptr<ModelTransformation> &tf, const BufferUpdateFlags &updateHint) :
		Mesh(GL_TRIANGLES, updateHint),
		tf_(tf) {
	shaderDefine("VERTEX_MASK_INDEX", "0");
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	basePos_ = ref_ptr<ShaderInput3f>::alloc("basePos");
	joinStates(tf);
	joinStates(ref_ptr<ToggleState>::alloc(RenderState::CLIP_DISTANCE0, true));
}

void GrassPatch::setMask(
		const ref_ptr<Texture2D> &maskTexture, uint32_t maskIndex,
		const MaskMesh::Config &patchConfig) {
	shaderDefine("VERTEX_MASK_INDEX", REGEN_STRING(maskIndex));
	setBufferMapMode(patchConfig.quad.mapMode);
	setClientAccessMode(patchConfig.quad.accessMode);
	maskMesh_ = ref_ptr<MaskMesh>::alloc(tf_, maskTexture, maskIndex, patchConfig);
	if (maskTexture.get()) {
		joinStates(maskMesh_->maskTextureState());
	}
}

void GrassPatch::setSilhouette(
		const ref_ptr<Texture2D> &silhouetteTexture,
		const SilhouetteMesh::Config &silhouetteConfig) {
	silhouetteMesh_ = ref_ptr<SilhouetteMesh>::alloc(silhouetteTexture, silhouetteConfig);
}

void GrassPatch::updateSilhouette() {
	if (silhouetteMesh_.get()) {
		silhouetteMesh_->updateSilhouette();
	}
}

void GrassPatch::updateTransforms() {
	if (maskMesh_.get()) {
		maskMesh_->updateMask();
	}
}

void GrassPatch::generateLODLevel(uint32_t lodLevel) {
	auto &grassLOD = meshLODs_[lodLevel];
	// map client data for writing
	auto grass_p  = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto grass_base = (Vec3f*) basePos_->clientBuffer()->clientData(0);
	auto grass_i = (byte*)indices_->clientBuffer()->clientData(0);
	auto indexType = indices_->baseType();
	// offsets into data arrays
	grass_p += grassLOD.vertexOffset();
	grass_base += grassLOD.vertexOffset();
	grass_i += grassLOD.indexOffset() * indices_->dataTypeBytes();
	const uint32_t vBaseOffset = grassLOD.vertexOffset();
	const auto &uvRects = silhouetteMesh_->silhouetteUVRects()[lodLevel];
	const uint32_t numQuadsPerVertex = uvRects.size();

	auto &maskLOD = maskMesh_->meshLODs()[lodLevel];
	auto mask_p = maskMesh_->pos()->mapClientData<Vec3f>(BUFFER_GPU_READ);
	const uint32_t maskOffset = maskLOD.vertexOffset();

	Vec2f grassQuadSize = Vec2f::one();
	if (lodLevel > 1) {
		// HACK: increase quad size for higher LODs where we use less vertices.
		//   That will make the grass look more dense.
		grassQuadSize.x *= static_cast<float>(lodLevel);
	}

	uint32_t vOffset = 0u;
	uint32_t iOffset = 0u;
	for (uint32_t objIdx = 0; objIdx < maskLOD.numVertices(); ++objIdx) {
		// The base position is centered at the bottom of the quad.
		const Vec3f &basePos = mask_p.r[maskOffset + objIdx];

		for (uint32_t objQuadIdx = 0; objQuadIdx < numQuadsPerVertex; ++objQuadIdx) {
			const auto &uvRect = uvRects[objQuadIdx];
			// uvRect = (u0, v0, u1, v1) in normalized 0..1 space
			float u0 = uvRect.x, v0 = uvRect.y, u1 = uvRect.z, v1 = uvRect.w;

			// use (0..1) local coords around center:
			Vec2f local0( (u0+u1)*0.5f - 0.5f, (v0+v1)*0.5f - 0.5f );
			Vec2f localSize( u1 - u0, v1 - v0 );

			// We'll create geometry in UV-space scaled to [-0.5..0.5] so that instancing can position it in world.
			Vec3f p0(local0.x - localSize.x*0.5f, local0.y - localSize.y*0.5f + 0.5f, 0.0f);
			Vec3f p1(local0.x + localSize.x*0.5f, local0.y - localSize.y*0.5f + 0.5f, 0.0f);
			Vec3f p2(local0.x + localSize.x*0.5f, local0.y + localSize.y*0.5f + 0.5f, 0.0f);
			Vec3f p3(local0.x - localSize.x*0.5f, local0.y + localSize.y*0.5f + 0.5f, 0.0f);
			p0.xy() *= grassQuadSize;
			p1.xy() *= grassQuadSize;
			p2.xy() *= grassQuadSize;
			p3.xy() *= grassQuadSize;

			// Start with the 4 vertices of the quad.
			grass_p[vOffset + 0] = basePos + p0;
			grass_p[vOffset + 1] = basePos + p1;
			grass_p[vOffset + 2] = basePos + p2;
			grass_p[vOffset + 3] = basePos + p3;
			for (uint32_t i=0; i<4; i++) {
				minPosition_.setMin(grass_p[vOffset + i]);
				maxPosition_.setMax(grass_p[vOffset + i]);
			}
			// Also set the base position for the grass quad.
			grass_base[vOffset + 0] = basePos;
			grass_base[vOffset + 1] = basePos;
			grass_base[vOffset + 2] = basePos;
			grass_base[vOffset + 3] = basePos;
			// Finally, set the indices for the quad.
			setIndexValue(grass_i, indexType, iOffset + 0, vBaseOffset + vOffset + 0);
			setIndexValue(grass_i, indexType, iOffset + 1, vBaseOffset + vOffset + 1);
			setIndexValue(grass_i, indexType, iOffset + 2, vBaseOffset + vOffset + 2);
			setIndexValue(grass_i, indexType, iOffset + 3, vBaseOffset + vOffset + 2);
			setIndexValue(grass_i, indexType, iOffset + 4, vBaseOffset + vOffset + 3);
			setIndexValue(grass_i, indexType, iOffset + 5, vBaseOffset + vOffset + 0);
			// Move to the next quad's vertices
			vOffset += 4;
			iOffset += 6;
		}
	}
}

void GrassPatch::updateAttributes() {
	// Extrude each vertex either into (1) a quad or (2) a cross of 3 quads.
	// Idea: only use cross for lod=0, else use a single quad.
	maskMesh_->updateAttributes();

	// count vertices and indices, and create LOD descriptions
	uint32_t numVertices = 0u;
	uint32_t numIndices = 0u;
	for (uint32_t lodIdx = 0u; lodIdx < maskMesh_->meshLODs().size(); ++lodIdx) {
		auto &maskLOD = maskMesh_->meshLODs()[lodIdx];
		const auto &uvRects = silhouetteMesh_->silhouetteUVRects()[lodIdx];
		const uint32_t numQuadsPerVertex = uvRects.size();
		auto &x = meshLODs_.emplace_back();
		x.d->numVertices = maskLOD.d->numVertices * numQuadsPerVertex * 4;
		x.d->numIndices = maskLOD.d->numVertices * numQuadsPerVertex * 6;
		x.d->vertexOffset = numVertices;
		x.d->indexOffset = numIndices;
		numVertices += x.d->numVertices;
		numIndices += x.d->numIndices;
	}
	/**
	if (maskMesh_->meshLODs().size() < 4){
		// create a LOD with zero vertices as fallback to not showing grass as mesh
		auto &x = meshLODs_.emplace_back();
		x.d->numVertices = 0;
		x.d->numIndices = 0;
		x.d->vertexOffset = numVertices;
		x.d->indexOffset = numIndices;
	}
	**/

	// allocate attributes
	pos_->setVertexData(numVertices);
	basePos_->setVertexData(numVertices);
	indices_ = createIndexInput(numIndices, numVertices);
	minPosition_ = Vec3f::zero();
	maxPosition_ = Vec3f::zero();

	for (auto lodIdx = 0u; lodIdx < maskMesh_->meshLODs().size(); ++lodIdx) {
		generateLODLevel(lodIdx);
	}

	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	setInput(basePos_);
	updateVertexData();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * indices_->dataTypeBytes();
	}
	activateLOD(0);
}

ref_ptr<GrassPatch> GrassPatch::load(
	LoadingContext &ctx,
	scene::SceneInputNode &input,
	const Rectangle::Config &quadCfg) {
	auto scene = ctx.scene();
	ref_ptr<ModelTransformation> tf;
	ref_ptr<State> dummy = ref_ptr<State>::alloc();

	std::vector<ref_ptr<scene::SceneInputNode>> handledChildren;
	for (auto &n: input.getChildren()) {
		if (n->getCategory() == "transform") {
			handledChildren.push_back(n);
			// load the model transformation
			tf = ModelTransformation::load(ctx, *n.get(), dummy);
			scene->putResource("ModelTransformation", n->getValue("id"), tf);
		}
	}
	for (auto &n: handledChildren) {
		// make sure mesh loading does not attempt to load materials again (this will cause a warning)
		input.removeChild(n);
	}
	if (!tf.get()) {
		tf = ref_ptr<ModelTransformation>::alloc();
	}

	MaskMesh::Config maskCfg;
	maskCfg.quad = quadCfg;
	if (input.hasAttribute("height-map")) {
		maskCfg.heightMap = scene->getResource<Texture2D>(input.getValue("height-map"));
	}
	maskCfg.height = input.getValue<float>("height", 0.0f);
	maskCfg.meshSize = input.getValue<Vec2f>("ground-size", Vec2f::create(10.0f));

	SilhouetteMesh::Config silhouetteCfg;
	if (input.hasAttribute("tile-counts")) {
		auto lodVec = input.getValue<Vec4ui>(
			"tile-counts", Vec4ui(8, 8, 8, 8));
		silhouetteCfg.silhouette.tileCounts.resize(4);
		silhouetteCfg.silhouette.tileCounts[0] = lodVec.x;
		silhouetteCfg.silhouette.tileCounts[1] = lodVec.y;
		silhouetteCfg.silhouette.tileCounts[2] = lodVec.z;
		silhouetteCfg.silhouette.tileCounts[3] = lodVec.w;
	} else {
		silhouetteCfg.silhouette.tileCounts.push_back(input.getValue<uint32_t>("lod", 8));
	}
	if (input.hasAttribute("alpha-cut")) {
		silhouetteCfg.silhouette.alphaCut = input.getValue<float>("alpha-cut", 0.01f);
	}
	if (input.hasAttribute("coverage-threshold")) {
		silhouetteCfg.silhouette.coverageThreshold = input.getValue<float>("coverage-threshold", 0.05f);
	}
	if (input.hasAttribute("silhouette-padding")) {
		silhouetteCfg.silhouette.padPixels = input.getValue<uint32_t>("silhouette-padding", 1u);
	}
	if (input.hasAttribute("texco-scale")) {
		silhouetteCfg.texcoScale = input.getValue<Vec2f>("texco-scale", Vec2f::one());
	}

	ref_ptr<Texture2D> maskTexture;
	uint32_t maskIndex = input.getValue<uint32_t >("mask-index", 0u);
	if (input.hasAttribute("mask")) {
		maskTexture = scene->getResource<Texture2D>(input.getValue("mask"));
	} else if (input.hasAttribute("material-weights")) {
		maskIndex = input.getValue<uint32_t >("material-index", maskIndex);
		uint32_t materialTextureIdx = maskIndex / 4;
		auto materialTextureName = REGEN_STRING(
			input.getValue("material-weights") << "-" << materialTextureIdx);
		maskTexture = scene->getResource<Texture2D>(materialTextureName);
		maskIndex = maskIndex % 4;
	}

	ref_ptr<Texture2D> silhouetteTexture = scene->getResource<Texture2D>(
		input.getValue("silhouette-texture"));
	if (silhouetteTexture.get() == nullptr) {
		REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load silhouette texture.");
		return {};
	}

	auto m = ref_ptr<GrassPatch>::alloc(tf, maskCfg.quad.updateHint);
	m->setMask(maskTexture, maskIndex, maskCfg);
	m->setSilhouette(silhouetteTexture, silhouetteCfg);
	m->updateSilhouette();
	m->updateAttributes();
	m->updateTransforms();
	return m;
}
