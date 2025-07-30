#include "model-matrix-motion.h"

using namespace regen;

ModelMatrixMotion::ModelMatrixMotion(const ref_ptr<ModelTransformation> &tf, uint32_t index)
		: tf_(tf), index_(index) {
	// validate index
	if (index_ >= static_cast<uint32_t>(tf_->numInstances())) {
		REGEN_WARN("Invalid matrix index " << index_ << ". Using 0 instead.");
		index_ = 0u;
	}
}

void ModelMatrixMotion::getWorldTransform(btTransform &worldTrans) const {
	auto &regenMat = tf_->modelMat(index_);
	worldTrans.setFromOpenGLMatrix((const btScalar*) &regenMat.x);
}

void ModelMatrixMotion::setWorldTransform(const btTransform &worldTrans) {
	worldTrans.getOpenGLMatrix((btScalar*) &tmpMat_.x);
	tf_->setModelMat(index_, tmpMat_);
	tf_->updateShaderData();
}


ModelMatrixUpdater::ModelMatrixUpdater(const ref_ptr<ModelTransformation> &tf)
		: Animation(false, true),
		  tf_(tf) {
	backBuffer_ = new Mat4f[tf_->numInstances()];
	std::memcpy(
		backBuffer_,
		tf_->modelMat().data(),
		tf_->modelMat().size() * sizeof(Mat4f));
}

ModelMatrixUpdater::~ModelMatrixUpdater() {
	delete[] backBuffer_;
}

void ModelMatrixUpdater::animate(GLdouble dt) {
	if (stamp_ == tf_->stamp()) return;
	stamp_ = tf_->stamp();
	tf_->setModelMat(backBuffer_);
	tf_->updateShaderData();
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
		auto &regenData = modelMatrix_->tf()->modelMat(tfIndex_);
		worldTrans.setFromOpenGLMatrix((const btScalar *) &regenData.x);
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
