
#include "model-matrix-motion.h"
using namespace regen;

ModelMatrixMotion::ModelMatrixMotion(const btTransform &initialpos,
    const ref_ptr<ModelTransformation> &modelMatrix)
: modelMatrix_(modelMatrix),
  transform_(initialpos)
{
}

void ModelMatrixMotion::getWorldTransform(btTransform &worldTrans) const
{ worldTrans = transform_; }

void ModelMatrixMotion::setWorldTransform(const btTransform &worldTrans)
{
  ShaderInputMat4* modelMatrix = modelMatrix_->modelMatPtr();
  worldTrans.getOpenGLMatrix((btScalar*)modelMatrix->dataPtr());
  modelMatrix->nextStamp();
}
