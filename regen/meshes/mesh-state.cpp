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

// TODO: think about making a distinction between mesh resource and state.
// TODO: think about introducing a notion of model replacing mesh vector.

using namespace regen;

Mesh::Mesh(GLenum primitive, BufferUsage usage)
		: State(),
		  HasInput(ARRAY_BUFFER, usage),
		  primitive_(primitive),
		  lodLevel_(ref_ptr<uint32_t>::alloc(0u)),
		  vao_(ref_ptr<VAO>::alloc()),
		  minPosition_(-1.0f),
		  maxPosition_(1.0f) {
	draw_ = &InputContainer::draw;
	set_primitive(primitive);
	lodThresholds_ = ref_ptr<ShaderInput3f>::alloc("lodThresholds");
	lodThresholds_->setUniformData(Vec3f::zero());
	sharedState_ = ref_ptr<State>::alloc();
}

Mesh::Mesh(const ref_ptr<Mesh> &sourceMesh)
		: State(sourceMesh),
		  HasInput(sourceMesh->inputContainer()),
		  primitive_(sourceMesh->primitive_),
		  meshLODs_(sourceMesh->meshLODs_),
		  v_lodThresholds_(sourceMesh->v_lodThresholds_),
		  lodLevel_(sourceMesh->lodLevel_),
		  cullShape_(sourceMesh->cullShape_),
		  boundingShape_(sourceMesh->boundingShape_),
		  shapeType_(sourceMesh->shapeType_),
		  shaderKey_(sourceMesh->shaderKey_),
		  shaderStageKeys_(sourceMesh->shaderStageKeys_),
		  sharedState_(sourceMesh->sharedState_),
		  feedbackCount_(0),
		  hasInstances_(sourceMesh->hasInstances_),
		  sourceMesh_(sourceMesh),
		  isMeshView_(GL_TRUE),
		  minPosition_(sourceMesh->minPosition()),
		  maxPosition_(sourceMesh->maxPosition()),
		  geometryStamp_(sourceMesh->geometryStamp_) {
	vao_ = ref_ptr<VAO>::alloc();
	draw_ = sourceMesh_->draw_;
	sourceMesh_->meshViews_.insert(this);
	lodThresholds_ = ref_ptr<ShaderInput3f>::alloc("lodThresholds");
	lodThresholds_->setUniformData(sourceMesh->lodThresholds()->getVertex(0).r);
	// create copies of LOD meshes
	for (auto & lod : meshLODs_) {
		if (lod.impostorMesh.get()) {
			lod.impostorMesh = ref_ptr<Mesh>::alloc(lod.impostorMesh);
		}
	}
}

Mesh::~Mesh() {
	if (isMeshView_) {
		sourceMesh_->meshViews_.erase(this);
	}
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
		for (auto &blockUniform: block->blockInputs()) {
			if (blockUniform.in_->numInstances() > 1) {
				inputContainer_->set_numInstances(blockUniform.in_->numInstances());
				hasInstances_ = true;
			}
		}
	}
	if (in->numInstances() > 1) {
		inputContainer_->set_numInstances(in->numInstances());
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
			inputContainer_->inputBuffer()->alloc(in);
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
	if (cullShape_.get()) {
		shaderConfigurer.addState(cullShape_.get());
	}
	createShader(parentNode, shaderConfigurer.cfg());
}

void Mesh::createShader(const ref_ptr<StateNode> &parentNode, StateConfig &shaderConfig) {
	auto shaderState = ref_ptr<ShaderState>::alloc();
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
			lod.impostorMesh->createShader(parentNode);
		}
	}
}

void Mesh::updateVAO(const StateConfig &cfg, const ref_ptr<Shader> &meshShader) {
	// remember the shader
	meshShader_ = meshShader;
	hasInstances_ = cfg.numInstances_ > 1;
	if (cfg.numInstances_ > 1) {
		inputContainer()->set_numInstances(cfg.numInstances_);
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
			rs->arrayBuffer().apply(lastArrayBuffer);
		}
		in->enableAttribute(vaoAttribute.location);
		if (in->numInstances() > 1) hasInstances_ = true;
	}
	// bind the index buffer
	if (inputContainer_->indexBuffer() > 0) {
		rs->elementArrayBuffer().apply(inputContainer_->indexBuffer());
	}

	if (meshLODs_.empty()) {
		meshLODs_.emplace_back(
			inputContainer_->numVertices(),
			inputContainer_->vertexOffset(),
			inputContainer_->numIndices(),
			inputContainer_->indexOffset());
	} else {
		for (auto &lodData : meshLODs_) {
			if (lodData.d->numVertices == lastNumVertices_ && !lodData.impostorMesh.get()) {
				// update the full LOD data if it is not an impostor mesh
				lodData.d->numVertices = inputContainer_->numVertices();
				lodData.d->vertexOffset = inputContainer_->vertexOffset();
				lodData.d->numIndices = inputContainer_->numIndices();
				lodData.d->indexOffset = inputContainer_->indexOffset();
			}
		}
	}
	lastNumVertices_ = inputContainer_->numVertices();

	// initialize num visible instances and offsets for all LODs
	resetVisibility();
}

void Mesh::updateDrawFunction() {
	if (inputContainer_->indexBuffer() > 0) {
		if (inputContainer_->hasIndirectDrawBuffer()) {
			if (indirectDrawGroups_.empty()) {
				draw_ = &InputContainer::drawIndirectIndexed;
			} else {
				draw_ = &InputContainer::drawMultiIndirectIndexed;
			}
		} else if (hasInstances_) {
			draw_ = &InputContainer::drawBaseInstancesIndexed;
		} else {
			draw_ = &InputContainer::drawIndexed;
		}
	} else {
		if (inputContainer_->hasIndirectDrawBuffer()) {
			if (indirectDrawGroups_.empty()) {
				draw_ = &InputContainer::drawIndirect;
			} else {
				draw_ = &InputContainer::drawMultiIndirect;
			}
		} else if (hasInstances_) {
			draw_ = &InputContainer::drawBaseInstances;
		} else {
			draw_ = &InputContainer::draw;
		}
	}
}

uint32_t Mesh::numLODs() const {
	return meshLODs_.empty() ? 1u : meshLODs_.size();
}

const ref_ptr<InputContainer> &Mesh::activeInputContainer() const {
	auto &currentLOD = meshLODs_[*lodLevel_.get()];
	if (currentLOD.impostorMesh.get()) {
		return currentLOD.impostorMesh->inputContainer();
	} else {
		return inputContainer_;
	}
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
	if (meshLODs_.empty()) {
		meshLODs_.emplace_back(
			inputContainer_->numVertices(),
			inputContainer_->vertexOffset(),
			inputContainer_->numIndices(),
			inputContainer_->indexOffset());
	}
	meshLODs_.push_back(meshLOD);
	if (meshLOD.impostorMesh.get() && cullShape_.get()) {
		meshLOD.impostorMesh->setCullShape(cullShape_);
	}
}

void Mesh::updateLOD(float cameraDistance) {
	activateLOD(getLODLevel(cameraDistance));
}

void Mesh::activateLOD(uint32_t lodLevel) {
	resetVisibility(true);
	updateVisibility(lodLevel, inputContainer()->numInstances(), 0);
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
	InputContainer *inputContainer;
	if (lod.impostorMesh.get()) {
		inputContainer = lod.impostorMesh->inputContainer().get();
	} else {
		inputContainer = this->inputContainer_.get();
	}
	// finally, configure the input container with the LOD data.
	if (inputContainer->indexBuffer() > 0) {
		inputContainer->set_numIndices(lod.d->numIndices);
		inputContainer->set_indexOffset(lod.d->indexOffset);
	} else {
		inputContainer->set_numVertices(lod.d->numVertices);
		inputContainer->set_vertexOffset(lod.d->vertexOffset);
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
		firstLOD.d->numVisibleInstances = inputContainer()->numInstances();
		firstLOD.d->instanceOffset = 0;
		if (firstLOD.impostorMesh.get()) {
			firstLOD.impostorMesh->updateVisibility(0, inputContainer()->numInstances(), 0);
		}
	}
}

void Mesh::updateVisibility(uint32_t lodLevel, uint32_t numInstances, uint32_t instanceOffset) {
	auto n = numLODs();
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

void Mesh::setIndirectDrawBuffer(const ref_ptr<SSBO> &indirectDrawBuffer, uint32_t baseDrawIdx) {
	inputContainer_->setIndirectDrawBuffer(indirectDrawBuffer, baseDrawIdx);
	// group together LODs that can be drawn with multi draw calls,
	// i.e. those that do not have impostor meshes.
	indirectDrawGroups_.clear();
	if (meshLODs_.size()>1) {
		uint32_t drawGroupIdx = 0;
		for (auto & lod : meshLODs_) {
			if (indirectDrawGroups_.size() <= drawGroupIdx) {
				indirectDrawGroups_.emplace_back(0);
			}
			if (lod.impostorMesh.get()) {
				indirectDrawGroups_.emplace_back(1);
				// note: for now do not use multi draw calls for impostor meshes
				drawGroupIdx += 2;
			} else {
				indirectDrawGroups_[drawGroupIdx] += 1;
			}
		}
	}
	if (indirectDrawGroups_.size() == meshLODs_.size()) {
		// seems nothing was joined...
		indirectDrawGroups_.clear();
	}
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
		joinShaderInput(cs->instanceIDBuffer());
	}
	for (auto & lod : meshLODs_) {
		if (lod.impostorMesh.get()) {
			lod.impostorMesh->setCullShape(cullShape_);
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

void Mesh::drawMeshLOD(RenderState *rs, uint32_t lodLevel, int32_t multiDrawCount) {
	auto &lod = meshLODs_[lodLevel];
	if (!inputContainer_->hasIndirectDrawBuffer() && lod.d->numVisibleInstances == 0) {
		// no instances to draw, skip
		// note: we do not know number of visible instances in case of indirect draw buffers.
		return;
	}
	// set the LOD level vertex meta data
	activateLOD_(lodLevel);

	// set number of instances to draw
	auto c = activeInputContainer();

	if (lod.impostorMesh.get()) {
		// let the LOD mesh do the draw call.
		// NOTE: assuming here the impostor does not itself have LODs!
		if (inputContainer_->hasIndirectDrawBuffer()) {
			c->setIndirectDrawBuffer(
					inputContainer_->indirectDrawBuffer(),
					inputContainer_->baseDrawIndex() + lodLevel);
			lod.impostorMesh->updateDrawFunction();
		} else {
			lod.impostorMesh->resetVisibility(true);
			lod.impostorMesh->updateVisibility(0,
					lod.d->numVisibleInstances,
					lod.d->instanceOffset);
		}
		lod.impostorMesh->draw(rs);
	}
	else {
		c->set_numVisibleInstances(lod.d->numVisibleInstances);
		c->set_baseInstance(lod.d->instanceOffset);
		if (c->hasIndirectDrawBuffer()) {
			c->set_indirectOffset(
				c->indirectDrawBuffer()->blockReference()->address() +
				// each segment in the indirect draw buffer takes sizeof(DrawCommand)=32byte space
				(c->baseDrawIndex() + lodLevel) * sizeof(DrawCommand));
			c->set_multiDrawCount(multiDrawCount);
		}
		drawMesh(rs);
		c->set_numVisibleInstances(c->numInstances());
		c->set_baseInstance(0);
		c->set_indirectOffset(0);
		c->set_multiDrawCount(1);
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
	if (inputContainer_->hasIndirectDrawBuffer()) {
		rs->drawIndirectBuffer().apply(inputContainer_->indirectDrawBuffer()->blockReference()->bufferID());
	}
	(inputContainer_.get()->*draw_)(primitive_);

	if (feedbackRange_.get()) {
		rs->endTransformFeedback();
		rs->feedbackBufferRange().pop(0);
	}
}

void Mesh::enable(RenderState *rs) {
	State::enable(rs);

	if (meshLODs_.empty()) {
		drawMesh(rs);
	}
	else if (indirectDrawGroups_.empty()) {
		if (lodSortMode_ == SortMode::BACK_TO_FRONT) {
			for (uint32_t lodLevel = meshLODs_.size(); lodLevel > 0; --lodLevel) {
				drawMeshLOD(rs, lodLevel - 1, 1);
			}
		}
		else {
			for (uint32_t lodLevel = 0; lodLevel < meshLODs_.size(); ++lodLevel) {
				drawMeshLOD(rs, lodLevel, 1);
			}
		}
		activateLOD_(0);
	} else {
		uint32_t lodLevel = 0u;
		for (int32_t groupSize : indirectDrawGroups_) {
			drawMeshLOD(rs, lodLevel, groupSize);
			lodLevel += groupSize;
		}
	}
}

void Mesh::disable(RenderState *rs) {
	State::disable(rs);
}

ref_ptr<ShaderInput> Mesh::positions() const {
	return inputContainer_->getInput(ATTRIBUTE_NAME_POS);
}

ref_ptr<ShaderInput> Mesh::normals() const {
	return inputContainer_->getInput(ATTRIBUTE_NAME_NOR);
}

ref_ptr<ShaderInput> Mesh::colors() const {
	return inputContainer_->getInput(ATTRIBUTE_NAME_COL0);
}

ref_ptr<ShaderInput> Mesh::boneWeights() const {
	return inputContainer_->getInput("boneWeights");
}

ref_ptr<ShaderInput> Mesh::boneIndices() const {
	return inputContainer_->getInput("boneIndices");
}

Vec3f Mesh::centerPosition() const {
	return (maxPosition_ + minPosition_) * 0.5;
}

void Mesh::set_bounds(const Vec3f &min, const Vec3f &max) {
	minPosition_ = min;
	maxPosition_ = max;
	geometryStamp_++;
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
