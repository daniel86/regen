#ifndef REGEN_BATCH_OF_SHAPES_H_
#define REGEN_BATCH_OF_SHAPES_H_

#include "regen/shapes/bounding-sphere.h"
#include "regen/utility/aligned-array.h"

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
		void resize(uint32_t newCapacity) {
			resizeFun(*this, newCapacity);
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
		void (*resizeFun)(BatchOfShapes&, uint32_t) = nullptr;
		void (*pushFun)(BatchOfShapes&, const BoundingShape &, uint32_t) = nullptr;
	};

	/**
	 * @brief Batch structure for sphere shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple spheres in a batched manner.
	 */
	struct BatchOfSpheres : BatchOfShapes {
		BatchOfSpheres() : BatchOfShapes() {
			resizeFun = &BatchOfSpheres::doResize;
			pushFun = &BatchOfSpheres::doPush;
		}
		~BatchOfSpheres() override = default;
		// Queued sphere center position + radius
		AlignedArray<float> posX;
		AlignedArray<float> posY;
		AlignedArray<float> posZ;
		AlignedArray<float> radius;

	protected:
		static void doResize(BatchOfShapes &batch, uint32_t newCapacity);
		static void doPush(BatchOfShapes &self, const BoundingShape &shape, uint32_t index);
	};

	/**
	 * @brief Batch structure for axis-aligned bounding box (AABB) shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple AABBs in a batched manner.
	 */
	struct BatchOfAABBs : BatchOfShapes {
		BatchOfAABBs() : BatchOfShapes() {
			resizeFun = &BatchOfAABBs::doResize;
			pushFun = &BatchOfAABBs::doPush;
		}
		~BatchOfAABBs() override = default;
		// Queued AABB min/max points
		AlignedArray<float> minX;
		AlignedArray<float> minY;
		AlignedArray<float> minZ;
		AlignedArray<float> maxX;
		AlignedArray<float> maxY;
		AlignedArray<float> maxZ;

	protected:
		static void doResize(BatchOfShapes &self, uint32_t newCapacity);
		static void doPush(BatchOfShapes &self, const BoundingShape &shape, uint32_t index);
	};

	/**
	 * @brief Batch structure for oriented bounding box (OBB) shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple OBBs in a batched manner.
	 */
	struct BatchOfOBBs : BatchOfShapes {
		// NOTE: Frustum shapes do not support batched intersection tests yet.
		BatchOfOBBs() : BatchOfShapes() {
			resizeFun = &BatchOfOBBs::doResize;
			pushFun = &BatchOfOBBs::doPush;
		}
		~BatchOfOBBs() override = default;
	protected:
		static void doResize(BatchOfShapes&, uint32_t) {}
		static void doPush(BatchOfShapes&, const BoundingShape&, uint32_t) {}
	};

	/**
	 * @brief Batch structure for frustum shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple frustums in a batched manner.
	 */
	struct BatchOfFrustums : BatchOfShapes {
		// NOTE: Frustum shapes do not support batched intersection tests yet.
		BatchOfFrustums() : BatchOfShapes() {
			resizeFun = &BatchOfFrustums::doResize;
			pushFun = &BatchOfFrustums::doPush;
		}
		~BatchOfFrustums() override = default;
	protected:
		static void doResize(BatchOfShapes&, uint32_t) {}
		static void doPush(BatchOfShapes&, const BoundingShape&, uint32_t) {}
	};
} // namespace

#endif /* REGEN_BATCH_OF_SHAPES_H_ */
