
#include "model-matrix-motion.h"
using namespace regen;

#include <regen/utility/logging.h>

ModelMatrixMotion::ModelMatrixMotion(
    const ref_ptr<ShaderInputMat4> &modelMatrix, GLuint index)
: modelMatrix_(modelMatrix),
  index_(index)
{
  // validate index
  if(index_ >= modelMatrix_->numInstances()) {
    REGEN_WARN("Invalid matrix index " << index_ << ". Using 0 instead.");
    index_ = 0u;
  }
  Mat4f *matrices = (Mat4f*)modelMatrix_->clientDataPtr();
  Mat4f *mat = &matrices[index_];
  transform_.setFromOpenGLMatrix((btScalar*)mat);
}

void ModelMatrixMotion::getWorldTransform(btTransform &worldTrans) const
{
  worldTrans = transform_;
}

void ModelMatrixMotion::setWorldTransform(const btTransform &worldTrans)
{
  Mat4f *matrices = (Mat4f*)modelMatrix_->clientDataPtr();
  Mat4f *mat = &matrices[index_];
  worldTrans.getOpenGLMatrix((btScalar*)mat);
  modelMatrix_->nextStamp();
}
