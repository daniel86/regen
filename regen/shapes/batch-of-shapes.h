#ifndef REGEN_BATCH_OF_SHAPES_H_
#define REGEN_BATCH_OF_SHAPES_H_

#include <vector>
#include <cstdint>
#include <regen/memory/aligned-array.h>
#include <regen/shapes/bounding-shape.h>

namespace regen {
	class BoundingShape;

	/**
	 * @brief Base structure for a batch of shapes for batched intersection tests.
	 * This is useful for laying out shape data in contiguous memory for SIMD processing.
	 */
	struct BatchOfShapes {
		explicit BatchOfShapes(uint32_t numArrays) : soaData_(numArrays) {}

		virtual ~BatchOfShapes() = default;

		BatchOfShapes(const BatchOfShapes&) = delete;
		BatchOfShapes& operator=(const BatchOfShapes&) = delete;

		std::vector<AlignedArray<float>> soaData_{}; // Structure of Arrays data for the batch
		// The current capacity of the batch.
		// Note that resizing is expensive and should be minimized.
		uint32_t capacity = 0u;

		/**
		 * @brief Resize the batch to a new capacity.
		 * @param newCapacity The new capacity for the batch.
		 */
		void resize(uint32_t newCapacity, bool preserveData = false) {
			if (newCapacity != capacity) {
				capacity = newCapacity;
				for (auto &array : soaData_) {
					array.resize(newCapacity, preserveData);
				}
			}
		}
	};
} // namespace regen

#endif /* REGEN_BATCH_OF_SHAPES_H_ */
