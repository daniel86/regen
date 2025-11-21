#ifndef REGEN_OBB_H_
#define REGEN_OBB_H_

#include <regen/shapes/bounding-box.h>
#include <regen/shapes/aabb.h>
#include <regen/utility/aligned-array.h>
#include "batch-of-shapes.h"

namespace regen {
	/**
	 * @brief Batch structure for oriented bounding box (OBB) shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple OBBs in a batched manner.
	 */
	struct BatchOfOBBs : BatchOfShapes {
		// NOTE: Frustum shapes do not support batched intersection tests yet.
		BatchOfOBBs() : BatchOfShapes(15) {}
		~BatchOfOBBs() override = default;

		struct AxisBatch {
			AlignedArray<float> &x;
			AlignedArray<float> &y;
			AlignedArray<float> &z;
		};

		struct AxisBatch_const {
			const AlignedArray<float> &x;
			const AlignedArray<float> &y;
			const AlignedArray<float> &z;
		};

		AlignedArray<float>& centerX() noexcept { return soaData_[0]; }
		AlignedArray<float>& centerY() noexcept { return soaData_[1]; }
		AlignedArray<float>& centerZ() noexcept { return soaData_[2]; }

		const AlignedArray<float>& centerX() const noexcept { return soaData_[0]; }
		const AlignedArray<float>& centerY() const noexcept { return soaData_[1]; }
		const AlignedArray<float>& centerZ() const noexcept { return soaData_[2]; }

		AlignedArray<float>& halfSizeX() noexcept { return soaData_[3]; }
		AlignedArray<float>& halfSizeY() noexcept { return soaData_[4]; }
		AlignedArray<float>& halfSizeZ() noexcept { return soaData_[5]; }

		const AlignedArray<float>& halfSizeX() const noexcept { return soaData_[3]; }
		const AlignedArray<float>& halfSizeY() const noexcept { return soaData_[4]; }
		const AlignedArray<float>& halfSizeZ() const noexcept { return soaData_[5]; }

		AlignedArray<float>& axis0X() noexcept { return soaData_[6]; }
		AlignedArray<float>& axis0Y() noexcept { return soaData_[7]; }
		AlignedArray<float>& axis0Z() noexcept { return soaData_[8]; }

		AlignedArray<float>& axis1X() noexcept { return soaData_[9]; }
		AlignedArray<float>& axis1Y() noexcept { return soaData_[10]; }
		AlignedArray<float>& axis1Z() noexcept { return soaData_[11]; }

		AlignedArray<float>& axis2X() noexcept { return soaData_[12]; }
		AlignedArray<float>& axis2Y() noexcept { return soaData_[13]; }
		AlignedArray<float>& axis2Z() noexcept { return soaData_[14]; }

		const AlignedArray<float>& axis0X() const noexcept { return soaData_[6]; }
		const AlignedArray<float>& axis0Y() const noexcept { return soaData_[7]; }
		const AlignedArray<float>& axis0Z() const noexcept { return soaData_[8]; }

		const AlignedArray<float>& axis1X() const noexcept { return soaData_[9]; }
		const AlignedArray<float>& axis1Y() const noexcept { return soaData_[10]; }
		const AlignedArray<float>& axis1Z() const noexcept { return soaData_[11]; }

		const AlignedArray<float>& axis2X() const noexcept { return soaData_[12]; }
		const AlignedArray<float>& axis2Y() const noexcept { return soaData_[13]; }
		const AlignedArray<float>& axis2Z() const noexcept { return soaData_[14]; }

		std::array<AxisBatch_const, 3> axes() const {
			return {
				AxisBatch_const{soaData_[6],  soaData_[7],  soaData_[8]},
				AxisBatch_const{soaData_[9],  soaData_[10], soaData_[11]},
				AxisBatch_const{soaData_[12], soaData_[13], soaData_[14]}
			};
		}
	};

	/**
	 * @brief Oriented bounding box
	 */
	class OBB : public BoundingBox<BatchOfOBBs> {
	public:
		/**
		 * @brief Construct a new OBB object
		 * @param mesh The mesh
		 */
		OBB(const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts);

		/**
		 * @brief Construct a new OBB object
		 * @param bounds The min/max of the OBB
		 */
		explicit OBB(const Bounds<Vec3f> &bounds);

		~OBB() override = default;

		/**
		 * @brief Get the vertices of this box
		 * @return The vertices
		 */
		auto *boxVertices() const { return vertices_; }

		/**
		 * @brief Check if this OBB has intersection with an AABB
		 * @param other The AABB
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithAABB(const AABB &other) const;

		/**
		 * @brief Check if this OBB has intersection with another OBB
		 * @param other The other OBB
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWithOBB(const OBB &other) const;

		// override BoundingBox::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// BoundingShape interface
		void updateBaseBounds(const Vec3f &min, const Vec3f &max) override;

		// BoundingShape interface
		bool updateTransform(bool forceUpdate) final;

		/**
		 * @brief Batch intersection test of this shape against a batch of spheres.
		 */
		static void batchTest_Spheres(BatchedIntersectionCase&);

		/**
		 * @brief Batch intersection test of this shape against a batch of AABBs.
		 */
		static void batchTest_AABBs(BatchedIntersectionCase&);

		/**
		 * @brief Batch intersection test of this shape against a batch of OBBs.
		 */
		static void batchTest_OBBs(BatchedIntersectionCase&);

		/**
		 * @brief Batch intersection test of this shape against a batch of frustums.
		 */
		static void batchTest_Frustums(BatchedIntersectionCase&);

	protected:
		// transformed vertices
		Vec3f vertices_[8];

		void updateOBB();

		void updateBaseSize();

		void applyTransform(const Mat4f &tf);
	};

	/**
	 * @brief Shape traits for OBB shapes.
	 */
	template<> struct ShapeTraits<BoundingShapeType::OBB> {
		using BatchType = BatchOfOBBs;
		static constexpr auto NumSoAArrays = 15; // center, halfSize, axis0, axis1, axis2
	};

	/**
	 * @brief Intersection traits specialization for OBB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::OBB, BoundingShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::OBB_SPHERES;
		static constexpr auto Test = OBB::batchTest_Spheres;
	};

	/**
	 * @brief Intersection traits specialization for OBB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::OBB, BoundingShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::OBB_AABBs;
		static constexpr auto Test = OBB::batchTest_AABBs;
	};

	/**
	 * @brief Intersection traits specialization for OBB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::OBB, BoundingShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::OBB_OBBs;
		static constexpr auto Test = OBB::batchTest_OBBs;
	};

	/**
	 * @brief Intersection traits specialization for OBB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::OBB, BoundingShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::OBB_FRUSTUMS;
		static constexpr auto Test = OBB::batchTest_Frustums;
	};

	/**
	 * @brief Batch intersection test of OBB shapes vs. sphere shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::OBB,
			BoundingShapeType::SPHERE>(BatchedIntersectionCase &td) {
		OBB::batchTest_Spheres(td);
	}

	/**
	 * @brief Batch intersection test of OBB shapes vs. AABB shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::OBB,
			BoundingShapeType::AABB>(BatchedIntersectionCase &td) {
		OBB::batchTest_AABBs(td);
	}

	/**
	 * @brief Batch intersection test of OBB shapes vs. OBB shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::OBB,
			BoundingShapeType::OBB>(BatchedIntersectionCase &td) {
		OBB::batchTest_OBBs(td);
	}

	/**
	 * @brief Batch intersection test of OBB shapes vs. frustum shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::OBB,
			BoundingShapeType::FRUSTUM>(BatchedIntersectionCase &td) {
		OBB::batchTest_Frustums(td);
	}

	/**
	 * @brief Intersection shape data for oriented bounding box (OBB) shapes.
	 */
	struct IntersectionData_OBB : IntersectionShapeData {
		void update(const BoundingShape&) {}
	};
} // namespace regen

#endif /* REGEN_OBB_H_ */
