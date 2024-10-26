#ifndef GEOM_PICKING_STATE_H_
#define GEOM_PICKING_STATE_H_

#include <regen/states/state.h>
#include <regen/gl-types/fbo.h>
#include "regen/camera/camera.h"
#include "state-node.h"

namespace regen {
	/**
	 * \brief Picks an object using geometry shader.
	 */
	class GeomPicking : public StateNode {
	public:
		struct PickData {
			GLint objectID;
			GLint instanceID;
			GLfloat depth;
		};

		GeomPicking(const ref_ptr<Camera> &camera, const ref_ptr<ShaderInput2f> &mouseTexco);

		~GeomPicking() override;

		/**
		 * @return true if an object has been picked.
		 */
		auto hasPickedObject() const -> GLboolean { return hasPickedObject_; }

		/**
		 * @return the picked object.
		 */
		auto* pickedObject() const { return hasPickedObject_ ? &pickedObject_ : nullptr; }

		// override
		void traverse(RenderState *state) override;

	protected:
		ref_ptr<Camera> camera_;
		GLuint maxPickedObjects_;
		GLdouble pickInterval_;

		GLboolean hasPickedObject_;
		PickData pickedObject_;

		ref_ptr<ShaderInput2f> mouseTexco_;
		ref_ptr<ShaderInput3f> mousePosVS_;
		ref_ptr<ShaderInput3f> mouseDirVS_;

		ref_ptr<ShaderInput1i> pickObjectID_;
		ref_ptr<ShaderInput1i> pickInstanceID_;
		ref_ptr<ShaderInput1f> pickDepth_;

		ref_ptr<FeedbackSpecification> feedbackState_;
		ref_ptr<VBO> feedbackBuffer_;
		ref_ptr<BufferRange> bufferRange_;
		VBOReference vboRef_;
		GLuint bufferSize_;

		GLdouble dt_;

		void pick(RenderState *rs, GLuint feedbackCount);

		void updateMouse();
	};
} // namespace

#endif /* GEOM_PICKING_STATE_H_ */
