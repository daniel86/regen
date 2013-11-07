
#ifndef MODEL_MATRIX_H_
#define MODEL_MATRIX_H_

#include <regen/states/model-transformation.h>
#include <regen/utility/ref-ptr.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
  /**
   * Used to attach a physical object to an
   * model matrix.
   */
  class ModelMatrixMotion : public btMotionState
  {
    public:
      /**
       * Default Constructor.
       * Initially sets physical object transform to the
       * current model matrix.
       * @param modelMatrix The model matrix.
       * @param index Instance index or 0.
       */
      ModelMatrixMotion(
          const ref_ptr<ShaderInputMat4> &modelMatrix,
          GLuint index=0);
      virtual ~ModelMatrixMotion() {};

      // Override
      virtual void getWorldTransform(btTransform &worldTrans) const;
      virtual void setWorldTransform(const btTransform &worldTrans);

    protected:
      ref_ptr<ShaderInputMat4> modelMatrix_;
      btTransform transform_;
      GLuint index_;
  };
} // namespace

#endif /* MODEL_MATRIX_H_ */
