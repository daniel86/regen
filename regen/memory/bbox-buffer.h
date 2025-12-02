#ifndef REGEN_BOUNDING_BOX_BUFFER_H_
#define REGEN_BOUNDING_BOX_BUFFER_H_

#include "ssbo.h"
#include "staging-buffer.h"
#include "regen/shapes/bounds.h"

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
		explicit BBoxBuffer(
			const Bounds<Vec3f> &initialBounds,
			std::string_view name="BoundingBox");

		~BBoxBuffer() override = default;

		/**
		 * @return the bounding box of the mesh.
		 */
		auto &bbox() { return bbox_; }

		/**
		 * Clears the bounding box buffer to zero.
		 * This must be called each frame before computing the bounding box,
		 * as this resets some counters to zero that are used in compute.
		 */
		void clear();

		/**
		 * Read the bounding box values from the GPU.
		 * @return true if the bounding box has changed, false otherwise.
		 */
		bool updateBoundingBox();

	protected:
		Bounds<Vec3f> bbox_;
		Vec3f bboxMin_, bboxMax_;
		ref_ptr<BufferReference> clearRef_;
	};
} // namespace

#endif /* REGEN_BOUNDING_BOX_BUFFER_H_ */
