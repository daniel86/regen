/*
 * picking.cpp
 *
 *  Created on: 17.02.2013
 *      Author: daniel
 */

#include <cfloat>
#include <regen/states/atomic-states.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>

#include "picking.h"

using namespace regen;

GLuint PickingGeom::PICK_EVENT = EventObject::registerEvent("pickEvent");

PickingGeom::PickingGeom(
		const ref_ptr<ShaderInput2f> &mouseTexco,
		const ref_ptr<ShaderInputMat4> &inverseProjectionMatrix)
		: State(),
		  Animation(GL_TRUE, GL_FALSE),
		  pickMeshID_(1) {
	inverseProjectionMatrix_ = inverseProjectionMatrix;
	maxPickedObjects_ = 100;

	pickedMesh_ = NULL;
	pickedInstance_ = 0;
	pickedObject_ = 0;
	dt_ = 0.0;
	pickInterval_ = 50.0;

	mouseTexco_ = mouseTexco;
	mousePosVS_ = ref_ptr<ShaderInput3f>::alloc("mousePosVS");
	mousePosVS_->setUniformData(Vec3f(0.0f));
	mouseDirVS_ = ref_ptr<ShaderInput3f>::alloc("mouseDirVS");
	mouseDirVS_->setUniformData(Vec3f(0.0f));

	pickObjectID_ = ref_ptr<ShaderInput1i>::alloc("pickObjectID");
	pickObjectID_->setUniformData(0);

	bufferSize_ = sizeof(PickData) * maxPickedObjects_;
	feedbackBuffer_ = ref_ptr<VBO>::alloc(VBO::USAGE_FEEDBACK);
	vboRef_ = feedbackBuffer_->alloc(bufferSize_);
	bufferRange_.buffer_ = vboRef_->bufferID();

	joinStates(ref_ptr<ToggleState>::alloc(RenderState::RASTARIZER_DISCARD, GL_TRUE));

	ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
	depth->set_useDepthWrite(GL_FALSE);
	depth->set_useDepthTest(GL_FALSE);
	joinStates(depth);

	{
		std::map<GLenum, std::string> unprocessed;
		std::map<GLenum, std::string> processed;
		unprocessed[GL_GEOMETRY_SHADER] = "#include regen.picking.gs";
		Shader::preProcess(processed,
						   PreProcessorConfig(150, unprocessed),
						   Shader::singleStagePreProcessor());

		pickerCode_ = processed[GL_GEOMETRY_SHADER];
		pickerShader_ = glCreateShader(GL_GEOMETRY_SHADER);

		GLint length = -1, status = 0;
		const char *cstr = pickerCode_.c_str();
		glShaderSource(pickerShader_, 1, &cstr, &length);
		glCompileShader(pickerShader_);

		glGetShaderiv(pickerShader_, GL_COMPILE_STATUS, &status);
		if (!status) {
			Shader::printLog(pickerShader_,
							 GL_GEOMETRY_SHADER, pickerCode_.c_str(), GL_FALSE);
		}
	}

	glGenQueries(1, &countQuery_);
}

PickingGeom::~PickingGeom() {
	glDeleteQueries(1, &countQuery_);
}

void PickingGeom::set_pickInterval(GLdouble interval) { pickInterval_ = interval; }

void PickingGeom::emitPickEvent() {
	ref_ptr<PickEvent> ev = ref_ptr<PickEvent>::alloc();
	ev->instanceId = pickedInstance_;
	ev->objectId = pickedObject_;
	ev->state = pickedMesh_;
	State::emitEvent(PICK_EVENT, ev);
}

const Mesh *PickingGeom::pickedMesh() const { return pickedMesh_; }

GLint PickingGeom::pickedInstance() const { return pickedInstance_; }

GLint PickingGeom::pickedObject() const { return pickedObject_; }

ref_ptr<ShaderState> PickingGeom::createPickShader(Shader *shader) {
	static const GLenum stages[] =
			{
					GL_VERTEX_SHADER
#ifdef GL_TESS_CONTROL_SHADER
					, GL_TESS_CONTROL_SHADER
#endif
#ifdef GL_TESS_EVALUATION_SHADER
					, GL_TESS_EVALUATION_SHADER
#endif
			};
	std::map<GLenum, std::string> shaderCode;
	std::map<GLenum, ref_ptr<GLuint> > shaders;

	// set picking geometry shader
	shaderCode[GL_GEOMETRY_SHADER] = pickerCode_;
	ref_ptr<GLuint> gsID = ref_ptr<GLuint>::alloc();
	GLuint &pickShaderID = *gsID.get();
	pickShaderID = pickerShader_;
	shaders[GL_GEOMETRY_SHADER] = gsID;

	// copy stages from provided shader
	for (GLuint i = 0; i < sizeof(stages) / sizeof(GLenum); ++i) {
		GLenum stage = stages[i];
		if (shader->hasStage(stage)) {
			shaderCode[stage] = shader->stageCode(stage);
			shaders[stage] = shader->stage(stage);
		}
	}

	ref_ptr<Shader> pickShader = ref_ptr<Shader>::alloc(shaderCode, shaders);

	std::list<std::string> tfNames;
	tfNames.emplace_back("pickObjectID");
	tfNames.emplace_back("pickInstanceID");
	tfNames.emplace_back("pickDepth");
	pickShader->setTransformFeedback(tfNames, GL_INTERLEAVED_ATTRIBS, GL_GEOMETRY_SHADER);

	if (!pickShader->link()) return {};

	pickShader->setInputs(shader->inputs());
	pickShader->setInput(pickObjectID_);
	pickShader->setInput(mousePosVS_);
	pickShader->setInput(mouseDirVS_);
	for (auto it = shader->textures().begin(); it != shader->textures().end(); ++it) {
		pickShader->setTexture(it->second.tex, it->second.name);
	}

	return ref_ptr<ShaderState>::alloc(pickShader);
}

GLboolean PickingGeom::add(
		const ref_ptr<Mesh> &mesh,
		const ref_ptr<Shader> &meshShader,
		const ref_ptr<State> &meshState) {
	PickMesh pickMesh;
	pickMesh.state_ = ref_ptr<State>::alloc();
	pickMesh.mesh_ = ref_ptr<Mesh>::alloc(mesh);
	do {
		pickMesh.id_ = ++pickMeshID_;
	} while (pickMesh.id_ == 0 ||
			 meshes_.count(pickMesh.id_) > 0);
	pickMesh.pickShader_ = createPickShader(meshShader.get());
	if (pickMesh.pickShader_.get() == nullptr) { return GL_FALSE; }

	StateConfigurer stateCfg;
	if (meshState.get()) {
		stateCfg.addState(meshState.get());
		pickMesh.state_->joinStates(meshState);
	}
	for (auto it = mesh->joined().begin(); it != mesh->joined().end(); ++it) {
		if (dynamic_cast<ShaderState *>(it->get())) {}
		else {
			pickMesh.state_->joinStates(*it);
			stateCfg.addState(it->get());
		}
	}

	// create VAO
	pickMesh.mesh_->updateVAO(RenderState::get(),
							  stateCfg.cfg(), pickMesh.pickShader_->shader());

	meshes_[pickMesh.id_] = pickMesh;
	meshToID_[mesh.get()] = pickMesh.id_;

	return GL_TRUE;
}

void PickingGeom::remove(Mesh *mesh) {
	if (meshToID_.count(mesh) == 0) { return; }

	GLint id = meshToID_[mesh];
	meshToID_.erase(mesh);
	meshes_.erase(id);
}

void PickingGeom::glAnimate(RenderState *rs, GLdouble dt) {
	dt_ += dt;
	if (dt_ < pickInterval_) { return; }
	dt_ = 0.0;
	update(rs);
}

void PickingGeom::pick(RenderState *rs, GLuint feedbackCount, PickData &picked) {
	rs->copyReadBuffer().push(vboRef_->bufferID());
	auto *bufferData = (PickData *) glMapBufferRange(
			GL_COPY_READ_BUFFER, vboRef_->address(), bufferSize_,
			GL_MAP_READ_BIT);
	// find pick result with max depth (camera looks in negative z direction)
	PickData *bestPicked = &picked;
	for (GLuint i = 0; i < feedbackCount; ++i) {
		PickData &picked_x = bufferData[i];
		if (picked_x.depth > bestPicked->depth) {
			bestPicked = &picked_x;
		}
	}
	picked = *bestPicked;
	glUnmapBuffer(GL_COPY_READ_BUFFER);
	rs->copyReadBuffer().pop();
}

void PickingGeom::update(RenderState *rs) {
	GL_ERROR_LOG();
#ifdef REGEN_DEBUG_BUILD
	if (rs->isTransformFeedbackAcive()) {
		REGEN_WARN("Transform Feedback was active when the Geometry Picker was updated.");
		return;
	}
#endif
	GLuint feedbackCount = 0;
	PickData picked{};
	picked.depth = -FLT_MAX;
	picked.objectID = 0;
	picked.instanceID = 0;

	{
		const Mat4f &inverseProjectionMatrix = inverseProjectionMatrix_->getVertex(0);
		const Vec2f &mouse = mouseTexco_->getVertex(0);
		// find view space mouse ray intersecting the frustum
		Vec2f mouseNDC = mouse * 2.0 - Vec2f(1.0);
		// in NDC space the ray starts at (mx,my,0) and ends at (mx,my,1)
		Vec4f mouseRayNear = inverseProjectionMatrix ^ Vec4f(mouseNDC, 0.0, 1.0);
		Vec4f mouseRayFar = inverseProjectionMatrix ^ Vec4f(mouseNDC, 1.0, 1.0);
		mouseRayNear.xyz_() /= mouseRayNear.w;
		mouseRayFar.xyz_() /= mouseRayFar.w;
		mousePosVS_->setVertex(0, mouseRayNear.xyz_());
		mouseDirVS_->setVertex(0, mouseRayNear.xyz_() - mouseRayFar.xyz_());
	}

	State::enable(rs);

	for (auto it = meshes_.begin(); it != meshes_.end(); ++it) {
		PickMesh &m = it->second;

		pickObjectID_->setVertex(0, m.id_);
		m.state_->enable(rs);
		m.pickShader_->enable(rs);

		bufferRange_.offset_ = feedbackCount * sizeof(PickData);
		bufferRange_.size_ = bufferSize_ - bufferRange_.offset_;
		bufferRange_.offset_ += vboRef_->address();
		rs->feedbackBufferRange().push(0, bufferRange_);
		glBeginQuery(GL_PRIMITIVES_GENERATED, countQuery_);
		rs->beginTransformFeedback(GL_POINTS);

		m.mesh_->enable(rs);
		m.mesh_->disable(rs);

		rs->endTransformFeedback();
		rs->feedbackBufferRange().pop(0);
		// remember number of hovered objects,
		// depth test is done later on CPU
		glEndQuery(GL_PRIMITIVES_GENERATED);
		feedbackCount += getGLQueryResult(countQuery_);

		m.pickShader_->disable(rs);
		m.state_->disable(rs);
		if (feedbackCount >= maxPickedObjects_) {
			pick(rs, maxPickedObjects_, picked);
			feedbackCount = 0;
		}
	}
	if (feedbackCount > 0) {
		pick(rs, feedbackCount, picked);
	}

	State::disable(rs);

	if (picked.objectID == 0) { // no mesh hovered
		if (pickedMesh_ != nullptr) {
			pickedMesh_ = nullptr;
			pickedInstance_ = 0;
			pickedObject_ = 0;
			emitPickEvent();
		}
	} else {
		auto it = meshes_.find(picked.objectID);
		if (it == meshes_.end()) {
			REGEN_ERROR("Invalid pick object ID " << picked.objectID <<
												  " count=" << feedbackCount <<
												  " depth=" << picked.depth <<
												  " instance=" << picked.instanceID << ".");
		} else if (picked.objectID != pickedObject_ || picked.instanceID != pickedInstance_) {
			pickedMesh_ = it->second.mesh_.get();
			pickedInstance_ = picked.instanceID;
			pickedObject_ = picked.objectID;
			emitPickEvent();
		}
	}

	GL_ERROR_LOG();
}
