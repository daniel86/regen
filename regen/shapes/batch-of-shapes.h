#ifndef REGEN_BATCH_OF_SHAPES_H_
#define REGEN_BATCH_OF_SHAPES_H_

#include "regen/shapes/bounding-shape.h"

namespace regen {
	/**
	 * @brief Base structure for a batch of shapes for batched intersection tests.
	 * This is useful for laying out shape data in contiguous memory for SIMD processing.
	 */
	struct BatchOfShapes {
		BatchOfShapes() = default;
		virtual ~BatchOfShapes() = default;

		// The current capacity of the batch.
		// Note that resizing is expensive and should be minimized.
		uint32_t capacity = 0u;

		/**
		 * @brief Resize the batch to a new capacity.
		 * @param newCapacity The new capacity for the batch.
		 */
		void resize(uint32_t newCapacity, bool preserveData = false) {
			resizeFun(*this, newCapacity, preserveData);
			capacity = newCapacity;
		}

		/**
		 * @brief Push a shape into the batch at the specified index.
		 * Make sure the batch has enough capacity before pushing.
		 * @param shape The shape to push into the batch.
		 * @param index The index at which to push the shape.
		 */
		void push(const BoundingShape &shape, uint32_t index) {
			pushFun(*this, shape, index);
		}

	protected:
		// TODO: Avoid function pointers, use some templating instead
		void (*resizeFun)(BatchOfShapes&, uint32_t, bool) = nullptr;
		void (*pushFun)(BatchOfShapes&, const BoundingShape &, uint32_t) = nullptr;
	};
} // namespace

#endif /* REGEN_BATCH_OF_SHAPES_H_ */
