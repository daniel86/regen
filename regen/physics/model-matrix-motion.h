
#ifndef MODEL_MATRIX_H_
#define MODEL_MATRIX_H_

#include <regen/animations/animation.h>
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
		explicit ModelMatrixMotion(const ref_ptr<ModelTransformation> &tf, uint32_t index = 0);

		~ModelMatrixMotion() override = default;
		// Override from btMotionState
		void getWorldTransform(btTransform &worldTrans) const override;
		// Override from btMotionState
		void setWorldTransform(const btTransform &worldTrans) override;

	protected:
		ref_ptr<ModelTransformation> tf_;
		uint32_t index_;
	};

	/**
	 * Used to update the model matrix from the physics engine.
	 */
	class ModelMatrixUpdater : public Animation {
	public:
		explicit ModelMatrixUpdater(const ref_ptr<ModelTransformation> &tf);

		~ModelMatrixUpdater() override;

		ModelMatrixUpdater(const ModelMatrixUpdater &) = delete;

		auto backBuffer() { return backBuffer_; }

		auto tf() { return tf_; }

		void nextStamp() { stamp_++; }

		// Override from Animation
		void animate(GLdouble dt) override;

	protected:
		ref_ptr<ModelTransformation> tf_;
		Mat4f *backBuffer_;
		unsigned int stamp_ = 0u;
	};

	/**
	 * Used to attach a physical object to an model matrix.
	 */
	class Mat4fMotion : public btMotionState {
	public:
		Mat4fMotion(const ref_ptr<ModelMatrixUpdater> &modelMatrix, unsigned int index);

		explicit Mat4fMotion(Mat4f *glModelMatrix);

		~Mat4fMotion() override = default;

		// Override from btMotionState
		void getWorldTransform(btTransform &worldTrans) const override;
		// Override from btMotionState
		void setWorldTransform(const btTransform &worldTrans) override;
	protected:
		ref_ptr<ModelMatrixUpdater> modelMatrix_;
		Mat4f *glModelMatrix_;
		unsigned int tfIndex_;
	};
} // namespace

#endif /* MODEL_MATRIX_H_ */
