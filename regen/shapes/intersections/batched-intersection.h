#ifndef REGEN_BATCHED_INTERSECTION_H_
#define REGEN_BATCHED_INTERSECTION_H_

#include "batch-of-shapes.h"
#include "shape-data.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/bounding-sphere.h"
#include "regen/shapes/frustum.h"
#include "regen/shapes/obb.h"
#include "regen/shapes/spatial-index.h"

namespace regen {
	/**
	 * Enumeration of different intersection shape types.
	 */
	enum class IntersectionShapeType {
		SPHERE = 0,
		AABB,
		OBB,
		FRUSTUM,
		LAST // keep last
	};

	/**
	 * Traits structure for intersection tests between different shape types.
	 */
	template<IntersectionShapeType Test, IntersectionShapeType Indexed> struct IntersectionTraits;

	/**
	 * Different cases of intersection tests handled by BatchedIntersection.
	 * These roughly correspond to different execution paths.
	 */
	enum class IntersectionCaseType {
		FRUSTUM_SPHERES = 0,
		FRUSTUM_AABBs,
		FRUSTUM_OBBs,
		FRUSTUM_FRUSTUMS,
		SPHERE_SPHERES,
		SPHERE_AABBs,
		SPHERE_OBBs,
		SPHERE_FRUSTUMS,
		AABB_SPHERES,
		AABB_AABBs,
		AABB_OBBs,
		AABB_FRUSTUMS,
		OBB_SPHERES,
		OBB_AABBs,
		OBB_OBBs,
		OBB_FRUSTUMS,
		LAST // keep last
	};

	/**
	 * @brief Structure representing a case of intersection tests.
	 *
	 * This structure holds the necessary data and callbacks for performing
	 * intersection tests between a test shape and multiple indexed shapes.
	 */
	struct BatchedIntersectionCase {
		BatchedIntersectionCase() = default;
		virtual ~BatchedIntersectionCase() = default;
		// Test shape
		const BoundingShape *testShape = nullptr;
		// Number of queued shapes
		uint32_t numQueued = 0;
		// Indices of queued shapes
		std::vector<uint32_t> queuedIndices;
		// Vector over all shapes in the index
		std::vector<ref_ptr<BoundingShape>> const *indexedShapes = nullptr;
		// Memory for the test/indexed shapes
		IntersectionShapeData *shapeData = nullptr;
		BatchOfShapes *batchData = nullptr;
		// Hit buffer for storing successful intersections by index
		HitBuffer *hits = nullptr;

		/**
		 * @brief Initialize the intersection case with a test shape and callback.
		 * @param shape The test shape to use for intersection tests.
		 * @param cb The callback to invoke for each intersecting shape.
		 * @param capacity The maximum number of shapes to queue before flushing.
		 */
		void init(const BoundingShape &shape, uint32_t capacity);

		/**
		 * @brief Insert a shape into the intersection case for testing.
		 * @param shape The shape to insert.
		 * @param shapeIdx The index of the shape in the indexed shapes vector.
		 */
		void push(const ref_ptr<BoundingShape> &shape, uint32_t shapeIdx);

		/**
		 * @brief Flush the queued shapes, processing all queued intersection tests.
		 */
		inline void flush() {
			if (numQueued != 0) {
				doFlush(*this);
				numQueued = 0;
			}
		}

		/**
		 * @brief No-operation insert function.
		 */
		static void case_NOOP(BatchedIntersectionCase&, const BoundingShape&) {}

	protected:
		// Function pointers for flush and insert operations
		void (*doInit)(BatchedIntersectionCase&, const BoundingShape&) = &case_NOOP;
		void (*doFlush)(BatchedIntersectionCase &td) = nullptr;

	private:
		friend class BatchedIntersectionTest;
	};

	/**
	 * @brief Class for batched intersection tests between a test shape and multiple indexed shapes.
	 *
	 * This class allows queuing multiple shapes for intersection testing against a single test shape,
	 * and processes them in batches for improved performance.
	 */
	class BatchedIntersectionTest {
	public:
		static constexpr int NUM_SHAPE_TYPES = static_cast<int>(IntersectionShapeType::LAST);
		static constexpr int NUM_TEST_CASES = static_cast<int>(IntersectionCaseType::LAST);

		/**
		 * @brief Construct a new BatchedIntersectionTest object.
		 */
		BatchedIntersectionTest();

		~BatchedIntersectionTest() = default;

		/**
		 * @brief Set the indexed shapes to be used for intersection tests.
		 * @param shapes A pointer to the vector of indexed shapes.
		 */
		void setIndexedShapes(const std::vector<ref_ptr<BoundingShape>> *shapes);

		/**
		 * @brief Set the hit buffer to store intersecting shape indices.
		 * @param hits A pointer to the hit buffer.
		 */
		void setHitBuffer(HitBuffer *hits);

		/**
		 * @brief Set the batch capacity for queued shapes.
		 * @param capacity The maximum number of shapes to queue before flushing.
		 */
		void setBatchCapacity(uint32_t capacity);

		/**
		 * @brief Begin a new frame for intersection testing with a specified test shape and callback.
		 * Only one frame may be active at a given time, this is not thread-safe.
		 * @param testShape The test shape to use for intersection tests.
		 */
		void beginFrame(const BoundingShape &testShape);

		/**
		 * @brief Insert a shape into the current frame for intersection testing.
		 * If the number of queued shapes exceeds the flush threshold, the queued shapes are processed.
		 * @param shapeIdx The index of the shape to insert from the indexed shapes vector.
		 */
		void push(uint32_t shapeIdx);

		/**
		 * @brief Flush the queued shapes, processing all queued intersection tests.
		 */
		void flush();

		/**
		 * @brief End the current frame, flushing any remaining queued shapes.
		 */
		void endFrame() { flush(); }

		/**
		 * @brief Get the number of currently queued (and unflushed) shapes.
		 * @return The number of queued shapes.
		 */
		uint32_t numQueuedShapes() const { return numQueuedShapes_; }

	protected:

		// A vector over all shapes in the spatial index
		const std::vector<ref_ptr<BoundingShape>> *indexedShapes_ = nullptr;

		// The number of queued shapes where the system automatically flushes
		uint32_t batchOfCapacity_ = 2048u;
		// The number of currently queued shapes
		uint32_t numQueuedShapes_ = 0u;

		// One item for each combination of (fixed) test shape type, and (variable) inde{}xed shape type.
		// E.g., frustum-sphere, frustum-AABB, sphere-sphere, frustum-frustum.
		std::array<uint32_t, NUM_SHAPE_TYPES> frameCases_ = { 0u };
		// Local buffers for each case, size: IntersectionCase::LAST
		std::array<std::unique_ptr<BatchedIntersectionCase>, NUM_TEST_CASES> cases_;
		// Local memory for each shape type, size: NUM_SHAPE_TYPES
		std::array<std::unique_ptr<IntersectionShapeData>, NUM_SHAPE_TYPES> shapeData_;
		std::array<std::unique_ptr<BatchOfShapes>, NUM_SHAPE_TYPES> batchesOfShapes_;

		// Helper function to register a case buffer for a given test and indexed shape type.
		template<IntersectionShapeType TestShapeType, IntersectionShapeType IndexShapeType> void registerCase() {
			using Traits = IntersectionTraits<TestShapeType, IndexShapeType>;
			constexpr auto id = static_cast<int>(Traits::Case);
			cases_[id] = std::make_unique<BatchedIntersectionCase>();
			cases_[id]->doInit = Traits::Init;
			cases_[id]->doFlush = Traits::Flush;
			cases_[id]->indexedShapes = indexedShapes_;
			cases_[id]->shapeData = shapeData_[static_cast<uint32_t>(TestShapeType)].get();
			cases_[id]->batchData = batchesOfShapes_[static_cast<uint32_t>(IndexShapeType)].get();
		}

		// Helper function to register all indexed shape cases for a given test shape type.
		template<IntersectionShapeType Test> void registerAllCases() {
			registerCase<Test, IntersectionShapeType::SPHERE>();
			registerCase<Test, IntersectionShapeType::AABB>();
			registerCase<Test, IntersectionShapeType::OBB>();
			registerCase<Test, IntersectionShapeType::FRUSTUM>();
		}
	};
} // namespace

#endif /* REGEN_BATCHED_INTERSECTION_H_ */
