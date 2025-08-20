#include "grass-patch.h"
#include "regen/textures/texture-loader.h"
#include "regen/meshes/mesh-vector.h"

using namespace regen;

//#define USE_HIGH_LOD_CROSS_MESH

GrassPatch::GrassPatch(
				const ref_ptr<ModelTransformation> &tf,
				const ref_ptr<Texture2D> &maskTexture,
				uint32_t maskIndex,
				const MaskMesh::Config &patchCfg) :
		Mesh(GL_TRIANGLES, patchCfg.quad.updateHint) {
	shaderDefine("VERTEX_MASK_INDEX", REGEN_STRING(maskIndex));

	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	basePos_ = ref_ptr<ShaderInput3f>::alloc("basePos");
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	setBufferMapMode(patchCfg.quad.mapMode);
	setClientAccessMode(patchCfg.quad.accessMode);
	maskMesh_ = ref_ptr<MaskMesh>::alloc(tf, maskTexture, maskIndex, patchCfg);
	joinStates(tf);
	joinStates(maskMesh_->maskTextureState());
}

void GrassPatch::updateTransforms() {
	maskMesh_->updateMask();
}

void GrassPatch::generateLODLevel(uint32_t lodLevel) {
	// TODO: in later LODs, rather add rows of grass quads
	const Vec2f grassQuadSize = Vec2f(1.0f);
	auto &maskLOD = maskMesh_->meshLODs()[lodLevel];
	auto &grassLOD = meshLODs_[lodLevel];
	// map client data for reading
	auto mask_p = maskMesh_->pos()->mapClientData<Vec3f>(BUFFER_GPU_READ);
	// map client data for writing
	auto grass_i  = (GLuint*)indices_->clientBuffer()->clientData(0);
	auto grass_p  = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto grass_base = (Vec3f*) basePos_->clientBuffer()->clientData(0);
	// offsets into data arrays
	grass_p += grassLOD.vertexOffset();
	grass_base += grassLOD.vertexOffset();
	grass_i += grassLOD.indexOffset();
	uint32_t maskOffset = maskLOD.vertexOffset();
	uint32_t vBaseOffset = grassLOD.vertexOffset();
#ifdef USE_HIGH_LOD_CROSS_MESH
	uint32_t numQuadsPerVertex = (lodLevel > 0 ? 1 : 3);
#else
	uint32_t numQuadsPerVertex = 1;
#endif

	Vec3f quadNormal;
	const Mat4f quadRotation[3] = {
		Mat4f::rotationMatrix(0.0f, 0.0f, 0.0f), // No rotation
		Mat4f::rotationMatrix(0.0f, M_PI/3.0, 0.0f), // 60 degrees rotation
		Mat4f::rotationMatrix(0.0f, M_PI*2.0/3.0, 0.0f) // 120 degrees rotation
	};

	for (uint32_t objIdx = 0; objIdx < maskLOD.numVertices(); ++objIdx) {
		// The base position is centered at the bottom of the quad.
		const Vec3f &basePos = mask_p.r[maskOffset + objIdx];
		uint32_t vOffset = objIdx * numQuadsPerVertex * 4;
		uint32_t iOffset = objIdx * numQuadsPerVertex * 6;

		for (uint32_t quadIdx = 0; quadIdx < numQuadsPerVertex; ++quadIdx) {
			const Mat4f &rot = quadRotation[quadIdx];
			// Start with the 4 vertices of the quad.
			grass_p[vOffset + 0] = basePos + rot.transformVector(
				Vec3f(-0.5f*grassQuadSize.x, 1.0f*grassQuadSize.y, 0.0f));
			grass_p[vOffset + 1] = basePos + rot.transformVector(
				Vec3f(0.5f*grassQuadSize.x, 1.0f*grassQuadSize.y, 0.0f));
			grass_p[vOffset + 2] = basePos + rot.transformVector(
				Vec3f(0.5f*grassQuadSize.x, 0.0f, 0.0f));
			grass_p[vOffset + 3] = basePos + rot.transformVector(
				Vec3f(-0.5f*grassQuadSize.x, 0.0f, 0.0f));
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
			grass_i[iOffset + 0] = vBaseOffset + vOffset + 3;
			grass_i[iOffset + 1] = vBaseOffset + vOffset + 1;
			grass_i[iOffset + 2] = vBaseOffset + vOffset + 0;
			grass_i[iOffset + 3] = vBaseOffset + vOffset + 3;
			grass_i[iOffset + 4] = vBaseOffset + vOffset + 2;
			grass_i[iOffset + 5] = vBaseOffset + vOffset + 1;
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
#ifdef USE_HIGH_LOD_CROSS_MESH
		uint32_t numQuadsPerVertex = (lodIdx > 0 ? 1 : 3);
#else
		uint32_t numQuadsPerVertex = 1;
#endif
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
	indices_->setVertexData(numIndices);
	minPosition_ = Vec3f(0.0);
	maxPosition_ = Vec3f(0.0);

	for (auto lodIdx = 0u; lodIdx < maskMesh_->meshLODs().size(); ++lodIdx) {
		generateLODLevel(lodIdx);
	}

	begin(INTERLEAVED);
	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	setInput(basePos_);
	end();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * sizeof(GLuint);
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

	MaskMesh::Config meshCfg;
	meshCfg.quad = quadCfg;
	if (input.hasAttribute("height-map")) {
		meshCfg.heightMap = scene->getResource<Texture2D>(input.getValue("height-map"));
	}
	meshCfg.height = input.getValue<float>("height", 0.0f);
	meshCfg.meshSize = input.getValue<Vec2f>("ground-size", Vec2f(10.0f));

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
	if (maskTexture.get() == nullptr) {
		REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load mask texture.");
		return {};
	}

	auto m = ref_ptr<GrassPatch>::alloc(tf, maskTexture, maskIndex, meshCfg);
	m->updateAttributes();
	m->updateTransforms();
	return m;
}
