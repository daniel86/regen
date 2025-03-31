#ifndef REGEN_BOUNDING_BOX_BUFFER_H_
#define REGEN_BOUNDING_BOX_BUFFER_H_

#include <regen/gl-types/ssbo.h>
#include <regen/gl-types/pbo.h>
#include <regen/shapes/bounds.h>

namespace regen {
	/**
	 * \brief A buffer object that stores the bounding box of a mesh.
	 *
	 * The bounding box is supposed to be updated by the GPU.
	 * This buffer then uses a PBO to read back the bounding box values, and provide
	 * it to some CPU code.
	 * The update does not happen automatically, it must be triggered by the user
	 * by calling updateBoundingBox().
	 */
	class BBoxBuffer : public SSBO {
	public:
		/**
		 * Default constructor.
		 * @param name the name of the buffer block in shaders.
		 */
		explicit BBoxBuffer(const std::string &name="BoundingBox");

		~BBoxBuffer() override = default;

		/**
		 * @return the bounding box of the mesh.
		 */
		auto &bbox() { return bbox_; }

		/**
		 * Read the bounding box values from the GPU.
		 * @param rs the render state.
		 * @return true if the bounding box has changed, false otherwise.
		 */
		bool updateBoundingBox(RenderState *rs);

	private:
		Bounds<Vec3f> bbox_;
		ref_ptr<PBO> bboxPBO_;

	};
} // namespace

#endif /* REGEN_BOUNDING_BOX_BUFFER_H_ */
