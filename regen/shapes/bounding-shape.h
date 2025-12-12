#ifndef REGEN_BOUNDING_SHAPE_H_
#define REGEN_BOUNDING_SHAPE_H_

#include "regen/objects/model-transformation.h"
#include "orthogonal-projection.h"

namespace regen {
	/**
	 * @brief Types of bounding shapes
	 */
	enum class BoundingShapeType {
		SPHERE = 0,
		AABB,
		OBB,
		FRUSTUM,
		LAST // keep last
	};

	class Mesh;
	struct BatchOfShapes;
	class IndexedShape;

	/**
	 * @brief Bounding shape
	 */
	class BoundingShape {
	public:
		// If the SKIP bit is set, the shape is skipped during traversal.
		static constexpr uint32_t TRAVERSAL_BIT_SKIP = 0;
		// If the DRAW bit is set, the shape is considered during draw traversal.
		static constexpr uint32_t TRAVERSAL_BIT_DRAW = 1;
		// If the COLLISION bit is set, the shape is considered during collision traversal.
		static constexpr uint32_t TRAVERSAL_BIT_COLLISION = 2;

		/**
		 * @brief Construct a new Bounding Shape object
		 * @param shapeType The type of the shape
		 */
		explicit BoundingShape(BoundingShapeType shapeType);

		/**
		 * @brief Construct a new Bounding Shape object
		 * @param shapeType The type of the shape
		 * @param mesh The mesh
		 */
		BoundingShape(BoundingShapeType shapeType, const ref_ptr<Mesh> &mesh, const std::vector<ref_ptr<Mesh>> &parts);

		virtual ~BoundingShape();

		/**
		 * @brief Set the name of this shape
		 * @param name The name
		 */
		void setName(std::string_view name) { name_ = name; }

		/**
		 * @brief Get the name of this shape
		 * @return The name
		 */
		const std::string &name() const { return name_; }

		/**
		 * @brief Set the instance ID of this shape
		 * @param instanceID The instance ID
		 */
		void setInstanceID(uint32_t instanceID) { instanceID_ = instanceID; }

		/**
		 * @brief Get the instance ID of this shape
		 * @return The instance ID
		 */
		uint32_t instanceID() const { return instanceID_; }

		/**
		 * @brief Toggle use of local stamp.
		 * If true, the local stamp is used, otherwise the stamp of the ModelTransformation.
		 * @param useLocalStamp True to use local stamp, false to use ModelTransformation stamp
		 */
		void setUseLocalStamp(bool useLocalStamp);

		/**
		 * @return true if local stamp is used, false if ModelTransformation stamp is used
		 */
		bool useLocalStamp() const { return useLocalStamp_; }

		/**
		 * Advance the local stamp by one.
		 * This is used when no ModelTransformation is set and the local transform is used
		 */
		void nextLocalStamp() { localStamp_.fetch_add(1u, std::memory_order_relaxed); }

		/**
		 * Set the bitmask used to skip the shape during traversal.
		 * @param mask The traversal mask to set
		 */
		void setTraversalMask(uint32_t mask) { traversalMask_ = mask; }

		/**
		 * @brief Enable or disable a traversal bit
		 * @param bit The bit to set
		 * @param enabled True to enable, false to disable
		 */
		void setTraversalBit(uint32_t bit, bool enabled) {
			if (enabled) {
				traversalMask_ |= (1 << bit);
			} else {
				traversalMask_ &= ~(1 << bit);
			}
		}

		/**
		 * @brief Get the traversal mask
		 * @return The traversal mask
		 */
		uint32_t traversalMask() const { return traversalMask_; }

		/**
		 * @brief Get the number of instances
		 * @return The number of instances
		 */
		uint32_t numInstances() const { return numInstances_; }

		/**
		 * @brief Get the type of this shape
		 * @return The type
		 */
		BoundingShapeType shapeType() const { return shapeType_; }

		/**
		 * @brief Check if this box is an AABB
		 * @return True if this box is an AABB, false otherwise
		 */
		bool isAABB() const { return shapeType_ == BoundingShapeType::AABB; }

		/**
		 * @brief Check if this box is an OBB
		 * @return True if this box is an OBB, false otherwise
		 */
		bool isOBB() const { return shapeType_ == BoundingShapeType::OBB; }

		/**
		 * @brief Check if this shape is a box
		 * @return True if this shape is a box, false otherwise
		 */
		bool isBox() const { return shapeType_ == BoundingShapeType::AABB || shapeType_ == BoundingShapeType::OBB; }

		/**
		 * @brief Check if this shape is a sphere
		 * @return True if this shape is a sphere, false otherwise
		 */
		bool isSphere() const { return shapeType_ == BoundingShapeType::SPHERE; }

		/**
		 * @brief Check if this shape is a frustum
		 * @return True if this shape is a frustum, false otherwise
		 */
		bool isFrustum() const { return shapeType_ == BoundingShapeType::FRUSTUM; }

		/**
		 * @brief Set the transform of this shape
		 * @param transform The transform
		 */
		void setTransform(const ref_ptr<ModelTransformation> &transform, unsigned int instanceIndex = 0);

		/**
		 * @brief Set the local transform of this shape
		 * This is used when no ModelTransformation is set
		 * @param localTransform The local transform
		 */
		void setTransform(const Mat4f &localTransform);

		/**
		 * @brief Get the transform of this shape, if any
		 * @return The transform
		 */
		const ref_ptr<ModelTransformation> &transform() const { return transform_; }

		/**
		 * @brief Get the translation of this shape
		 * @return The translation
		 */
		const Vec3f& translation() const;

		/**
		 * @brief Get the center position of this shape
		 * This is the geometric center position with applied TF
		 * @return The center position
		 */
		const Vec3f& tfOrigin() const { return tfOrigin_; }

		/**
		 * @brief Get the stamp of the center
		 * @return The stamp
		 */
		uint32_t tfStamp() const;

		/**
		 * @brief Get the mesh of this shape
		 * @return The mesh
		 */
		const ref_ptr<Mesh> &mesh() const { return mesh_; }

		/**
		 * @brief Get the head mesh of this shape
		 * This is the main mesh if set, otherwise the first part
		 * @return The head mesh
		 */
		const ref_ptr<Mesh> &baseMesh() const { return baseMesh_; }

		/**
		 * @brief Get the parts of this shape
		 * @return The parts
		 */
		const std::vector<ref_ptr<Mesh>> &parts() const { return parts_; }

		/**
		 * Add a part to this shape
		 * @param part The part to add
		 */
		void addPart(const ref_ptr<Mesh> &part);

		/**
		 * @brief Update the transform
		 */
		virtual bool updateTransform(bool forceUpdate) = 0;

		/**
		 * @brief Update the shape
		 */
		bool updateGeometry();

		/**
		 * @brief Update the geometry of this shape (without TF)
		 */
		virtual void updateBaseBounds(const Vec3f &min, const Vec3f &max) = 0;

		/**
		 * Set the base offset of this shape which is added to the base bounds.
		 * @param offset The offset
		 */
		void setBaseOffset(const Vec3f &offset);

		/**
		 * @brief Check if this shape has intersection with another shape
		 * @param other The other shape
		 * @return True if there is an intersection, false otherwise
		 */
		bool hasIntersectionWith(const BoundingShape &other) const;

		/**
		 * @brief Get the closest point on the surface of this shape
		 * @param point The point
		 * @return The closest point
		 */
		virtual Vec3f closestPointOnSurface(const Vec3f &point) const = 0;

		/**
		 * @brief Get the orthogonal projection of this shape.
		 * If needed the projection is recomputed, ie. when the geometry or transform has changed.
		 * @return The orthogonal projection
		 */
		void updateOrthogonalProjection();

		/**
		 * @brief Get the orthogonal projection of this shape.
		 * If needed the projection is recomputed, ie. when the geometry or transform has changed.
		 * @return The orthogonal projection
		 */
		const OrthogonalProjection &orthoProjection() const { return *orthoProjection_.get(); }

		/**
		 * @brief Get the orthogonal projection of this shape.
		 * If needed the projection is recomputed, ie. when the geometry or transform has changed.
		 * @return The orthogonal projection
		 */
		OrthogonalProjection &orthoProjection() { return *orthoProjection_.get(); }


		/**
		 * Assign a world object to this shape.
		 * This is used to link the shape to a world object.
		 * @param obj The world object
		 */
		void setWorldObject(Resource *obj) { worldObject_ = obj; }

		/**
		 * Unset the world object from this shape if it matches the given object.
		 * This is used to unlink the shape from a world object.
		 * @param obj The world object
		 */
		void unsetWorldObject(Resource *obj) {
			if (worldObject_ == obj) {
				worldObject_ = nullptr;
			}
		}

		/**
		 * Unset the world object from this shape.
		 * This is used to unlink the shape from a world object.
		 */
		Resource *worldObject() const { return worldObject_; }

		/**
		 * @return true if a world object is assigned, false otherwise
		 */
		bool hasWorldObject() const { return worldObject_ != nullptr; }

		/**
		 * @brief Get the global index of this shape instance in the batch data buffer
		 * @return The global index
		 */
		uint32_t globalIndex() const { return globalIndex_; }

		/**
		 * @brief Get the global batch data buffer
		 * @return The global batch data buffer
		 */
		BatchOfShapes &globalBatchData() { return *globalShapeData_untyped_; }

		/**
		 * @brief Get the global batch data buffer
		 * @return The global batch data buffer
		 */
		const BatchOfShapes &globalBatchData() const { return *globalShapeData_untyped_; }

	protected:
		const BoundingShapeType shapeType_;
		ref_ptr<Mesh> mesh_;
		ref_ptr<Mesh> baseMesh_;
		std::vector<ref_ptr<Mesh>> parts_;
		Resource *worldObject_ = nullptr;

		// Projection of the shape onto the xz-plane.
		// This is computed in a lazy manner when requested.
		ref_ptr<OrthogonalProjection> orthoProjection_;
		ref_ptr<ModelTransformation> transform_;
		// only used in case no TF is set
		Mat4f localTransform_ = Mat4f::identity();
		Vec3f tfOrigin_ = Vec3f::zero();
		Vec3f baseOffset_ = Vec3f::zero();

		bool useLocalStamp_ = false;
		std::atomic<uint32_t> localStamp_;
		uint32_t lastTransformStamp_ = 0;
		uint32_t lastGeometryStamp_;
		uint32_t nextGeometryStamp_ = 0u;
		uint32_t transformIndex_ = 0;
		// a member function pointer that returns the current TF stamp
		// this is either from the ModelTransformation or the local stamp
		uint32_t (BoundingShape::*stampFun_)() const = &BoundingShape::getLocalStamp;
		std::string name_;
		uint32_t numInstances_ = 1;
		uint32_t instanceID_ = 0;
		uint32_t traversalMask_ = (1 << TRAVERSAL_BIT_DRAW); // default: draw

		uint32_t globalIndex_ = 0;
		BatchOfShapes *globalShapeData_untyped_ = nullptr;

		uint32_t getTransformStamp() const {
			return transform_->stamp();
		}

		uint32_t getLocalStamp() const {
			return localStamp_.load(std::memory_order_acquire);
		}

		void updateStampFunction();

		friend class SpatialIndex;

	};

	/**
	 * @brief Batched bounding shape
	 * @tparam BatchType The type of the batch data
	 * Batched shape data is drawn from a shared global buffer, and each shape instance
	 * has an index into that buffer. This allows efficient storage and retrieval of
	 * shape data for operations like collision detection and spatial queries.
	 * This is not thread-safe, so make sure to create/destroy shapes in a single thread.
	 */
	template <typename BatchType>
	class BatchedBoundingShape : public BoundingShape {
	public:
		/**
		 * @brief Construct a new Batched Bounding Shape object
		 * @param shapeType The type of the shape
		 */
		explicit BatchedBoundingShape(BoundingShapeType shapeType)
			: BoundingShape(shapeType) {
			this->globalIndex_ = reserveGlobalIndex();
			this->globalShapeData_untyped_ = &globalBatchData_;
		}

		/**
		 * @brief Construct a new Batched Bounding Shape object
		 * @param shapeType The type of the shape
		 * @param mesh The mesh
		 * @param parts The parts of the mesh
		 */
		BatchedBoundingShape(BoundingShapeType shapeType,
					const ref_ptr<Mesh> &mesh,
					const std::vector<ref_ptr<Mesh>> &parts)
			: BoundingShape(shapeType, mesh, parts) {
			this->globalIndex_ = reserveGlobalIndex();
			this->globalShapeData_untyped_ = &globalBatchData_;
		}

		/**
		 * @brief Destroy the Batched Bounding Shape object
		 */
		~BatchedBoundingShape() override {
			const uint32_t numCopies = copyCounter_.refCount();
			if (numCopies == 1) {
				releaseGlobalIndex(globalIndex_);
			}
		}

		/**
		 * @brief Get the global batch data buffer
		 * @return The global batch data buffer
		 */
		static const BatchType &globalBatchData_t() { return globalBatchData_; }

	protected:
		// A global contiguous buffer holding batched shape data for all instances in SOA layout.
		inline static BatchType globalBatchData_ = BatchType();
		// A pool of free indices into the global batch data buffer, only indices < nextGlobalIndex_ are added,
		// initially this pool is empty.
		inline static std::vector<uint32_t> freeGlobalIndices_ = std::vector<uint32_t>();
		// The number of free global indices available in the pool (size() could be larger)
		inline static uint32_t numFreeGlobalIndices_ = 0u;
		// The index into the global batch data buffer for the next new shape instance.
		inline static uint32_t nextGlobalIndex_ = 0u;

	private:
		// we use this only for counting how many copies are made, such that we can
		// safely release the global index on destruction of the last copy.
		ref_ptr<bool> copyCounter_ = ref_ptr<bool>::alloc(true);

		static uint32_t reserveGlobalIndex() {
			if (numFreeGlobalIndices_ > 0) {
				// reuse an index from the free pool
				return freeGlobalIndices_[--numFreeGlobalIndices_];
			} else {
				// assign a new index
				uint32_t globalIndex = nextGlobalIndex_++;
				// ensure global batch data has enough capacity
				if (globalBatchData_.capacity < nextGlobalIndex_) {
					static constexpr bool preserveData = true;
					globalBatchData_.resize(nextGlobalIndex_ * 2, preserveData);
				}
				return globalIndex;
			}
		}

		static void releaseGlobalIndex(uint32_t globalIndex) {
			// add the index to the free pool
			if (numFreeGlobalIndices_ < freeGlobalIndices_.size()) {
				freeGlobalIndices_[numFreeGlobalIndices_++] = globalIndex;
			} else {
				freeGlobalIndices_.push_back(globalIndex);
				numFreeGlobalIndices_++;
			}
		}
	};

	/**
	 * @brief Shape traits for bounding shape types
	 * @tparam ShapeType The type of the bounding shape
	 */
	template<BoundingShapeType ShapeType> struct ShapeTraits;
} // namespace

#include <regen/shapes/batch-of-shapes.h>

#endif /* REGEN_BOUNDING_SHAPE_H_ */
