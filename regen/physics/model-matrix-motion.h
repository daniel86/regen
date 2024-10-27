
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
	class ModelMatrixMotion : public btMotionState {
	public:
		/**
		 * Default Constructor.
		 * Initially sets physical object transform to the
		 * current model matrix.
		 * @param modelMatrix The model matrix.
		 * @param index Instance index or 0.
		 */
		explicit ModelMatrixMotion(
				const ref_ptr<ShaderInputMat4> &modelMatrix,
				GLuint index = 0);

		~ModelMatrixMotion() override = default;

		/**
		 * Synchronization from regen to physics engine.
		 * @param worldTrans transformation output for physics engine.
		 */
		void getWorldTransform(btTransform &worldTrans) const override;

		/**
		 * Synchronization from physics engine to regen.
		 * @param worldTrans transformation input from physics engine.
		 */
		void setWorldTransform(const btTransform &worldTrans) override;

	protected:
		ref_ptr<ShaderInputMat4> modelMatrix_;
		btTransform transform_;
		GLuint index_;
	};

	/**
	 * Attach a physical object to a character model matrix.
	 */
	class CharacterMotion : public ModelMatrixMotion {
	public:
		explicit CharacterMotion(
				const ref_ptr<ShaderInputMat4> &modelMatrix,
				GLuint index = 0);

		~CharacterMotion() override = default;

		void getWorldTransform(btTransform &worldTrans) const override;

		void setWorldTransform(const btTransform &worldTrans) override;
	};
} // namespace

#endif /* MODEL_MATRIX_H_ */
