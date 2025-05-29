#include <regen/utility/string-util.h>
#include <regen/states/feedback-state.h>

#include "mesh-state.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/obb.h"
#include "regen/states/state-configurer.h"
#include "regen/shapes/cull-shape.h"

// TODO: think about making a distinction between mesh resource and state.
// TODO: think about introducing a notion of model replacing mesh vector.

using namespace regen;

Mesh::Mesh(GLenum primitive, BufferUsage usage)
		: State(),
		  HasInput(ARRAY_BUFFER, usage),
		  primitive_(primitive),
		  lodLevel_(ref_ptr<uint32_t>::alloc(0u)),
		  instanceIDOffset_loc_(-1),
		  vao_(ref_ptr<VAO>::alloc()),
		  minPosition_(-1.0f),
		  maxPosition_(1.0f) {
	draw_ = &InputContainer::drawArrays;
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
		  instanceIDOffset_loc_(-1),
		  cullShape_(sourceMesh->cullShape_),
		  boundingShape_(sourceMesh->boundingShape_),
		  shapeBuffer_(sourceMesh->shapeBuffer_),
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
		auto block = (BufferBlock *) (in.get());
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
	} else if (!in->isConstant()) {
		if (meshShader_->hasUniform(name) &&
			meshShader_->input(name).get() == in.get()) {
			// shader handles uniform already.
			return;
		}
		GLint loc = meshShader_->uniformLocation(name);
		if (loc == -1) {
			// not used in shader
			return;
		}
		meshUniforms_[loc] = InputLocation(in, loc);
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
	meshUniforms_.clear();
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
	instanceIDOffset_loc_ = meshShader_->uniformLocation("instanceIDOffset");

	updateVAO();
	updateDrawFunction();
}

void Mesh::updateVAO() {
	auto rs = RenderState::get();
	auto lastArrayBuffer = 0u;
	rs->vao().push(vao_->id());
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
	rs->vao().pop();

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
		if (hasInstances_) {
			draw_ = &InputContainer::drawElementsInstanced;
		} else {
			draw_ = &InputContainer::drawElements;
		}
	} else {
		if (hasInstances_) {
			draw_ = &InputContainer::drawArraysInstanced;
		} else {
			draw_ = &InputContainer::drawArrays;
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

unsigned int Mesh::getLODLevel(float depth) const {
    // Returns the LOD group for a given depth.
    return (depth >= v_lodThresholds_.x)
         + (depth >= v_lodThresholds_.y)
         + (depth >= v_lodThresholds_.z);
}

void Mesh::setLODThresholds(const Vec3f &thresholds) {
	v_lodThresholds_.x = thresholds.x;
	v_lodThresholds_.y = thresholds.y > v_lodThresholds_.x ? thresholds.y : FLT_MAX;
	v_lodThresholds_.z = thresholds.z > v_lodThresholds_.y ? thresholds.z : FLT_MAX;
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

namespace regen {
	struct SphereShape_GPU {
		Vec3f center = Vec3f::zero();
		float radius = 0.0f;
	};
	struct BoxShape_GPU {
		Vec3f aabbMin = Vec3f::zero();
		float padding = 0.0f;
		Vec3f aabbMax = Vec3f::zero();
	};
}

void Mesh::setBoundingShape(const ref_ptr<BoundingShape> &shape, bool uploadToGPU) {
	auto lastType = shapeType_;
	if (shape->shapeType() == BoundingShapeType::SPHERE) {
		shapeType_ = 0;
		shaderDefine("SHAPE_TYPE", "SPHERE");
	} else if (shape->shapeType() == BoundingShapeType::BOX) {
		auto box = (BoundingBox *) (shape.get());
		shapeType_ = box->isAABB() ? 1 : 2;
		shaderDefine("SHAPE_TYPE", box->isAABB() ? "AABB" : "OBB");
	} else {
		REGEN_WARN("Unsupported shape type for mesh: " << (int)shape->shapeType());
		if(!boundingShape_.get()) createBoundingSphere(uploadToGPU);
		return;
	}
	boundingShape_ = shape;

	if (uploadToGPU) {
		if (!shapeBuffer_.get() || lastType != shapeType_) {
			createShapeBuffer();
		}
		updateShapeBuffer();
	}
}

void Mesh::createBoundingSphere(bool uploadToGPU) {
	// create a sphere shape, compute radius from bounding box
	float radius = (maxPosition_ - minPosition_).length() * 0.5f;
	auto sphere = ref_ptr<BoundingSphere>::alloc(centerPosition(), radius);
	setBoundingShape(sphere, uploadToGPU);
}

void Mesh::createBoundingBox(bool isOBB, bool uploadToGPU) {
	// create a box shape, compute radius from bounding box
	Bounds<Vec3f> bounds(minPosition_, maxPosition_);
	if (isOBB) {
		auto box = ref_ptr<OBB>::alloc(bounds);
		setBoundingShape(box, uploadToGPU);
	} else {
		auto aabb = ref_ptr<AABB>::alloc(bounds);
		setBoundingShape(aabb, uploadToGPU);
	}
}

void Mesh::createShapeBuffer() {
	shapeBuffer_ = ref_ptr<UBO>::alloc("ShapeBuffer", BUFFER_USAGE_DYNAMIC_DRAW);
	if (shapeType_ == 0) {
		shapeBuffer_->addBlockInput(createUniform<ShaderInput3f, Vec3f>("shapeCenter", Vec3f::zero()));
		shapeBuffer_->addBlockInput(createUniform<ShaderInput1f, float>("shapeRadius", 0.0f));
	} else {
		shapeBuffer_->addBlockInput(createUniform<ShaderInput3f, Vec3f>("shapeAABBMin", Vec3f::zero()));
		shapeBuffer_->addBlockInput(createUniform<ShaderInput3f, Vec3f>("shapeAABBMax", Vec3f::zero()));
	}
	shapeBuffer_->update();
	setInput(shapeBuffer_);
}

void Mesh::updateShapeBuffer(byte *shapeData) {
	RenderState::get()->copyWriteBuffer().push(shapeBuffer_->blockReference()->bufferID());
	glBufferSubData(
			GL_COPY_WRITE_BUFFER,
			shapeBuffer_->blockReference()->address(),
			shapeBuffer_->blockReference()->allocatedSize(),
			&shapeData);
	RenderState::get()->copyWriteBuffer().pop();
}

void Mesh::updateShapeBuffer() {
	if (boundingShape_->shapeType() == BoundingShapeType::SPHERE) {
		auto *sphere = (BoundingSphere*)(boundingShape_.get());
		SphereShape_GPU shapeData;
		shapeData.radius = sphere->radius();
		updateShapeBuffer((byte*)&shapeData);
	}
	else if (boundingShape_->shapeType() == BoundingShapeType::BOX) {
		auto *box = (BoundingBox*)(boundingShape_.get());
		BoxShape_GPU shapeData;
		shapeData.aabbMin = box->bounds().min;
		shapeData.aabbMax = box->bounds().max;
		updateShapeBuffer((byte*)&shapeData);
	}
}

const ref_ptr<UBO>& Mesh::getShapeBuffer() {
	if (!shapeBuffer_.get()) {
		createShapeBuffer();
		updateShapeBuffer();
	}
	return shapeBuffer_;
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

void Mesh::drawMeshLOD(RenderState *rs, uint32_t lodLevel) {
	auto &lod = meshLODs_[lodLevel];
	if (lod.d->numVisibleInstances == 0) {
		// no instances to draw, skip
		return;
	}
	// set the LOD level vertex meta data
	activateLOD_(lodLevel);

	// set number of instances to draw
	auto c = activeInputContainer();
	c->set_numVisibleInstances(lod.d->numVisibleInstances);

	if (lod.impostorMesh.get()) {
		// let the LOD mesh do the draw call.
		// NOTE: assuming here the impostor does not itself have LODs!
		lod.impostorMesh->resetVisibility(true);
		lod.impostorMesh->updateVisibility(0,
				lod.d->numVisibleInstances,
				lod.d->instanceOffset);
		lod.impostorMesh->draw(rs);
	}
	else {
		if (instanceIDOffset_loc_ != -1) {
			glUniform1ui(instanceIDOffset_loc_, lod.d->instanceOffset);
		}
		drawMesh(rs);
	}

	c->set_numVisibleInstances(c->numInstances());
}

void Mesh::drawMesh(RenderState *rs) {
	if (feedbackRange_.get()) {
		// TODO: How to handle transform feedback with multiple LODs? how for impostors?
		feedbackCount_ = 0;
		rs->feedbackBufferRange().push(0, *feedbackRange_.get());
		rs->beginTransformFeedback(GL_POINTS);
	}
	{ // TODO: isn't this a bit redundant? I think the shader handles this
		for (auto & meshUniform : meshUniforms_) {
			InputLocation &x = meshUniform.second;
			x.input->enableUniform(x.location);
		}
	}

	rs->vao().push(vao_->id());
	(inputContainer_.get()->*draw_)(primitive_);
	rs->vao().pop();

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
	else {
		if (lodSortMode_ == SortMode::BACK_TO_FRONT) {
			for (uint32_t lodLevel = meshLODs_.size(); lodLevel > 0; --lodLevel) {
				drawMeshLOD(rs, lodLevel - 1);
			}
		}
		else {
			for (uint32_t lodLevel = 0; lodLevel < meshLODs_.size(); ++lodLevel) {
				drawMeshLOD(rs, lodLevel);
			}
		}
		activateLOD_(0);
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
