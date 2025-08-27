#ifndef REGEN_BOUNDING_SHAPE_H_
#define REGEN_BOUNDING_SHAPE_H_

#include "regen/states/model-transformation.h"
#include "regen/meshes/mesh-state.h"
#include "bounds.h"

namespace regen {
	enum class BoundingShapeType {
		BOX,
		SPHERE,
		FRUSTUM
	};

	/**
	 * @brief Bounding shape
	 */
	class BoundingShape {
	public:
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

		virtual ~BoundingShape() = default;

		/**
		 * @brief Set the name of this shape
		 * @param name The name
		 */
		void setName(const std::string &name) { name_ = name; }

		/**
		 * @brief Get the name of this shape
		 * @return The name
		 */
		const std::string &name() const { return name_; }

		/**
		 * @brief Set the instance ID of this shape
		 * @param instanceID The instance ID
		 */
		void setInstanceID(GLuint instanceID) { instanceID_ = instanceID; }

		/**
		 * @brief Get the instance ID of this shape
		 * @return The instance ID
		 */
		uint32_t instanceID() const { return instanceID_; }

		/**
		 * @brief Get the number of instances
		 * @return The number of instances
		 */
		uint32_t numInstances() const;

		/**
		 * @brief Get the type of this shape
		 * @return The type
		 */
		auto shapeType() const { return shapeType_; }

		/**
		 * @brief Check if this shape is a box
		 * @return True if this shape is a box, false otherwise
		 */
		bool isBox() const { return shapeType_ == BoundingShapeType::BOX; }

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
		 * This is the geometric center position plus the translation
		 * @return The center position
		 */
		const Vec3f& getShapeOrigin() const { return shapeOrigin_; }

		/**
		 * @brief Get the stamp of the center
		 * @return The stamp
		 */
		uint32_t transformStamp() const;

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
		void addPart(const ref_ptr<Mesh> &part) {
			if (part.get() != nullptr) {
				parts_.push_back(part);
			}
			if (!baseMesh_.get()) {
				baseMesh_ = part;
			}
		}

		/**
		 * @brief Update the transform
		 */
		virtual bool updateTransform(bool forceUpdate) = 0;

		/**
		 * @brief Update the shape
		 */
		bool updateGeometry();

		/**
		 * @brief Update the geometry of this shape
		 */
		virtual void updateBounds(const Vec3f &min, const Vec3f &max) = 0;

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

	protected:
		const BoundingShapeType shapeType_;
		ref_ptr<Mesh> mesh_;
		ref_ptr<Mesh> baseMesh_;
		std::vector<ref_ptr<Mesh>> parts_;

		ref_ptr<ModelTransformation> transform_;
		// only used in case no TF is set
		Mat4f localTransform_ = Mat4f::identity();
		Vec3f shapeOrigin_ = Vec3f::zero();

		uint32_t localStamp_ = 1u;
		uint32_t lastTransformStamp_ = 0;
		uint32_t lastGeometryStamp_;
		uint32_t nextGeometryStamp_ = 0u;
		uint32_t transformIndex_ = 0;
		std::string name_;
		uint32_t instanceID_ = 0;
		// custom data pointer used for spatial index intersection tests
		void *spatialIndexData_ = nullptr;
		friend class SpatialIndex;

	};
} // namespace

#endif /* REGEN_BOUNDING_SHAPE_H_ */
