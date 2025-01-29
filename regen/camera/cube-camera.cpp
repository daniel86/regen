/*
 * cube-camera.cpp
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#include "cube-camera.h"

using namespace regen;

CubeCamera::CubeCamera(
		const ref_ptr<Mesh> &mesh,
		const ref_ptr<Camera> &userCamera)
		: OmniDirectionalCamera(GL_TRUE, GL_FALSE),
		  userCamera_(userCamera) {
	shaderDefine("RENDER_TARGET", "CUBE");
	shaderDefine("RENDER_LAYER", "6");
	updateFrustum(
			userCamera_->fov()->getVertex(0),
			userCamera_->aspect()->getVertex(0),
			userCamera_->near()->getVertex(0),
			userCamera_->far()->getVertex(0),
			GL_FALSE);

	// Set matrix array size
	view_->set_elementCount(6);
	viewInv_->set_elementCount(6);
	viewproj_->set_elementCount(6);
	viewprojInv_->set_elementCount(6);

	// Allocate matrices
	proj_->setUniformDataUntyped(nullptr);
	projInv_->setUniformDataUntyped(nullptr);
	view_->setUniformDataUntyped(nullptr);
	viewInv_->setUniformDataUntyped(nullptr);
	viewproj_->setUniformDataUntyped(nullptr);
	viewprojInv_->setUniformDataUntyped(nullptr);

	// Initialize directions
	direction_->set_elementCount(6);
	direction_->setUniformDataUntyped(nullptr);
	const Vec3f *dir = Mat4f::cubeDirections();
	for (auto i = 0; i < 6; ++i) {
		direction_->setVertex(i, dir[i]);
	}

	auto modelMat = mesh->findShaderInput("modelMatrix");
	modelMatrix_ = ref_ptr<ShaderInputMat4>::dynamicCast(modelMat.value().in);
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(mesh->positions());
	matrixStamp_ = 0;
	positionStamp_ = 0;

	for (auto i = 0; i < 6; ++i) isCubeFaceVisible_[i] = GL_TRUE;

	// initially update shadow maps
	update();
}

void CubeCamera::set_isCubeFaceVisible(GLenum face, GLboolean visible) {
	isCubeFaceVisible_[face - GL_TEXTURE_CUBE_MAP_POSITIVE_X] = visible;
}

void CubeCamera::update() {
	GLuint positionStamp = (pos_.get() == nullptr ? 1 : pos_->stamp());
	GLuint matrixStamp = (modelMatrix_.get() == nullptr ? 1 : modelMatrix_->stamp());
	if (positionStamp_ == positionStamp && matrixStamp_ == matrixStamp) { return; }
	const GLfloat &near = userCamera_->near()->getVertex(0);
	const GLfloat &far = userCamera_->far()->getVertex(0);

	// Compute cube center position.
	Vec3f pos = Vec3f::zero();
	if (modelMatrix_.get() != nullptr) {
		pos = (modelMatrix_->getVertex(0) ^ Vec4f(pos, 1.0f)).xyz_();
	}
	position_->setVertex(0, pos);

	// Update projection matrix.
	updateFrustum(1.0f, 90.0f, near, far, GL_FALSE);
	updateProjection();

	// Update view and view-projection matrix.
	Mat4f::cubeLookAtMatrices(pos, (Mat4f *) view_->clientDataPtr());
	view_->nextStamp();

	for (auto i = 0; i < 6; ++i) {
		if (!isCubeFaceVisible_[i]) { continue; }
		viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
		updateViewProjection(0, i);
	}

	positionStamp_ = positionStamp;
	matrixStamp_ = matrixStamp;
}

void CubeCamera::enable(RenderState *rs) {
	update();
	Camera::enable(rs);
}
