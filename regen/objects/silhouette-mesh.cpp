#include "lod/tessellation.h"
#include "silhouette-mesh.h"
#include "regen/scene/resource-manager.h"
#include "regen/objects/lod/tile-merge-generator.h"

using namespace regen;

SilhouetteMesh::SilhouetteMesh(const ref_ptr<Texture2D> &silhouetteTex, const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.updateHints),
		  silhouetteCfg_(cfg),
		  silhouetteTex_(silhouetteTex) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	uv_ = ref_ptr<ShaderInput2f>::alloc("texco0");
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
}

SilhouetteMesh::Config::Config() = default;

void SilhouetteMesh::setSilhouetteGenerator(const ref_ptr<SilhouetteGenerator> &g) {
	silhouetteGenerator_ = g;
}

void SilhouetteMesh::updateAttributes() {
	// Generate multiple LOD levels of the torus
	const uint32_t numLODs = silhouetteCfg_.silhouette.tileCounts.size();
	uint32_t numVertices = 0;
	uint32_t numIndices = 0;
	for (uint32_t lodIdx=0; lodIdx<numLODs; lodIdx++) {
		const auto &uv_rects = silhouetteUVRects_[lodIdx];
		auto &x = meshLODs_.emplace_back();
		x.d->numVertices = uv_rects.size() * 4; // each rect has 4 vertices
		x.d->numIndices = uv_rects.size() * 6; // each rect has 2 triangles, 3 indices each
		x.d->vertexOffset = numVertices;
		x.d->indexOffset = numIndices;
		numVertices += x.d->numVertices;
		numIndices += x.d->numIndices;
	}

	pos_->setVertexData(numVertices);
	if (silhouetteCfg_.hasNormal) {
		nor_->setVertexData(numVertices);
	}
	if (silhouetteCfg_.hasTangent) {
		tan_->setVertexData(numVertices);
	}
	if (silhouetteCfg_.hasUV) {
		uv_->setVertexData(numVertices);
	}
	indices_ = createIndexInput(numIndices, numVertices);

	for (auto i = 0u; i < meshLODs_.size(); ++i) {
		generateLODLevel(i, meshLODs_[i].d->vertexOffset, meshLODs_[i].d->indexOffset);
	}

	// Set up the vertex attributes
	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (silhouetteCfg_.hasNormal) {
		setInput(nor_);
	}
	if (silhouetteCfg_.hasUV) {
		setInput(uv_);
	}
	if (silhouetteCfg_.hasTangent) {
		setInput(tan_);
	}
	updateVertexData();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * indices_->dataTypeBytes();
	}
	activateLOD(0);

	minPosition_ = Vec3f::create(-0.5f) * silhouetteCfg_.posScale;
	maxPosition_ = Vec3f::create(0.5f) * silhouetteCfg_.posScale;
}

void SilhouetteMesh::generateLODLevel(
		uint32_t lodLevel, uint32_t vertexOffset, uint32_t indexOffset) {
	// map client data for writing
	auto v_pos = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto v_nor = (silhouetteCfg_.hasNormal ? (Vec3f*) nor_->clientBuffer()->clientData(0) : nullptr);
	auto v_tan = (silhouetteCfg_.hasTangent ? (Vec4f*) tan_->clientBuffer()->clientData(0) : nullptr);
	auto v_uv = (silhouetteCfg_.hasUV ? (Vec2f*) uv_->clientBuffer()->clientData(0) : nullptr);
	auto indices = (byte*)indices_->clientBuffer()->clientData(0);
	auto indexType = indices_->baseType();
	// get a handle on the silhouette data for this LOD
	const auto &uv_rects = silhouetteUVRects_[lodLevel];
	// current offset into vertex and index arrays
	uint32_t vi = vertexOffset;
	uint32_t ii = indexOffset;
	Mat4f rotMat = Mat4f::rotationMatrix(
		silhouetteCfg_.rotation.x,
		silhouetteCfg_.rotation.y,
		silhouetteCfg_.rotation.z);

	float areaSum = 0.0f;

	for (const auto &uvRect : uv_rects) {
        // uvRect = (u0, v0, u1, v1) in normalized 0..1 space
		float u0 = uvRect.x, v0 = uvRect.y, u1 = uvRect.z, v1 = uvRect.w;

        // positions in local sprite plane.
        Vec2f local0( (u0+u1)*0.5f - 0.5f, (v0+v1)*0.5f - 0.5f );
        Vec2f localSize( u1 - u0, v1 - v0 );

        // We'll create geometry in UV-space scaled to [-0.5..0.5] so that instancing can position it in world.
        Vec3f p0(local0.x - localSize.x*0.5f, local0.y - localSize.y*0.5f, 0.0f);
        Vec3f p1(local0.x + localSize.x*0.5f, local0.y - localSize.y*0.5f, 0.0f);
        Vec3f p2(local0.x + localSize.x*0.5f, local0.y + localSize.y*0.5f, 0.0f);
        Vec3f p3(local0.x - localSize.x*0.5f, local0.y + localSize.y*0.5f, 0.0f);

        // compute the area of the quad
        float l1 = (p1 - p0).length();
        float l2 = (p3 - p0).length();
        areaSum += l1 * l2;

        v_pos[vi+0] = rotMat.transformVector(silhouetteCfg_.posScale * p0);
        v_pos[vi+1] = rotMat.transformVector(silhouetteCfg_.posScale * p1);
        v_pos[vi+2] = rotMat.transformVector(silhouetteCfg_.posScale * p2);
        v_pos[vi+3] = rotMat.transformVector(silhouetteCfg_.posScale * p3);

        if (v_uv) {
            v_uv[vi+0] = Vec2f(u0, v0);
            v_uv[vi+1] = Vec2f(u1, v0);
            v_uv[vi+2] = Vec2f(u1, v1);
            v_uv[vi+3] = Vec2f(u0, v1);
        }
        if (v_nor) {
            v_nor[vi+0] = Vec3f::front();
            v_nor[vi+1] = Vec3f::front();
            v_nor[vi+2] = Vec3f::front();
            v_nor[vi+3] = Vec3f::front();
        }
        if (v_tan) {
        	// Tangent space: X is right, Y is up, Z is forward
			v_tan[vi+0] = Vec4f(1.0f, 0.0f, 0.0f, 1.0f); // tangent
			v_tan[vi+1] = Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
			v_tan[vi+2] = Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
			v_tan[vi+3] = Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
		}
        // indices (two triangles)
        setIndexValue(indices, indexType, ii++, vi + 0);
        setIndexValue(indices, indexType, ii++, vi + 1);
        setIndexValue(indices, indexType, ii++, vi + 2);
        setIndexValue(indices, indexType, ii++, vi + 2);
        setIndexValue(indices, indexType, ii++, vi + 3);
        setIndexValue(indices, indexType, ii++, vi + 0);

        vi += 4;
	}

	REGEN_INFO("Silhouette LOD " << lodLevel
			  << " with " << uv_rects.size() << " UV rectangles,"
			  << " area=" << areaSum
			  << " vertexOffset=" << vertexOffset
			  << ", indexOffset=" << indexOffset << " num vertices=" << uv_rects.size() * 4
			  << ", num indices=" << uv_rects.size() * 6);
}

void SilhouetteMesh::updateSilhouette() {
	if (!silhouetteGenerator_.get()) {
		// use default generator is user did not specif one.
		silhouetteGenerator_ = ref_ptr<TileMergeGenerator>::alloc(
				silhouetteTex_, silhouetteCfg_.silhouette);
	}
	silhouetteTex_->ensureTextureData();

	const uint32_t numLODs = silhouetteCfg_.silhouette.tileCounts.size();
	silhouetteUVRects_.resize(numLODs);

	for (uint32_t lodIdx = 0; lodIdx < numLODs; ++lodIdx) {
		silhouetteUVRects_[lodIdx].clear();
		silhouetteGenerator_->generateLOD(lodIdx, silhouetteUVRects_[lodIdx]);
	}
}

ref_ptr<SilhouetteMesh> SilhouetteMesh::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();
	auto tex = scene->getResource<Texture2D>(
			input.getValue("silhouette-texture"));
	if (tex.get() == nullptr) {
		REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load silhouette texture.");
		return {};
	}

	SilhouetteMesh::Config cfg;
	if (input.hasAttribute("tile-counts")) {
		auto lodVec = input.getValue<Vec4ui>(
			"tile-counts", Vec4ui(8, 8, 8, 8));
		cfg.silhouette.tileCounts.resize(4);
		cfg.silhouette.tileCounts[0] = lodVec.x;
		cfg.silhouette.tileCounts[1] = lodVec.y;
		cfg.silhouette.tileCounts[2] = lodVec.z;
		cfg.silhouette.tileCounts[3] = lodVec.w;
	} else {
		cfg.silhouette.tileCounts.push_back(input.getValue<GLuint>("lod", 8));
	}
	if (input.hasAttribute("alpha-cut")) {
		cfg.silhouette.alphaCut = input.getValue<float>("alpha-cut", 0.01f);
	}
	if (input.hasAttribute("coverage-threshold")) {
		cfg.silhouette.coverageThreshold = input.getValue<float>("coverage-threshold", 0.05f);
	}
	if (input.hasAttribute("silhouette-padding")) {
		cfg.silhouette.padPixels = input.getValue<uint32_t>("silhouette-padding", 1u);
	}
	cfg.posScale = input.getValue<Vec3f>("pos-scale", Vec3f::one());
	cfg.rotation = input.getValue<Vec3f>("rotation", Vec3f::zero());
	cfg.texcoScale = input.getValue<Vec2f>("texco-scale", Vec2f::one());
	cfg.hasUV = input.getValue<bool>("has-uv", true);
	cfg.hasNormal = input.getValue<bool>("has-normal", false);
	cfg.hasTangent = input.getValue<bool>("has-tangent", false);

	auto silhouette = ref_ptr<SilhouetteMesh>::alloc(tex, cfg);
	silhouette->updateSilhouette();
	silhouette->updateAttributes();
	return silhouette;
}
