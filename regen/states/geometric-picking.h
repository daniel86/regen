#ifndef GEOM_PICKING_STATE_H_
#define GEOM_PICKING_STATE_H_

#include <regen/states/state.h>
#include "regen/textures/fbo.h"
#include "regen/camera/camera.h"
#include "state-node.h"
#include "pick-data.h"
#include "regen/buffer/pbo.h"
#include "regen/buffer/staging-buffer.h"

namespace regen {
	/**
	 * \brief Picks an object using geometry shader.
	 */
	class GeomPicking : public StateNode {
	public:
		GeomPicking(const ref_ptr<Camera> &camera, const ref_ptr<ShaderInput2f> &mouseTexco);

		~GeomPicking() override;

		static ref_ptr<GeomPicking> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @return true if an object has been picked.
		 */
		auto hasPickedObject() const -> GLboolean { return hasPickedObject_; }

		/**
		 * @return the picked object.
		 */
		auto *pickedObject() const { return hasPickedObject_ ? &pickedObject_ : nullptr; }

		// override
		void traverse(RenderState *state) override;

	protected:
		ref_ptr<Camera> camera_;
		GLuint maxPickedObjects_;

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
		ref_ptr<BufferReference> vboRef_;
		ref_ptr<StagingStructBuffer<PickData>> pickMapping_;
		BufferRange feedbackRange_;
		GLuint bufferSize_;

		void updateMouse();
	};
} // namespace

#endif /* GEOM_PICKING_STATE_H_ */
