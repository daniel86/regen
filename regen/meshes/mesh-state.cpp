#include <regen/utility/string-util.h>
#include <regen/states/feedback-state.h>

#include "mesh-state.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/obb.h"
#include "regen/states/state-configurer.h"
#include "regen/shapes/cull-shape.h"
#include "regen/gl-types/draw-command.h"
#include "regen/buffer/dibo.h"

// TODO: think about making a distinction between mesh resource and state.
// TODO: think about introducing a notion of model replacing mesh vector.

//#define REGEN_MESH_DISABLE_MULTI_DRAW

using namespace regen;

struct Mesh::SharedData {
	int32_t vertexOffset_ = 0;
	int32_t numVisibleInstances_ = 1;
	uint32_t baseInstance_ = 0u;
	// index data
	int32_t numIndices_ = 0u;
	uint32_t maxIndex_ = 0u;
	ref_ptr<ShaderInput> indices_;
};

Mesh::Mesh(GLenum primitive, const BufferUpdateFlags &hints)
		: State(),
		  primitive_(primitive),
		  vao_(ref_ptr<VAO>::alloc()),
		  lodLevel_(ref_ptr<uint32_t>::alloc(0u)),
		  minPosition_(-1.0f),
		  maxPosition_(1.0f),
		  shared_(ref_ptr<SharedData>::alloc()) {
	draw_ = &Mesh::draw;
	set_primitive(primitive);
	lodThresholds_ = ref_ptr<ShaderInput3f>::alloc("lodThresholds");
	lodThresholds_->setUniformData(Vec3f::zero());
	sharedState_ = ref_ptr<State>::alloc();
	meshBuffer_ = ref_ptr<VBO>::alloc(ARRAY_BUFFER, hints);
}

Mesh::Mesh(const ref_ptr<Mesh> &sourceMesh)
		: State(sourceMesh),
		  primitive_(sourceMesh->primitive_),
		  meshBuffer_(sourceMesh->meshBuffer_),
		  uploadLayout_(sourceMesh->uploadLayout_),
		  meshLODs_(sourceMesh->meshLODs_),
		  v_lodThresholds_(sourceMesh->v_lodThresholds_),
		  lodLevel_(sourceMesh->lodLevel_),
		  cullShape_(sourceMesh->cullShape_),
		  boundingShape_(sourceMesh->boundingShape_),
		  shapeType_(sourceMesh->shapeType_),
		  shaderKey_(sourceMesh->shaderKey_),
		  shaderStageKeys_(sourceMesh->shaderStageKeys_),
		  feedbackCount_(0),
		  hasInstances_(sourceMesh->hasInstances_),
		  sourceMesh_(sourceMesh),
		  isMeshView_(true),
		  minPosition_(sourceMesh->minPosition()),
		  maxPosition_(sourceMesh->maxPosition()),
		  geometryStamp_(sourceMesh->geometryStamp_),
		  shared_(sourceMesh->shared_),
		  sharedState_(sourceMesh->sharedState_) {
	vao_ = ref_ptr<VAO>::alloc();
	sourceMesh_->meshViews_.insert(this);
	lodThresholds_ = ref_ptr<ShaderInput3f>::alloc("lodThresholds");
	lodThresholds_->setUniformData(sourceMesh->lodThresholds()->getVertex(0).r);
	// create copies of LOD meshes
	for (auto & lod : meshLODs_) {
		if (lod.impostorMesh.get()) {
			lod.impostorMesh = ref_ptr<Mesh>::alloc(lod.impostorMesh);
		}
	}
	updateDrawFunction();
}

Mesh::~Mesh() {
	if (isMeshView_) {
		sourceMesh_->meshViews_.erase(this);
	}
}

void Mesh::setBufferMapMode(BufferMapMode mode) {
	meshBuffer_->setBufferMapMode(mode);
}

void Mesh::setClientAccessMode(ClientAccessMode mode) {
	meshBuffer_->setClientAccessMode(mode);
}

void Mesh::begin(DataLayout layout) {
	uploadLayout_ = layout;
}

ref_ptr<BufferReference> Mesh::end() {
	ref_ptr<BufferReference> ref;
	std::list<ref_ptr<ShaderInput>> attributes;

	// collect all attribute inputs that are not already uploaded
	for (auto &in : inputs()) {
		if (in.in_->isVertexAttribute()) {
			attributes.push_back(in.in_);
		}
	}
	// adopt a buffer range to store the vertex data
	if (!attributes.empty()) {
		if (uploadLayout_ == SEQUENTIAL) {
			ref = meshBuffer_->allocSequential(attributes);
		} else if (uploadLayout_ == INTERLEAVED) {
			ref = meshBuffer_->allocInterleaved(attributes);
		}
	}
	return ref;
}

ref_ptr<BufferReference> Mesh::setIndices(const ref_ptr<ShaderInput> &indices, GLuint maxIndex) {
	shared_->indices_ = indices;
	shared_->numIndices_ = static_cast<int32_t>(shared_->indices_->numVertices());
	shared_->maxIndex_ = maxIndex;
	return meshBuffer_->alloc(shared_->indices_);
}

void Mesh::set_vertexOffset(int32_t v) {
	shared_->vertexOffset_ = v;
}

int32_t Mesh::vertexOffset() const {
	return shared_->vertexOffset_;
}

uint32_t Mesh::baseInstance() const {
	return shared_->baseInstance_;
}

void Mesh::set_baseInstance(uint32_t v) {
	shared_->baseInstance_ = v;
}

int32_t Mesh::numVisibleInstances() const {
	return shared_->numVisibleInstances_;
}

void Mesh::set_numVisibleInstances(int32_t v) {
	shared_->numVisibleInstances_ = v;
}

void Mesh::set_numIndices(int32_t v) {
	shared_->numIndices_ = v;
}

int Mesh::numIndices() const {
	return shared_->numIndices_;
}

uint32_t Mesh::maxIndex() const {
	return shared_->maxIndex_;
}

uint32_t Mesh::indexOffset() const {
	return shared_->indices_.get() ? shared_->indices_->offset() : 0u;
}

const ref_ptr<ShaderInput> &Mesh::indices() const {
	return shared_->indices_;
}

void Mesh::set_indexOffset(uint32_t v) {
	if (shared_->indices_.get()) { shared_->indices_->set_offset(v); }
}

GLuint Mesh::indexBuffer() const {
	return shared_->indices_.get() ? shared_->indices_->buffer() : 0;
}

void Mesh::getMeshViews(std::set<Mesh *> &out) {
	out.insert(this);
	for (auto meshView : meshViews_) { meshView->getMeshViews(out); }
}

void Mesh::addShaderInput(const std::string &name, const ref_ptr<ShaderInput> &in) {
	if (!meshShader_.get()) return;

	if (in->isBufferBlock()) {
		auto *block = dynamic_cast<BufferBlock*>(in.get());
		if (!block) {
			REGEN_ERROR("Shader input '" << name << "' is not a BufferBlock.");
			return;
		}
		for (auto &blockUniform: block->stagedInputs()) {
			if (blockUniform.in_->numInstances() > 1) {
				set_numInstances(blockUniform.in_->numInstances());
				set_numVisibleInstances(blockUniform.in_->numInstances());
				hasInstances_ = true;
			}
		}
	}
	if (in->numInstances() > 1) {
		set_numInstances(in->numInstances());
		set_numVisibleInstances(in->numInstances());
		hasInstances_ = true;
	}

	if (in->isVertexAttribute()) {
		GLint loc = meshShader_->attributeLocation(name);
		if (loc == -1) {
			// not used in shader
			return;
		}
		if (!in->bufferIterator().get()) {
			// allocate VBO memory if not already allocated
			meshBuffer_->alloc(in);
		}

		auto needle = vaoLocations_.find(loc);
		if (needle == vaoLocations_.end()) {
			vaoAttributes_.emplace_back(in, loc);
			auto it = vaoAttributes_.end();
			--it;
			vaoLocations_[loc] = it;
		} else {
			*needle->second = InputLocation(in, loc);
		}
	}
}

void Mesh::createShader(const ref_ptr<StateNode> &parentNode) {
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addNode(parentNode.get());
	shaderConfigurer.addState(sharedState_.get());
	shaderConfigurer.addState(this);
	if (meshLODs_.size()>1) {
		shaderConfigurer.define("HAS_LOD", "TRUE");
	}
	if (!indirectDrawBuffer_.get()) {
		// If we do not have an indirect draw buffer,
		// then multi-layered rendering must be done with GS.
		shaderConfigurer.define("USE_GS_LAYERED_RENDERING", "TRUE");
	}
	if (cullShape_.get()) {
		shaderConfigurer.addState(cullShape_.get());
	}
	createShader(parentNode, shaderConfigurer.cfg());
}

void Mesh::createShader(const ref_ptr<StateNode> &parentNode, StateConfig &shaderConfig) {
	auto shaderState = ref_ptr<ShaderState>::alloc();
	for (auto &joinedState : joined()) {
		// disjoin any ShaderState that might be joined
		auto *shaderStateJoined = dynamic_cast<ShaderState *>(joinedState.get());
		if (shaderStateJoined != nullptr) {
			disjoinStates(joinedState);
			break;
		}
	}
	joinStates(shaderState);

	if(shaderKey_.empty()) {
		shaderKey_ = "regen.models.mesh";
	}
	if(shaderStageKeys_.empty()) {
		shaderState->createShader(shaderConfig, shaderKey_);
	}
	else {
		std::vector<std::string> stageKeys(glenum::glslStageCount());
		for (int i = 0; i < glenum::glslStageCount(); ++i) {
			stageKeys[i] = shaderKey_;
		}
		for (int i = 0; i < glenum::glslStageCount(); ++i) {
			auto stage = glenum::glslStages()[i];
			auto it = shaderStageKeys_.find(stage);
			if (it != shaderStageKeys_.end()) {
				stageKeys[i] = it->second;
			}
		}
		shaderState->createShader(shaderConfig, stageKeys);
	}

	updateVAO(shaderConfig, shaderState->shader());
	// create shader of lod meshes
	for (auto &lod : meshLODs_) {
		if (lod.impostorMesh.get()) {
			lod.impostorMesh->setIndirectDrawBuffer(
				indirectDrawBuffer_, baseDrawIdx_, numDrawLayers_);
			lod.impostorMesh->createShader(parentNode);
		}
	}
}

void Mesh::updateVAO(const StateConfig &cfg, const ref_ptr<Shader> &meshShader) {
	// remember the shader
	meshShader_ = meshShader;
	hasInstances_ = cfg.numInstances_ > 1;
	if (cfg.numInstances_ > 1) {
		set_numInstances(cfg.numInstances_);
		set_numVisibleInstances(cfg.numInstances_);
	}

	// reset attribute list
	vaoAttributes_.clear();
	vaoLocations_.clear();
	// and load from Config
	for (const auto & input : cfg.inputs_) { addShaderInput(input.name_, input.in_); }
	// Get input from mesh and joined states (might be handled by StateConfig allready)
	ShaderInputList localInputs;
	collectShaderInput(localInputs);
	for (auto & localInput : localInputs) {
		addShaderInput(localInput.name_, localInput.in_);
	}
	// Add Textures
	for (const auto & texture : cfg.textures_) {
		addShaderInput(texture.first, texture.second.first);
	}

	updateVAO();
	updateDrawFunction();
}

void Mesh::updateVAO() {
	auto rs = RenderState::get();
	auto lastArrayBuffer = 0u;
	rs->vao().apply(vao_->id());
	// Setup attributes
	for (auto & vaoAttribute : vaoAttributes_) {
		const ref_ptr<ShaderInput> &in = vaoAttribute.input;
		if (lastArrayBuffer != in->buffer()) {
			lastArrayBuffer = in->buffer();
			// NOTE: With VAO bound, ARRAY_BUFFER binding is still handled globally,
			//       it is not part of VAO state.
			rs->arrayBuffer().apply(lastArrayBuffer);
		}
		in->enableAttribute(vaoAttribute.location);
		if (in->numInstances() > 1) hasInstances_ = true;
	}
	// bind the index buffer
	if (indexBuffer() > 0) {
		// NOTE: ELEMENT_ARRAY_BUFFER binding is part of VAO state!
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer());
	}

	// group together LODs that can be drawn with multi draw calls,
	// i.e. those that do not have impostor meshes.
	indirectDrawGroups_.clear();

	if (meshLODs_.empty()) {
		ensureLOD();
	} else {
#ifndef REGEN_MESH_DISABLE_MULTI_DRAW
		uint32_t drawGroupIdx = 0;
#endif
		for (auto &lodData : meshLODs_) {
			if (lodData.d->numVertices == lastNumVertices_ && !lodData.impostorMesh.get()) {
				// update the full LOD data if it is not an impostor mesh
				lodData.d->numVertices = numVertices();
				lodData.d->vertexOffset = vertexOffset();
				lodData.d->numIndices = numIndices();
				lodData.d->indexOffset = indexOffset();
			}
#ifndef REGEN_MESH_DISABLE_MULTI_DRAW
			if (indirectDrawGroups_.size() <= drawGroupIdx) {
				indirectDrawGroups_.emplace_back(0);
			}
			if (lodData.impostorMesh.get()) {
				indirectDrawGroups_.emplace_back(1);
				// note: for now do not use multi draw calls for impostor meshes
				drawGroupIdx += 2;
			} else {
				indirectDrawGroups_[drawGroupIdx] += 1;
			}
#endif
		}
	}
#ifndef REGEN_MESH_DISABLE_MULTI_DRAW
	if (indirectDrawGroups_.size() == meshLODs_.size()) {
		// seems nothing was joined...
		indirectDrawGroups_.clear();
	}
#endif
	lastNumVertices_ = numVertices();

	// initialize num visible instances and offsets for all LODs
	resetVisibility();
}

void Mesh::updateDrawFunction() {
	if (indexBuffer() > 0) {
		if (hasIndirectDrawBuffer()) {
			if (indirectDrawGroups_.empty() && numDrawLayers_ == 1) {
				draw_ = &Mesh::drawIndirectIndexed;
			} else {
				draw_ = &Mesh::drawMultiIndirectIndexed;
			}
		} else if (hasInstances_) {
			draw_ = &Mesh::drawBaseInstancesIndexed;
		} else {
			draw_ = &Mesh::drawIndexed;
		}
	} else {
		if (hasIndirectDrawBuffer()) {
			if (indirectDrawGroups_.empty() && numDrawLayers_ == 1) {
				draw_ = &Mesh::drawIndirect;
			} else {
				draw_ = &Mesh::drawMultiIndirect;
			}
		} else if (hasInstances_) {
			draw_ = &Mesh::drawBaseInstances;
		} else {
			draw_ = &Mesh::draw;
		}
	}
}

uint32_t Mesh::numLODs() const {
	return meshLODs_.empty() ? 1u : meshLODs_.size();
}

unsigned int Mesh::getLODLevel(float distanceSquared) const {
    // Returns the LOD group for a given depth.
    return (distanceSquared >= v_lodThresholds_.x)
         + (distanceSquared >= v_lodThresholds_.y)
         + (distanceSquared >= v_lodThresholds_.z);
}

void Mesh::setLODThresholds(const Vec3f &thresholds) {
	v_lodThresholds_.x = powf(thresholds.x,2);
	v_lodThresholds_.y = powf(thresholds.y,2);
	v_lodThresholds_.z = powf(thresholds.z,2);
	if (v_lodThresholds_.y <= v_lodThresholds_.x) {
		v_lodThresholds_.y = FLT_MAX;
	}
	if (v_lodThresholds_.z <= v_lodThresholds_.y) {
		v_lodThresholds_.z = FLT_MAX;
	}
	lodThresholds_->setVertex(0, v_lodThresholds_);
}

void Mesh::setMeshLODs(const std::vector<MeshLOD> &meshLODs) {
	meshLODs_ = meshLODs;
}

void Mesh::addMeshLOD(const MeshLOD &meshLOD) {
	ensureLOD();
	meshLODs_.push_back(meshLOD);
	if (meshLOD.impostorMesh.get()) {
		if(cullShape_.get()) {
			meshLOD.impostorMesh->setCullShape(cullShape_);
		}
		if(instanceBuffer_.get()) {
			meshLOD.impostorMesh->setInstanceBuffer(instanceBuffer_);
		}
		if(indirectDrawBuffer_.get()) {
			meshLOD.impostorMesh->setIndirectDrawBuffer(
				indirectDrawBuffer_,
				baseDrawIdx_,
				numDrawLayers_);
		}
	}
}

void Mesh::ensureLOD() {
	if (meshLODs_.empty()) {
		meshLODs_.emplace_back(
			numVertices(),
			vertexOffset(),
			numIndices(),
			indexOffset());
	}
}

void Mesh::updateLOD(float cameraDistance) {
	activateLOD(getLODLevel(cameraDistance));
}

void Mesh::activateLOD(uint32_t lodLevel) {
	resetVisibility(true);
	updateVisibility(lodLevel, numInstances(), 0);
}

void Mesh::activateLOD_(uint32_t lodLevel) {
	auto n = numLODs();
	if (n <= lodLevel) {
		REGEN_WARN("LOD level " << lodLevel << " not available num LODs: " << n);
		return;
	}
	if (meshLODs_.empty()) { return; }
	auto &lod = meshLODs_[lodLevel];
	// set the LOD level
	*lodLevel_.get() = lodLevel;
	// select the input container (some LODs may use impostor meshes with different
	// input containers).
	Mesh *targetMesh;
	if (lod.impostorMesh.get()) {
		targetMesh = lod.impostorMesh.get();
	} else {
		targetMesh = this;
	}
	// finally, configure the input container with the LOD data.
	if (targetMesh->indexBuffer() > 0) {
		targetMesh->set_numIndices(lod.d->numIndices);
		targetMesh->set_indexOffset(lod.d->indexOffset);
	} else {
		targetMesh->set_numVertices(lod.d->numVertices);
		targetMesh->set_vertexOffset(lod.d->vertexOffset);
	}
}

void Mesh::resetVisibility(bool resetToInvisible) {
	if (meshLODs_.empty()) return;
	// reset visibility for all LODs
	for (auto &lod : meshLODs_) {
		lod.d->numVisibleInstances = 0;
		lod.d->instanceOffset = 0;
		if (lod.impostorMesh.get()) {
			lod.impostorMesh->resetVisibility(true);
		}
	}
	if (!resetToInvisible) {
		auto &firstLOD = meshLODs_[0];
		firstLOD.d->numVisibleInstances = numInstances();
		firstLOD.d->instanceOffset = 0;
		if (firstLOD.impostorMesh.get()) {
			firstLOD.impostorMesh->updateVisibility(0, numInstances(), 0);
		}
	}
}

void Mesh::updateVisibility(uint32_t lodLevel, uint32_t numInstances, uint32_t instanceOffset) {
	const uint32_t n = numLODs();
	if (n <= lodLevel) {
		REGEN_WARN("LOD level " << lodLevel << " not available num LODs: " << n);
		return;
	}
	if (meshLODs_.empty()) { return; }
	auto &lod = meshLODs_[lodLevel];
	lod.d->numVisibleInstances = numInstances;
	lod.d->instanceOffset = instanceOffset;
	if (lod.impostorMesh.get()) {
		lod.impostorMesh->updateVisibility(0, numInstances, instanceOffset);
	}
}

void Mesh::setIndirectDrawBuffer(
			const ref_ptr<SSBO> &indirectDrawBuffer,
			uint32_t baseDrawIdx,
			uint32_t numDrawLayers) {
	indirectDrawBuffer_ = indirectDrawBuffer;
	baseDrawIdx_ = baseDrawIdx;
	numDrawLayers_ = numDrawLayers;
	if (indirectDrawBuffer_.get()) {
		indirectOffset_ = indirectDrawBuffer_->offset() + baseDrawIdx_ * sizeof(DrawCommand);
	} else {
		indirectOffset_ = 0u;
	}
	updateDrawFunction();
}

void Mesh::createIndirectDrawBuffer(uint32_t numDrawLayers) {
	auto &partLODs = meshLODs();
	std::vector<DrawCommand> drawData(4 * numDrawLayers);
	ensureLOD();

	// Create the indirect draw data for this part and the first layer.
	// DrawID order: LOD0_layer0, LOD0_layer1, LOD0_layer2, ...
	// 				 LOD1_layer0, LOD1_layer1, LOD1_layer2, ...
	for (uint32_t lodIdx = 0; lodIdx < 4; ++lodIdx) {
		const uint32_t lodStartIdx = lodIdx * numDrawLayers;
		DrawCommand &drawParams = drawData[lodStartIdx];
		if (lodIdx < numLODs()) {
			auto &lodData = partLODs[lodIdx];
			Mesh *m = lodData.impostorMesh.get() ? lodData.impostorMesh.get() : this;
			if (m->indices().get()) {
				drawParams.mode = 1u; // 1=elements, 2=arrays
				drawParams.setCount(lodData.d->numIndices);
				drawParams.setFirstElement(lodData.d->indexOffset / sizeof(uint32_t));
				drawParams.data[3] = 0; // base vertex
			} else {
				drawParams.mode = 2u; // 1=elements, 2=arrays
				drawParams.setCount(lodData.d->numVertices);
				drawParams.setFirstElement(lodData.d->vertexOffset);
			}
		} else {
			// no LOD data available, use the base mesh
			drawParams.mode = indices().get() ? 1u : 2u; // 1=elements, 2=arrays
			drawParams.setCount(0);
			drawParams.setFirstElement(0);
		}
		drawParams.setInstanceCount(lodIdx==0 ? numInstances() : 0);
		drawParams.setBaseInstance(0);

		// Copy over the data for the remaining layers.
		for (uint32_t layerIdx = 1; layerIdx < numDrawLayers; ++layerIdx) {
			const uint32_t lodLayerIdx = lodStartIdx + layerIdx;
			std::memcpy(
				&drawData[lodLayerIdx],
				&drawData[lodStartIdx],
				sizeof(DrawCommand));
		}
	}

	// finally create the indirect draw buffer for this part
	indirectDrawBuffer_ = ref_ptr<DrawIndirectBuffer>::alloc(
			"IndirectDrawBuffer", BufferUpdateFlags::FULL_RARELY);
	auto input = ref_ptr<ShaderInputStruct<DrawCommand>>::alloc(
			"DrawCommand", "drawParams", 4 * numDrawLayers);
	input->setInstanceData(1, 1, (byte*)drawData.data());
	indirectDrawBuffer_->addStagedInput(input);
	indirectDrawBuffer_->update();
	numDrawLayers_ = numDrawLayers;
	updateDrawFunction();
}

void Mesh::setBoundingShape(const ref_ptr<BoundingShape> &shape) {
	if (shape->shapeType() == BoundingShapeType::SPHERE) {
		shapeType_ = 0;
		shaderDefine("SHAPE_TYPE", "SPHERE");
	} else if (shape->shapeType() == BoundingShapeType::BOX) {
		auto box = (BoundingBox *) (shape.get());
		shapeType_ = box->isAABB() ? 1 : 2;
		shaderDefine("SHAPE_TYPE", box->isAABB() ? "AABB" : "OBB");
	} else {
		REGEN_WARN("Unsupported shape type for mesh: " << (int)shape->shapeType());
		if(!boundingShape_.get()) createBoundingSphere();
		return;
	}
	boundingShape_ = shape;
}

void Mesh::createBoundingSphere() {
	// create a sphere shape, compute radius from bounding box
	float radius = (maxPosition_ - minPosition_).length() * 0.5f;
	auto sphere = ref_ptr<BoundingSphere>::alloc(centerPosition(), radius);
	setBoundingShape(sphere);
}

void Mesh::createBoundingBox(bool isOBB) {
	// create a box shape, compute radius from bounding box
	Bounds<Vec3f> bounds(minPosition_, maxPosition_);
	if (isOBB) {
		auto box = ref_ptr<OBB>::alloc(bounds);
		setBoundingShape(box);
	} else {
		auto aabb = ref_ptr<AABB>::alloc(bounds);
		setBoundingShape(aabb);
	}
}

void Mesh::setCullShape(const ref_ptr<State> &cullShape) {
	cullShape_ = cullShape;
	if (cullShape_.get()) {
		auto *cs = dynamic_cast<CullShape *>(cullShape_.get());
		if (cs->hasInstanceBuffer()) {
			setInstanceBuffer(cs->instanceBuffer());
		}
	}
	for (auto & lod : meshLODs_) {
		if (lod.impostorMesh.get()) {
			lod.impostorMesh->setCullShape(cullShape_);
		}
	}
}

void Mesh::setInstanceBuffer(const ref_ptr<SSBO> &instanceBuffer) {
	if (instanceBuffer_.get()) {
		if (instanceBuffer_.get() == instanceBuffer.get()) {
			// nothing to do, same buffer
			return;
		}
		removeInput(instanceBuffer_);
	}
	instanceBuffer_ = instanceBuffer;
	if (instanceBuffer_.get()) {
		// join the instance buffer state
		setInput(instanceBuffer_);
	}
	for (auto & lod : meshLODs_) {
		if (lod.impostorMesh.get()) {
			lod.impostorMesh->setInstanceBuffer(instanceBuffer);
		}
	}
}

void Mesh::setFeedbackRange(const ref_ptr<BufferRange> &range) {
	feedbackRange_ = range;
}

void Mesh::draw(RenderState *rs) {
	enable(rs);
	disable(rs);
}

void Mesh::drawMeshLOD(RenderState *rs, uint32_t lodLevel, uint32_t drawIdx, int32_t multiDrawCount) {
	auto &lod = meshLODs_[lodLevel];
	if (!hasIndirectDrawBuffer() && lod.d->numVisibleInstances == 0) {
		// no instances to draw, skip
		// Note: In case of indirect draw buffers, these could be updated GPU-side
		//   then we wouldn't know the instance count here.
		//   Also for the other case we currently skip setting the instance count
		//   on the LOD object, and only write to the buffer -- but we could skip
		//   the raw for this case too.
		return;
	}
	// set the LOD level vertex meta data
	activateLOD_(lodLevel);

	if (lod.impostorMesh.get()) {
		// let the LOD mesh do the draw call.
		// NOTE: assuming here the impostor does not itself have LODs!
		if (hasIndirectDrawBuffer()) {
			lod.impostorMesh->setIndirectDrawBuffer(
					indirectDrawBuffer_,
					drawIdx, numDrawLayers_);
		} else {
			lod.impostorMesh->resetVisibility(true);
			lod.impostorMesh->updateVisibility(0,
					lod.d->numVisibleInstances,
					lod.d->instanceOffset);
		}
		lod.impostorMesh->draw(rs);
	}
	else if (hasIndirectDrawBuffer()) {
		set_indirectOffset(
			indirectDrawBuffer_->drawBufferRef()->address() +
			drawIdx * sizeof(DrawCommand));
		set_multiDrawCount(multiDrawCount);
		drawMesh(rs);
		set_indirectOffset(0);
		set_multiDrawCount(1);
	} else {
		set_numVisibleInstances(lod.d->numVisibleInstances);
		set_baseInstance(lod.d->instanceOffset);
		drawMesh(rs);
		set_numVisibleInstances(numInstances());
		set_baseInstance(0);
	}
}

void Mesh::drawMesh(RenderState *rs) {
	if (feedbackRange_.get()) {
		// TODO: Reconsider transform feedback integration, especially how it should be used
		//    with LODs! e.g. some impostor meshes might cause unwanted behavior!
		feedbackCount_ = 0;
		rs->feedbackBufferRange().push(0, *feedbackRange_.get());
		rs->beginTransformFeedback(GL_POINTS);
	}

	rs->vao().apply(vao_->id());
	if (hasIndirectDrawBuffer()) {
		rs->drawIndirectBuffer().apply(indirectDrawBuffer()->drawBufferRef()->bufferID());
	}
	(this->*draw_)(primitive_);

	if (feedbackRange_.get()) {
		rs->endTransformFeedback();
		rs->feedbackBufferRange().pop(0);
	}
}

void Mesh::enable(RenderState *rs) {
	State::enable(rs);

	if (meshLODs_.empty()) {
		set_multiDrawCount(numDrawLayers_);
		drawMesh(rs);
		set_multiDrawCount(1);
	}
	else if (!indirectDrawBuffer_.get() || indirectDrawGroups_.empty()) {
		uint32_t drawIdx = baseDrawIdx_;
		if (lodSortMode_ == SortMode::BACK_TO_FRONT) {
			for (uint32_t lodLevel = meshLODs_.size(); lodLevel > 0; --lodLevel) {
				drawMeshLOD(rs, lodLevel - 1, drawIdx, numDrawLayers_);
				drawIdx += numDrawLayers_;
			}
		}
		else {
			for (uint32_t lodLevel = 0; lodLevel < meshLODs_.size(); ++lodLevel) {
				drawMeshLOD(rs, lodLevel, drawIdx, numDrawLayers_);
				drawIdx += numDrawLayers_;
			}
		}
		activateLOD_(0);
	} else {
		uint32_t lodLevel = 0u;
		uint32_t drawIdx = baseDrawIdx_;
		for (int32_t groupSize : indirectDrawGroups_) {
			const uint32_t numDraws = groupSize * numDrawLayers_;
			drawMeshLOD(rs, lodLevel, drawIdx, numDraws);
			lodLevel += groupSize;
			drawIdx += numDraws;
		}
	}
}

void Mesh::disable(RenderState *rs) {
	State::disable(rs);
}

ref_ptr<ShaderInput> Mesh::positions() const {
	return getInput(ATTRIBUTE_NAME_POS);
}

ref_ptr<ShaderInput> Mesh::normals() const {
	return getInput(ATTRIBUTE_NAME_NOR);
}

ref_ptr<ShaderInput> Mesh::colors() const {
	return getInput(ATTRIBUTE_NAME_COL0);
}

ref_ptr<ShaderInput> Mesh::boneWeights() const {
	return getInput("boneWeights");
}

ref_ptr<ShaderInput> Mesh::boneIndices() const {
	return getInput("boneIndices");
}

Vec3f Mesh::centerPosition() const {
	return (maxPosition_ + minPosition_) * 0.5;
}

void Mesh::set_bounds(const Vec3f &min, const Vec3f &max) {
	minPosition_ = min;
	maxPosition_ = max;
	geometryStamp_++;
}

void Mesh::draw(GLenum primitive) const {
	glDrawArrays(primitive, shared_->vertexOffset_, numVertices());
}

void Mesh::drawIndexed(GLenum primitive) const {
	glDrawElements(
			primitive,
			shared_->numIndices_,
			shared_->indices_->baseType(),
			REGEN_BUFFER_OFFSET(shared_->indices_->offset()));
}

void Mesh::drawInstances(GLenum primitive) const {
	glDrawArraysInstancedEXT(
			primitive,
			shared_->vertexOffset_,
			numVertices(),
			shared_->numVisibleInstances_);
}

void Mesh::drawInstancesIndexed(GLenum primitive) const {
	glDrawElementsInstancedEXT(
			primitive,
			shared_->numIndices_,
			shared_->indices_->baseType(),
			REGEN_BUFFER_OFFSET(shared_->indices_->offset()),
			shared_->numVisibleInstances_);
}

void Mesh::drawBaseInstances(GLenum primitive) const {
	glDrawArraysInstancedBaseInstance(
			primitive,
			shared_->vertexOffset_,
			numVertices(),
			shared_->numVisibleInstances_,
			shared_->baseInstance_);
}

void Mesh::drawBaseInstancesIndexed(GLenum primitive) const {
	glDrawElementsInstancedBaseInstance(
			primitive,
			shared_->numIndices_,
			shared_->indices_->baseType(),
			REGEN_BUFFER_OFFSET(shared_->indices_->offset()),
			shared_->numVisibleInstances_,
			shared_->baseInstance_);
}

void Mesh::drawIndirect(GLenum primitive) const {
	glDrawArraysIndirect(
		primitive,
		REGEN_BUFFER_OFFSET(indirectOffset_));
}

void Mesh::drawIndirectIndexed(GLenum primitive) const {
	glDrawElementsIndirect(
			primitive,
			shared_->indices_->baseType(),
			REGEN_BUFFER_OFFSET(indirectOffset_));
}

void Mesh::drawMultiIndirect(GLenum primitive) const {
	glMultiDrawArraysIndirect(
		primitive,
		REGEN_BUFFER_OFFSET(indirectOffset_),
		multiDrawCount_,
		sizeof(DrawCommand));
}

void Mesh::drawMultiIndirectIndexed(GLenum primitive) const {
	glMultiDrawElementsIndirect(
		primitive,
		shared_->indices_->baseType(),
		REGEN_BUFFER_OFFSET(indirectOffset_),
		multiDrawCount_,
		sizeof(DrawCommand));
}

void Mesh::loadShaderConfig(LoadingContext &ctx, scene::SceneInputNode &input) {
	if (input.hasAttribute("shader")) {
		setShaderKey(input.getValue<std::string>("shader", shaderKey_));
	} else if (input.hasAttribute("shader-key")) {
		setShaderKey(input.getValue<std::string>("shader-key", shaderKey_));
	} else if (input.hasAttribute("key")) {
		setShaderKey(input.getValue<std::string>("key", shaderKey_));
	}
	for (int i = 0; i < glenum::glslStageCount(); ++i) {
		auto stage = glenum::glslStages()[i];
		auto keyName = glenum::glslStagePrefix(stage);
		if (input.hasAttribute(keyName)) {
			setShaderKey(input.getValue(keyName), stage);
			continue;
		}
		keyName = REGEN_STRING("shader-" << keyName);
		if (input.hasAttribute(keyName)) {
			setShaderKey(input.getValue(keyName), stage);
			continue;
		}
	}
	// read child nodes into shared state
	for (auto &child : input.getChildren()) {
		auto processor = ctx.scene()->getStateProcessor(child->getCategory());
		if (processor.get() == nullptr) {
			REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
		} else {
			processor->processInput(ctx.scene(), *child.get(), ctx.parent(), sharedState_);
		}
	}
}
