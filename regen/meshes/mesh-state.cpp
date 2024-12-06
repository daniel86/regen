/*
 * attribute-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/feedback-state.h>
#include <regen/gl-types/gl-util.h>

#include "mesh-state.h"

using namespace regen;

Mesh::Mesh(const ref_ptr<Mesh> &sourceMesh)
		: State(sourceMesh),
		  HasInput(sourceMesh->inputContainer()),
		  primitive_(sourceMesh->primitive_),
		  feedbackCount_(0),
		  sourceMesh_(sourceMesh),
		  isMeshView_(GL_TRUE),
		  centerPosition_(sourceMesh->centerPosition()),
		  minPosition_(sourceMesh->minPosition()),
		  maxPosition_(sourceMesh->maxPosition()) {
	vao_ = ref_ptr<VAO>::alloc();
	hasInstances_ = GL_FALSE;
	draw_ = sourceMesh_->draw_;
	set_primitive(primitive_);
	sourceMesh_->meshViews_.insert(this);
	meshLODs_ = sourceMesh_->meshLODs_;
}

Mesh::Mesh(GLenum primitive, VBO::Usage usage)
		: State(),
		  HasInput(usage),
		  primitive_(primitive),
		  feedbackCount_(0),
		  isMeshView_(GL_FALSE),
		  centerPosition_(0.0f),
		  minPosition_(-1.0f),
		  maxPosition_(1.0f) {
	vao_ = ref_ptr<VAO>::alloc();
	hasInstances_ = GL_FALSE;
	draw_ = &ShaderInputContainer::drawArrays;
	set_primitive(primitive);
}

Mesh::~Mesh() {
	if (isMeshView_) {
		sourceMesh_->meshViews_.erase(this);
	}
}

void Mesh::getMeshViews(std::set<Mesh *> &out) {
	out.insert(this);
	for (auto it = meshViews_.begin(); it != meshViews_.end(); ++it) { (*it)->getMeshViews(out); }
}

void Mesh::addShaderInput(const std::string &name, const ref_ptr<ShaderInput> &in) {
	if (!meshShader_.get()) return;

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
		if (in->numInstances() > 1) {
			inputContainer_->set_numInstances(in->numInstances());
		}

		auto needle = vaoLocations_.find(loc);
		if (needle == vaoLocations_.end()) {
			vaoAttributes_.emplace_back(in, loc);
			auto it = vaoAttributes_.end();
			--it;
			vaoLocations_[loc] = it;
		} else {
			*needle->second = ShaderInputLocation(in, loc);
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
		meshUniforms_[loc] = ShaderInputLocation(in, loc);
	}
}

void Mesh::updateVAO(
		RenderState *rs,
		const StateConfig &cfg,
		const ref_ptr<Shader> &meshShader) {
	// remember the shader
	meshShader_ = meshShader;
	hasInstances_ = GL_FALSE;

	// reset attribute list
	vaoAttributes_.clear();
	vaoLocations_.clear();
	meshUniforms_.clear();
	// and load from Config
	for (auto it = cfg.inputs_.begin(); it != cfg.inputs_.end(); ++it) { addShaderInput(it->name_, it->in_); }
	// Get input from mesh and joined states (might be handled by StateConfig allready)
	ShaderInputList localInputs;
	collectShaderInput(localInputs);
	for (auto it = localInputs.begin(); it != localInputs.end(); ++it) {
		addShaderInput(it->name_, it->in_);
	}
	// Add Textures
	for (auto it = cfg.textures_.begin(); it != cfg.textures_.end(); ++it) { addShaderInput(it->first, it->second); }

	updateVAO(rs);
	updateDrawFunction();
}

void Mesh::updateDrawFunction() {
	if (inputContainer_->indexBuffer() > 0) {
		if (hasInstances_) {
			draw_ = &ShaderInputContainer::drawElementsInstanced;
		} else {
			draw_ = &ShaderInputContainer::drawElements;
		}
	} else {
		if (hasInstances_) {
			draw_ = &ShaderInputContainer::drawArraysInstanced;
		} else {
			draw_ = &ShaderInputContainer::drawArrays;
		}
	}
}

void Mesh::updateVAO(RenderState *rs) {
	GLuint lastArrayBuffer = 0;
	vao_->resetGL();
	rs->vao().push(vao_->id());
	// Setup attributes
	for (auto it = vaoAttributes_.begin(); it != vaoAttributes_.end(); ++it) {
		const ref_ptr<ShaderInput> &in = it->input;
		if (lastArrayBuffer != in->buffer()) {
			lastArrayBuffer = in->buffer();
			glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
		}
		in->enableAttribute(it->location);
		if (in->numInstances() > 1) hasInstances_ = GL_TRUE;
	}
	// bind the index buffer
	if (inputContainer_->indexBuffer() > 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, inputContainer_->indexBuffer());
	}
	rs->vao().pop();
}

void Mesh::activateLOD(GLuint lodLevel) {
	if (meshLODs_.size() <= lodLevel) {
		REGEN_WARN("LOD level " << lodLevel << " not available num LODs: " << meshLODs_.size());
		return;
	}
	MeshLOD &lod = meshLODs_[lodLevel];
	if (inputContainer_->indexBuffer() > 0) {
		inputContainer_->set_numIndices(lod.numIndices);
		inputContainer_->set_indexOffset(lod.indexOffset);
	} else {
		inputContainer_->set_numVertices(lod.numVertices);
		inputContainer_->set_vertexOffset(lod.vertexOffset);
	}
}

void Mesh::set_extends(const Vec3f &minPosition, const Vec3f &maxPosition) {
	minPosition_ = minPosition;
	maxPosition_ = maxPosition;
}

void Mesh::setFeedbackRange(const ref_ptr<BufferRange> &range) {
	feedbackRange_ = range;
}

void Mesh::enable(RenderState *rs) {
	State::enable(rs);

	for (auto it = meshUniforms_.begin(); it != meshUniforms_.end(); ++it) {
		ShaderInputLocation &x = it->second;
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

////////////
////////////

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
		: Mesh(GL_POINTS, VBO::USAGE_STATIC) {
	inputContainer_->set_numVertices(numVertices);
}
