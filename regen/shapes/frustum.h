#ifndef REGEN_FRUSTUM_H_
#define REGEN_FRUSTUM_H_

#include <vector>
#include <regen/compute/vector.h>
#include <regen/compute/plane.h>
#include <regen/shapes/bounding-shape.h>
#include <regen/shapes/bounding-box.h>
#include <regen/shapes/bounding-sphere.h>
#include <regen/shapes/aabb.h>
#include <regen/shapes/obb.h>

namespace regen {
	/**
	 * @brief Batch structure for frustum shapes.
	 * This structure holds the necessary data for performing
	 * intersection tests with multiple frustums in a batched manner.
	 */
	struct BatchOfFrustums : BatchOfShapes {
		// NOTE: Frustum shapes do not support batched intersection tests yet.
		BatchOfFrustums() : BatchOfShapes(0) {}
		~BatchOfFrustums() override = default;
	};

	/**
	 * A portion of a solid pyramid that lies between two parallel planes cutting it.
	 */
	class Frustum : public BoundingShape {
	public:
		/** The field of view angle. */
		double fov = 60.0;
		/** The aspect ratio. */
		double aspect = 8.0 / 6.0;
		/** The near plane distance. */
		double near = 0.1;
		/** The far plane distance. */
		double far = 100.0;
		/** Near plane size. */
		Vec2f nearPlaneHalfSize = Vec2f::create(0.1f);
		/** Far plane size. */
		Vec2f farPlaneHalfSize = Vec2f::create(1.0f);
		/** Bounds of parallel projection */
		Bounds<Vec2f> orthoBounds;
		/** The 8 frustum points. 0-3 are the near plane points, 4-7 far plane. */
		Vec3f points[8];
		/** The 6 frustum planes. */
		Plane planes[6];

		Frustum();

		/**
		 * Set projection parameters and compute near- and far-plane.
		 */
		void setPerspective(double aspect, double fov, double near, double far);

		/**
		 * Set projection parameters and compute near- and far-plane.
		 */
		void setOrtho(double left, double right, double bottom, double top, double near, double far);

		/**
		 * Update frustum points based on view point and direction.
		 */
		void update(const Vec3f &pos, const Vec3f &dir);

		/**
		 * Split this frustum along the view ray.
		 */
		void split(double splitWeight, std::vector<Frustum> &frustumSplit) const;

		/**
		 * @return true if the sphere intersects with this frustum.
		 */
		bool hasIntersectionWithSphere(const BoundingSphere &sphere) const;

		/**
		 * @return true if the box intersects with this frustum.
		 */
		bool hasIntersectionWithAABB(const AABB &box) const;

		/**
		 * @return true if the box intersects with this frustum.
		 */
		bool hasIntersectionWithOBB(const OBB &box) const;

		/**
		 * @return true if the frustum intersects with this frustum.
		 */
		bool hasIntersectionWithFrustum(const Frustum &other) const;

		// override BoundingShape::closestPointOnSurface
		Vec3f closestPointOnSurface(const Vec3f &point) const final;

		// override BoundingShape::update
		bool updateTransform(bool forceUpdate) final;

		// BoundingShape interface
		void updateBaseBounds(const Vec3f &min, const Vec3f &max) override { }

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
		ref_ptr<ShaderInput3f> direction_;
		unsigned int lastDirectionStamp_ = 0;

		unsigned int directionStamp() const;

		void updatePointsPerspective(const Vec3f &pos, const Vec3f &dir);
		void updatePointsOrthogonal(const Vec3f &pos, const Vec3f &dir);
	};

	/**
	 * @brief Shape traits for frustum shapes.
	 */
	template<> struct ShapeTraits<BoundingShapeType::FRUSTUM> {
		using BatchType = BatchOfFrustums;
		static constexpr auto NumSoAArrays = 0; // Not supported yet
	};

	/**
	 * @brief Intersection traits for frustum shapes vs. sphere shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::FRUSTUM, BoundingShapeType::SPHERE> {
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_SPHERES;
		static constexpr auto Test = Frustum::batchTest_Spheres;
	};

	/**
	 * @brief Intersection traits for frustum shapes vs. AABB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::FRUSTUM, BoundingShapeType::AABB> {
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_AABBs;
		static constexpr auto Test = Frustum::batchTest_AABBs;
	};

	/**
	 * @brief Intersection traits for frustum shapes vs. OBB shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::FRUSTUM, BoundingShapeType::OBB> {
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_OBBs;
		static constexpr auto Test = Frustum::batchTest_OBBs;
	};

	/**
	 * @brief Intersection traits for frustum shapes vs. frustum shapes.
	 */
	template<> struct IntersectionTraits<BoundingShapeType::FRUSTUM, BoundingShapeType::FRUSTUM> {
		static constexpr auto Case = IntersectionCaseType::FRUSTUM_FRUSTUMS;
		static constexpr auto Test = Frustum::batchTest_Frustums;
	};

	/**
	 * @brief Batch intersection test of SPHERE shapes vs. sphere shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::FRUSTUM,
			BoundingShapeType::SPHERE>(BatchedIntersectionCase &td) {
		Frustum::batchTest_Spheres(td);
	}

	/**
	 * @brief Batch intersection test of SPHERE shapes vs. AABB shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::FRUSTUM,
			BoundingShapeType::AABB>(BatchedIntersectionCase &td) {
		Frustum::batchTest_AABBs(td);
	}

	/**
	 * @brief Batch intersection test of SPHERE shapes vs. OBB shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::FRUSTUM,
			BoundingShapeType::OBB>(BatchedIntersectionCase &td) {
		Frustum::batchTest_OBBs(td);
	}

	/**
	 * @brief Batch intersection test of SPHERE shapes vs. frustum shapes.
	 */
	template<> inline void batchIntersectionTest<
			BoundingShapeType::FRUSTUM,
			BoundingShapeType::FRUSTUM>(BatchedIntersectionCase &td) {
		Frustum::batchTest_Frustums(td);
	}

	/**
	 * @brief Intersection shape data for frustum shapes.
	 */
	struct IntersectionData_Frustum : IntersectionShapeData {
		std::array<Vec4f,6> planes;
		void update(const BoundingShape &shape) {
			const auto &frustum = static_cast<const Frustum &>(shape);
			for (size_t i = 0; i < 6; ++i) {
				planes[i] = frustum.planes[i].coefficients;
			}
		}
	};
} // namespace

#endif /* REGEN_FRUSTUM_H_ */
