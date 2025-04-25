#include <regen/utility/string-util.h>
#include <regen/states/feedback-state.h>

#include "mesh-state.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/obb.h"

using namespace regen;

Mesh::Mesh(const ref_ptr<Mesh> &sourceMesh)
		: State(sourceMesh),
		  HasInput(sourceMesh->inputContainer()),
		  primitive_(sourceMesh->primitive_),
		  meshLODs_(sourceMesh->meshLODs_),
		  v_lodThresholds_(sourceMesh->v_lodThresholds_),
		  lodLevel_(sourceMesh->lodLevel_),
		  boundingShape_(sourceMesh->boundingShape_),
		  shapeBuffer_(sourceMesh->shapeBuffer_),
		  shapeType_(sourceMesh->shapeType_),
		  feedbackCount_(0),
		  hasInstances_(sourceMesh->hasInstances_),
		  sourceMesh_(sourceMesh),
		  isMeshView_(GL_TRUE),
		  minPosition_(sourceMesh->minPosition()),
		  maxPosition_(sourceMesh->maxPosition()),
		  geometryStamp_(sourceMesh->geometryStamp_) {
	vao_ = ref_ptr<VAO>::alloc();
	draw_ = sourceMesh_->draw_;
	set_primitive(primitive_);
	sourceMesh_->meshViews_.insert(this);
	lodThresholds_ = ref_ptr<ShaderInput3f>::alloc("lodThresholds");
	lodThresholds_->setUniformData(sourceMesh->lodThresholds()->getVertex(0).r);
}

Mesh::Mesh(GLenum primitive, BufferUsage usage)
		: State(),
		  HasInput(ARRAY_BUFFER, usage),
		  primitive_(primitive),
		  feedbackCount_(0),
		  isMeshView_(GL_FALSE),
		  minPosition_(-1.0f),
		  maxPosition_(1.0f) {
	vao_ = ref_ptr<VAO>::alloc();
	hasInstances_ = GL_FALSE;
	draw_ = &InputContainer::drawArrays;
	set_primitive(primitive);
	lodThresholds_ = ref_ptr<ShaderInput3f>::alloc("lodThresholds");
	lodThresholds_->setUniformData(Vec3f::zero());
	setLODThresholds(Vec3f(10.0, 30.0, 60.0));
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
				hasInstances_ = GL_TRUE;
			}
		}
	}
	if (in->numInstances() > 1) {
		inputContainer_->set_numInstances(in->numInstances());
		hasInstances_ = GL_TRUE;
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

void Mesh::updateVAO(
		RenderState *rs,
		const StateConfig &cfg,
		const ref_ptr<Shader> &meshShader) {
	// remember the shader
	meshShader_ = meshShader;
	hasInstances_ = cfg.numInstances_ > 1;
	if (cfg.numInstances_ > 1) {
		inputContainer()->set_numInstances(cfg.numInstances_);
		shaderDefine("HAS_INSTANCES", "TRUE");
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
	for (const auto & texture : cfg.textures_) { addShaderInput(texture.first, texture.second); }

	updateVAO(rs);
	updateDrawFunction();
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

void Mesh::updateVAO(RenderState *rs) {
	GLuint lastArrayBuffer = 0;
	rs->vao().push(vao_->id());
	// Setup attributes
	for (auto & vaoAttribute : vaoAttributes_) {
		const ref_ptr<ShaderInput> &in = vaoAttribute.input;
		if (lastArrayBuffer != in->buffer()) {
			lastArrayBuffer = in->buffer();
			glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
		}
		in->enableAttribute(vaoAttribute.location);
		if (in->numInstances() > 1) hasInstances_ = GL_TRUE;
	}
	// bind the index buffer
	if (inputContainer_->indexBuffer() > 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inputContainer_->indexBuffer());
	}
	rs->vao().pop();
}

unsigned int Mesh::getLODLevel(float depth) const {
    // Returns the LOD group for a given depth.
    return (depth >= v_lodThresholds_.x)
         + (depth >= v_lodThresholds_.y)
         + (depth >= v_lodThresholds_.z);
}

void Mesh::setMeshLODs(const std::vector<MeshLOD> &meshLODs) {
	meshLODs_ = meshLODs;
	setLODThresholds(Vec3f(10.0, 30.0, 60.0));
}

void Mesh::setLODThresholds(const Vec3f &thresholds) {
	v_lodThresholds_.x = thresholds.x;
	v_lodThresholds_.y = thresholds.y > v_lodThresholds_.x ? thresholds.y : FLT_MAX;
	v_lodThresholds_.z = thresholds.z > v_lodThresholds_.y ? thresholds.z : FLT_MAX;
	lodThresholds_->setVertex(0, v_lodThresholds_);
}

void Mesh::updateLOD(float cameraDistance) {
	activateLOD(getLODLevel(cameraDistance));
}

void Mesh::activateLOD(GLuint lodLevel) {
	if (meshLODs_.size() <= lodLevel) {
		REGEN_WARN("LOD level " << lodLevel << " not available num LODs: " << meshLODs_.size());
		return;
	}
	MeshLOD &lod = meshLODs_[lodLevel];
	lodLevel_ = lodLevel;
	if (inputContainer_->indexBuffer() > 0) {
		inputContainer_->set_numIndices(lod.numIndices);
		inputContainer_->set_indexOffset(lod.indexOffset);
	} else {
		inputContainer_->set_numVertices(lod.numVertices);
		inputContainer_->set_vertexOffset(lod.vertexOffset);
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

void Mesh::setFeedbackRange(const ref_ptr<BufferRange> &range) {
	feedbackRange_ = range;
}

void Mesh::enable(RenderState *rs) {
	State::enable(rs);

	for (auto & meshUniform : meshUniforms_) {
		InputLocation &x = meshUniform.second;
		// For uniforms below the shader it is expected that
		// they will be set multiple times during shader lifetime.
		// So we upload uniform data each time.
		x.input->enableUniform(x.location);
	}

	if (feedbackRange_.get()) {
		feedbackCount_ = 0;
		rs->feedbackBufferRange().push(0, *feedbackRange_.get());
		rs->beginTransformFeedback(GL_POINTS);
	}

	rs->vao().push(vao_->id());
	(inputContainer_.get()->*draw_)(primitive_);
}

void Mesh::disable(RenderState *rs) {
	if (feedbackRange_.get()) {
		rs->endTransformFeedback();
		rs->feedbackBufferRange().pop(0);
	}

	rs->vao().pop();
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
