
#ifndef MODEL_MATRIX_H_
#define MODEL_MATRIX_H_

#include <regen/states/model-transformation.h>
#include <regen/utility/ref-ptr.h>

#include <btBulletDynamicsCommon.h>

namespace regen {
	/**
	 * Used to attach a physical object to an model matrix.
	 */
	class ModelMatrixMotion : public btMotionState {
	public:
		/**
		 * @param modelMatrix The model matrix.
		 * @param index Instance index or 0.
		 */
		explicit ModelMatrixMotion(const ref_ptr<ShaderInputMat4> &modelMatrix, GLuint index = 0);

		~ModelMatrixMotion() override = default;
		// Override from btMotionState
		void getWorldTransform(btTransform &worldTrans) const override;
		// Override from btMotionState
		void setWorldTransform(const btTransform &worldTrans) override;

	protected:
		ref_ptr<ShaderInputMat4> modelMatrix_;
		GLuint index_;
	};

	/**
	 * Used to attach a physical object to an model matrix.
	 */
	class Mat4fMotion : public btMotionState {
	public:
		/**
		 * @param glModelMatrix The model matrix.
		 */
		explicit Mat4fMotion(Mat4f *glModelMatrix);
		~Mat4fMotion() override = default;
		// Override from btMotionState
		void getWorldTransform(btTransform &worldTrans) const override;
		// Override from btMotionState
		void setWorldTransform(const btTransform &worldTrans) override;
	protected:
		Mat4f *glModelMatrix_;
	};
} // namespace

#endif /* MODEL_MATRIX_H_ */
