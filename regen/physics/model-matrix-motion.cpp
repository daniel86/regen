
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


Mat4fMotion::Mat4fMotion(Mat4f *glModelMatrix)
		: glModelMatrix_(glModelMatrix) {
}

void Mat4fMotion::getWorldTransform(btTransform &worldTrans) const {
	worldTrans.setFromOpenGLMatrix((btScalar *) glModelMatrix_);
}

void Mat4fMotion::setWorldTransform(const btTransform &worldTrans) {
	worldTrans.getOpenGLMatrix((btScalar *) glModelMatrix_);
}
