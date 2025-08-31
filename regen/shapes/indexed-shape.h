#ifndef REGEN_INDEXED_SHAPE_H_
#define REGEN_INDEXED_SHAPE_H_

#include "regen/glsl/shader-input.h"
#include "regen/shapes/bounding-shape.h"
#include "regen/camera/camera.h"

namespace regen {
	/**
	 * \brief Encapsulates information about a shape in the index and its visibility
	 * for a given camera.
	 */
	class IndexedShape {
	public:
		IndexedShape(
			const ref_ptr <Camera> &camera,
			const ref_ptr <Camera> &sortCamera,
			const ref_ptr <BoundingShape> &shape);

		~IndexedShape() = default;

		/**
		 * \brief Get the number of LODs for this shape
		 * In case the shape has multiple parts, the maximum number of LODs
		 * of all parts is returned.
		 * \return The number of LODs
		 */
		inline uint32_t numLODs() const { return numLODs_; }

		/**
		 * \brief Check if the shape is visible
		 * \return True if the shape is visible, false otherwise
		 */
		inline bool isVisibleInLayer(uint32_t layerIdx) const { return visible_[layerIdx]; }

		/**
		 * \brief Check if the shape is visible in any layer
		 * \return True if the shape is visible in any layer, false otherwise
		 */
		inline bool isVisibleInAnyLayer() const { return isVisibleInAnyLayer_; }

		/**
		 * \brief Map the instance IDs for the shape
		 * \param mapMode The mapping mode
		 * \return The mapped data
		 */
		ClientData_rw<uint32_t> mapInstanceIDs(int mapMode);

		/**
		 * \brief Map the instance counts for the shape
		 * \param mapMode The mapping mode
		 * \return The mapped data
		 */
		ClientData_rw<uint32_t> mapInstanceCounts(int mapMode);

		/**
		 * \brief Map the base instances for the shape
		 * \param mapMode The mapping mode
		 * \return The mapped data
		 */
		ClientData_rw<uint32_t> mapBaseInstances(int mapMode);

		/**
		 * \brief Get the camera
		 * \return The camera
		 */
		auto &camera() const { return camera_; }

		/**
		 * \brief Get the sorting camera
		 * \return The sorting camera
		 */
		auto &sortCamera() const { return sortCamera_; }

		/**
		 * \brief Get the shape
		 * \return The shape
		 */
		auto &shape() const { return shape_; }

		/**
		 * @param mode The sort mode to set
		 */
		void setInstanceSortMode(SortMode mode) { instanceSortMode_ = mode; }

		/**
		 * @return The sort mode used for this indexed shape
		 */
		SortMode instanceSortMode() const { return instanceSortMode_; }

		/**
		 * Get the LOD shift vector, will be added to the computed LOD levels.
		 * @param shift The LOD shift vector to set
		 */
		void setLODShift(const Vec4i &shift) { lodShift_ = shift; }

		/**
		 * Get the LOD shift for this indexed shape
		 * @return The LOD shift vector
		 */
		const Vec4i &lodShift() const { return lodShift_; }

	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<Camera> sortCamera_;
		ref_ptr<BoundingShape> shape_;
		std::vector<ref_ptr<BoundingShape>> boundingShapes_;
		SortMode instanceSortMode_ = SortMode::FRONT_TO_BACK;
		Vec4i lodShift_ = Vec4i::zero();
		uint32_t numLODs_;

		ref_ptr<ShaderInput> idVec_;
		ref_ptr<ShaderInput> countVec_;
		ref_ptr<ShaderInput> baseVec_;
		// TODO: make atomic?
		std::vector<bool> visible_;
		bool isVisibleInAnyLayer_ = false;

		// per-layer visibility flag, instance count, and base instance
		// used during traversal only.
		std::vector<bool> tmp_layerVisibility_;
		// per (lod, layer) data flattened as lod * numLayers + layer
		std::vector<uint32_t> tmp_binCounts_;   // size = numLODs * numLayers
		std::vector<uint32_t> tmp_binBase_;     // size = numLODs * numLayers

		struct ShapeDistance {
			uint32_t instanceID;
			float distance;
		};
		std::vector<std::vector<ShapeDistance>> tmp_layerShapes_; // size = numLayers

		struct MappedData {
			explicit MappedData(
				const ref_ptr <ShaderInput> &idVec,
				const ref_ptr <ShaderInput> &countVec,
				const ref_ptr <ShaderInput> &baseVec);

			~MappedData();

			ClientData_rw<uint32_t> ids;
			ClientData_rw<uint32_t> count;
			ClientData_rw<uint32_t> base;
		};
		std::optional <MappedData> mappedInstanceIDs_;

		void mapInstanceData_internal();

		void unmapInstanceData_internal();

		uint32_t *mappedInstanceIDs();

		uint32_t *mappedInstanceCounts();

		uint32_t *mappedBaseInstance();

		friend class SpatialIndex;
	};
} // namespace

#endif /* REGEN_SPATIAL_INDEX_H_ */
