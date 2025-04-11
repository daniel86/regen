#include <regen/utility/string-util.h>
#include <regen/states/feedback-state.h>

#include "mesh-state.h"

using namespace regen;

Mesh::Mesh(const ref_ptr<Mesh> &sourceMesh)
		: State(sourceMesh),
		  HasInput(sourceMesh->inputContainer()),
		  primitive_(sourceMesh->primitive_),
		  meshLODs_(sourceMesh->meshLODs_),
		  lodFar_(sourceMesh->lodFar_),
		  lodLevel_(sourceMesh->lodLevel_),
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
	vao_->resetGL();
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

unsigned int Mesh::getLODLevel(float cameraDistance) {
	auto normalizedDistance = std::min(cameraDistance / lodFar_, 0.9999f);
	auto lodLevel = static_cast<float>(meshLODs_.size()) * normalizedDistance;
	lodLevel = std::trunc(lodLevel);
	return static_cast<unsigned int>(lodLevel);
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
