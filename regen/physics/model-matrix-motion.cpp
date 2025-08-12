#include "model-matrix-motion.h"

using namespace regen;

ModelMatrixMotion::ModelMatrixMotion(const ref_ptr<ModelTransformation> &tf, uint32_t index)
		: tf_(tf), index_(index) {
	// validate index
	if (index_ >= static_cast<uint32_t>(tf_->modelMat()->numInstances())) {
		REGEN_WARN("Invalid matrix index " << index_ << ". Using 0 instead.");
		index_ = 0u;
	}
}

void ModelMatrixMotion::getWorldTransform(btTransform &worldTrans) const {
	auto regenData = tf_->modelMat()->mapClientData<Mat4f>(BUFFER_GPU_READ);
	auto &regenMat = regenData.r[index_];
	worldTrans.setFromOpenGLMatrix((const btScalar*) &regenMat.x);
}

void ModelMatrixMotion::setWorldTransform(const btTransform &worldTrans) {
	auto regenData = tf_->modelMat()->mapClientVertex<Mat4f>(BUFFER_GPU_WRITE, index_);
	worldTrans.getOpenGLMatrix((btScalar*) &regenData.w.x);
}


ModelMatrixUpdater::ModelMatrixUpdater(const ref_ptr<ModelTransformation> &tf)
		: Animation(false, true),
		  tf_(tf) {
	backBuffer_ = new Mat4f[tf_->modelMat()->numInstances()];
	auto regenData = tf_->modelMat()->mapClientDataRaw(BUFFER_GPU_READ);
	std::memcpy(backBuffer_, regenData.r, tf_->modelMat()->inputSize());
}

ModelMatrixUpdater::~ModelMatrixUpdater() {
	delete[] backBuffer_;
}

void ModelMatrixUpdater::animate(GLdouble dt) {
	if (stamp_ == tf_->modelMat()->stampOfReadData()) return;
	stamp_ = tf_->modelMat()->stampOfReadData();
	auto regenData = tf_->modelMat()->mapClientDataRaw(BUFFER_GPU_WRITE);
	std::memcpy(regenData.w, backBuffer_, tf_->modelMat()->inputSize());
}


Mat4fMotion::Mat4fMotion(const ref_ptr<ModelMatrixUpdater> &modelMatrix, GLuint index)
		: modelMatrix_(modelMatrix),
		  glModelMatrix_(modelMatrix->backBuffer() + index),
		  tfIndex_(index) {
}

Mat4fMotion::Mat4fMotion(Mat4f *glModelMatrix)
		: glModelMatrix_(glModelMatrix),
		  tfIndex_(0) {
}

void Mat4fMotion::getWorldTransform(btTransform &worldTrans) const {
	if (modelMatrix_.get()) {
		auto regenData = modelMatrix_->tf()->modelMat()->mapClientVertex<Mat4f>(BUFFER_GPU_READ, tfIndex_);
		worldTrans.setFromOpenGLMatrix((const btScalar *) &regenData.r);
	} else {
		worldTrans.setFromOpenGLMatrix((const btScalar *) glModelMatrix_);
	}
}

void Mat4fMotion::setWorldTransform(const btTransform &worldTrans) {
	worldTrans.getOpenGLMatrix((btScalar *) glModelMatrix_);
	if (modelMatrix_.get()) {
		modelMatrix_->nextStamp();
	}
}
