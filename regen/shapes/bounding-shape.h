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
		BoundingShape(BoundingShapeType shapeType, const ref_ptr<Mesh> &mesh);

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
		GLuint instanceID() const { return instanceID_; }

		/**
		 * @brief Get the number of instances
		 * @return The number of instances
		 */
		GLuint numInstances() const;

		/**
		 * @brief Get the type of this shape
		 * @return The type
		 */
		auto shapeType() const { return shapeType_; }

		/**
		 * @brief Check if this shape is a box
		 * @return True if this shape is a box, false otherwise
		 */
		auto isBox() const { return shapeType_ == BoundingShapeType::BOX; }

		/**
		 * @brief Check if this shape is a sphere
		 * @return True if this shape is a sphere, false otherwise
		 */
		auto isSphere() const { return shapeType_ == BoundingShapeType::SPHERE; }

		/**
		 * @brief Check if this shape is a frustum
		 * @return True if this shape is a frustum, false otherwise
		 */
		auto isFrustum() const { return shapeType_ == BoundingShapeType::FRUSTUM; }

		/**
		 * @brief Set the transform of this shape
		 * @param transform The transform
		 */
		void setTransform(const ref_ptr<ModelTransformation> &transform, unsigned int instanceIndex = 0);

		/**
		 * @brief Set the transform of this shape
		 * @param center The transform
		 */
		void setTransform(const ref_ptr<ShaderInput3f> &center, unsigned int instanceIndex = 0);

		/**
		 * @brief Get the transform of this shape
		 * @return The transform
		 */
		auto &transform() const { return transform_; }

		/**
		 * @brief Get the model offset of this shape
		 * @return The model offset
		 */
		auto &modelOffset() const { return modelOffset_; }

		/**
		 * @brief Get the translation of this shape
		 * @return The translation
		 */
		Vec3f translation() const;

		/**
		 * @brief Get the center position of this shape
		 * This is the geometric center position plus the translation
		 * @return The center position
		 */
		virtual Vec3f getCenterPosition() const = 0;

		/**
		 * @brief Get the stamp of the center
		 * @return The stamp
		 */
		unsigned int transformStamp() const;

		/**
		 * @brief Get the mesh of this shape
		 * @return The mesh
		 */
		auto &mesh() const { return mesh_; }

		/**
		 * @brief Get the parts of this shape
		 * @return The parts
		 */
		auto &parts() const { return parts_; }

		/**
		 * @brief Add a part to this shape
		 * @param part The part
		 */
		void addPart(const ref_ptr<Mesh> &part) { parts_.push_back(part); }

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
		ref_ptr<ModelTransformation> transform_;
		ref_ptr<ShaderInput3f> modelOffset_;
		ref_ptr<Mesh> mesh_;
		std::vector<ref_ptr<Mesh>> parts_;
		unsigned int lastTransformStamp_ = 0;
		unsigned int lastGeometryStamp_;
		unsigned int nextGeometryStamp_ = 0u;
		unsigned int transformIndex_ = 0;
		unsigned int modelOffsetIndex_ = 0;
		std::string name_;
		unsigned int instanceID_ = 0;
	};
} // namespace

#endif /* REGEN_BOUNDING_SHAPE_H_ */
