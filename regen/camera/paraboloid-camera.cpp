/*
 * paraboloid-camera.cpp
 *
 *  Created on: Dec 16, 2013
 *      Author: daniel
 */

#include "paraboloid-camera.h"

using namespace regen;

ParaboloidCamera::ParaboloidCamera(
		const ref_ptr<Mesh> &mesh,
		const ref_ptr<Camera> &userCamera,
		GLboolean hasBackFace)
		: OmniDirectionalCamera(hasBackFace, GL_FALSE),
		  userCamera_(userCamera) {
	GLuint numLayer = (hasBackFace ? 2 : 1);
	shaderDefine("RENDER_TARGET", hasBackFace ? "DUAL_PARABOLOID" : "PARABOLOID");
	shaderDefine("RENDER_LAYER", REGEN_STRING(numLayer));
	shaderDefine("USE_PARABOLOID_PROJECTION", "TRUE");
	updateFrustum(180.0f, 1.0f,
				  userCamera_->near()->getVertex(0),
				  userCamera_->far()->getVertex(0),
				  GL_FALSE);

	// Set matrix array size
	view_->set_elementCount(numLayer);
	viewInv_->set_elementCount(numLayer);
	viewproj_->set_elementCount(numLayer);
	viewprojInv_->set_elementCount(numLayer);

	// Allocate matrices
	proj_->setUniformDataUntyped(nullptr);
	projInv_->setUniformDataUntyped(nullptr);
	view_->setUniformDataUntyped(nullptr);
	viewInv_->setUniformDataUntyped(nullptr);
	viewproj_->setUniformDataUntyped(nullptr);
	viewprojInv_->setUniformDataUntyped(nullptr);

	// Projection is calculated in shaders.
	proj_->setVertex(0, Mat4f::identity());
	projInv_->setVertex(0, Mat4f::identity());

	// Initialize directions.
	direction_->set_elementCount(numLayer);
	direction_->setUniformDataUntyped(nullptr);
	direction_->setVertex(0, Vec3f(0.0, 0.0, 1.0));
	if (hasBackFace_)
		direction_->setVertex(1, Vec3f(0.0, 0.0, -1.0));

	auto modelMat = mesh->findShaderInput("modelMatrix");
	modelMatrix_ = ref_ptr<ShaderInputMat4>::dynamicCast(modelMat.value().in);
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(mesh->positions());
	nor_ = ref_ptr<ShaderInput3f>::dynamicCast(mesh->normals());
	matrixStamp_ = 0;
	positionStamp_ = 0;
	normalStamp_ = 0;

	// initially update shadow maps
	update();
}

void ParaboloidCamera::update() {
	GLuint positionStamp = (pos_.get() == nullptr ? 1 : pos_->stamp());
	GLuint normalStamp = (nor_.get() == nullptr ? 1 : nor_->stamp());
	GLuint matrixStamp = (modelMatrix_.get() == nullptr ? 1 : modelMatrix_->stamp());
	if (positionStamp_ == positionStamp &&
		normalStamp_ == normalStamp &&
		matrixStamp_ == matrixStamp) { return; }

	// Compute cube center position.
	Vec3f pos = Vec3f::zero();
	if (modelMatrix_.get() != nullptr) {
		pos = (modelMatrix_->getVertex(0) ^ Vec4f(pos, 1.0f)).xyz_();
	}
	position_->setVertex(0, pos);

	if (nor_.get() != nullptr) {
		Vec3f dir = nor_->getVertex(0);
		if (modelMatrix_.get() != nullptr) {
			dir = (modelMatrix_->getVertex(0) ^ Vec4f(dir, 0.0f)).xyz_();
		}
		direction_->setVertex(0, -dir);
		if (hasBackFace_) direction_->setVertex(1, dir);
	}

	// Update view matrices
	for (int i = 0; i < 1 + hasBackFace_; ++i) {
		view_->setVertex(i, Mat4f::lookAtMatrix(
				pos, direction_->getVertex(i), Vec3f::up()));
		viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
		viewproj_->setVertex(i, view_->getVertex(i));
		viewprojInv_->setVertex(i, viewInv_->getVertex(i));

	}

	positionStamp_ = positionStamp;
	normalStamp_ = normalStamp;
	matrixStamp_ = matrixStamp;
}

void ParaboloidCamera::enable(RenderState *rs) {
	update();
	Camera::enable(rs);
}

