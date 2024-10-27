
#include "model-matrix-motion.h"

using namespace regen;

#include <regen/utility/logging.h>

ModelMatrixMotion::ModelMatrixMotion(
		const ref_ptr<ShaderInputMat4> &modelMatrix, GLuint index)
		: modelMatrix_(modelMatrix),
		  index_(index) {
	// validate index
	if (index_ >= modelMatrix_->numInstances()) {
		REGEN_WARN("Invalid matrix index " << index_ << ". Using 0 instead.");
		index_ = 0u;
	}
	auto *matrices = (Mat4f *) modelMatrix_->clientDataPtr();
	auto *mat = &matrices[index_];
	transform_.setFromOpenGLMatrix((btScalar *) mat);
}

void ModelMatrixMotion::getWorldTransform(btTransform &worldTrans) const {
	auto *matrices = (Mat4f *) modelMatrix_->clientDataPtr();
	auto *mat = &matrices[index_];
	worldTrans.setFromOpenGLMatrix((btScalar *) mat);
}

void ModelMatrixMotion::setWorldTransform(const btTransform &worldTrans) {
	auto *matrices = (Mat4f *) modelMatrix_->clientDataPtr();
	auto *mat = &matrices[index_];
	worldTrans.getOpenGLMatrix((btScalar *) mat);
	modelMatrix_->nextStamp();
}


CharacterMotion::CharacterMotion(
	const ref_ptr<ShaderInputMat4> &modelMatrix, GLuint index)
	: ModelMatrixMotion(modelMatrix, index) {}

void CharacterMotion::getWorldTransform(btTransform &worldTrans) const {
	auto *matrices = (Mat4f *) modelMatrix_->clientDataPtr();
	auto *mat = &matrices[index_];
	worldTrans.setFromOpenGLMatrix((btScalar *) mat);
}

void CharacterMotion::setWorldTransform(const btTransform &worldTrans) {
	btTransform uprightTransform = worldTrans;
	btQuaternion rotation = worldTrans.getRotation();
	btVector3 up(0, 1, 0);
	btVector3 forward = btMatrix3x3(rotation) * btVector3(0, 0, 1);
	btVector3 right = up.cross(forward).normalized();
	forward = right.cross(up).normalized();
	btMatrix3x3 basis;
	basis[0] = right;
	basis[1] = up;
	basis[2] = forward;
	uprightTransform.setBasis(basis);
	ModelMatrixMotion::setWorldTransform(uprightTransform);
}
