#ifndef REGEN_ATOMIC_COUNTER_H_
#define REGEN_ATOMIC_COUNTER_H_

#include <regen/buffer/buffer-object.h>
#include "regen/shapes/bounds.h"

namespace regen {
	/**
	 * @brief An atomic counter buffer object.
	 * @details An atomic counter buffer object is used to store unsigned integer values that can be read and written atomically.
	 */
	class AtomicCounter : public BufferObjectT<ATOMIC_COUNTER_BUFFER> {
	public:
		AtomicCounter();

		~AtomicCounter() override = default;

		AtomicCounter(const AtomicCounter &) = delete;
	};

	/**
	 * @brief An atomic counter buffer object for bounding boxes.
	 */
	class BoundingBoxCounter : public AtomicCounter {
	public:
		BoundingBoxCounter();

		~BoundingBoxCounter() override = default;

		BoundingBoxCounter(const BoundingBoxCounter &) = delete;

		Bounds<Vec3f> &updateBounds();

		Bounds<Vec3f> &bounds() { return bounds_; }

	protected:
		uint32_t initialData_[6];
		Bounds<Vec3f> bounds_;
		ref_ptr<BufferReference> ref_;
	};
} // namespace

#endif /* REGEN_ATOMIC_COUNTER_H_ */
