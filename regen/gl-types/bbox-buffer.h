#ifndef REGEN_BOUNDING_BOX_BUFFER_H_
#define REGEN_BOUNDING_BOX_BUFFER_H_

#include <regen/gl-types/ssbo.h>
#include <regen/gl-types/buffer-mapping.h>
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
		struct BoundingBoxBlock {
			Vec4i positiveMin;
			Vec4i positiveMax;
			Vec4i negativeMin;
			Vec4i negativeMax;
			Vec4i positiveFlags;
			Vec4i negativeFlags;
		};

		Bounds<Vec3f> bbox_;
		ref_ptr<BufferStructMapping<BoundingBoxBlock>> bboxMapping_;
		Vec3f bboxMin_, bboxMax_;

	};
} // namespace

#endif /* REGEN_BOUNDING_BOX_BUFFER_H_ */
