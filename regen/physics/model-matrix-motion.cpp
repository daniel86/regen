
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
	auto regenData = modelMatrix_->mapClientData<Mat4f>(ShaderData::READ);
	auto &regenMat = regenData.r[index_];
	worldTrans.setFromOpenGLMatrix((const btScalar*) &regenMat.x);
}

void ModelMatrixMotion::setWorldTransform(const btTransform &worldTrans) {
	auto regenData = modelMatrix_->mapClientData<Mat4f>(ShaderData::WRITE | ShaderData::INDEX);
	auto &regenMat = regenData.w[index_];
	worldTrans.getOpenGLMatrix((btScalar*) &regenMat.x);
}


ModelMatrixUpdater::ModelMatrixUpdater(const ref_ptr<ShaderInputMat4> &modelMatrix)
		: Animation(false, true),
		  modelMatrix_(modelMatrix) {
	backBuffer_ = new Mat4f[modelMatrix->numInstances()];
}

ModelMatrixUpdater::~ModelMatrixUpdater() {
	delete[] backBuffer_;
}

void ModelMatrixUpdater::animate(GLdouble dt) {
	if (stamp_ == modelMatrix_->stamp()) return;
	stamp_ = modelMatrix_->stamp();
	auto regenData = modelMatrix_->mapClientDataRaw(ShaderData::WRITE);
	std::memcpy(regenData.w, backBuffer_, modelMatrix_->inputSize());
}


Mat4fMotion::Mat4fMotion(const ref_ptr<ModelMatrixUpdater> &modelMatrix, GLuint index)
		: modelMatrix_(modelMatrix),
		  glModelMatrix_(modelMatrix->backBuffer() + index) {
}

Mat4fMotion::Mat4fMotion(Mat4f *glModelMatrix)
		: glModelMatrix_(glModelMatrix) {
}

void Mat4fMotion::getWorldTransform(btTransform &worldTrans) const {
	worldTrans.setFromOpenGLMatrix((const btScalar *) glModelMatrix_);
}

void Mat4fMotion::setWorldTransform(const btTransform &worldTrans) {
	worldTrans.getOpenGLMatrix((btScalar *) glModelMatrix_);
	if (modelMatrix_.get()) {
		modelMatrix_->nextStamp();
	}
}
