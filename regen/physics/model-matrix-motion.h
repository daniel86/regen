
#ifndef MODEL_MATRIX_H_
#define MODEL_MATRIX_H_

#include <regen/states/model-transformation.h>
#include <regen/utility/ref-ptr.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
  class ModelMatrixMotion : public btMotionState
  {
    public:
      ModelMatrixMotion(
          const btTransform &initialpos,
          const ref_ptr<ModelTransformation> &modelMatrix);
      virtual ~ModelMatrixMotion() {};

      virtual void getWorldTransform(btTransform &worldTrans) const;
      virtual void setWorldTransform(const btTransform &worldTrans);

    protected:
      /** ... */
      ref_ptr<ModelTransformation> modelMatrix_;
      /** ... */
      btTransform transform_;
  };
} // namespace

#endif /* MODEL_MATRIX_H_ */
