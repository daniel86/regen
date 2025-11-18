#ifndef REGEN_INDEXED_SHAPE_H_
#define REGEN_INDEXED_SHAPE_H_

#include "regen/shader/shader-input.h"
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
			const Vec4i &lodShift,
			const ref_ptr <BoundingShape> &shape);

		~IndexedShape() = default;

		/**
		 * \brief Get the number of LODs for this shape
		 * In case the shape has multiple parts, the maximum number of LODs
		 * of all parts is returned.
		 * \return The number of LODs
		 */
		uint32_t numLODs() const { return numLODs_; }

		/**
		 * \brief Get the shape index
		 * \return The shape index
		 */
		uint32_t indexedShapeIdx() const { return indexedShapeIdx_; }

		/**
		 * \bried Get the global base instance index for this shape
		 * @return The global base instance index
		 */
		uint32_t shapeBase() const { return shapeBase_; }

		/**
		 * \brief Check if the shape is visible
		 * \return True if the shape is visible, false otherwise
		 */
		bool isVisibleInLayer(uint32_t layerIdx) const { return visible_[layerIdx]; }

		/**
		 * \brief Check if the shape is visible in any layer
		 * \return True if the shape is visible in any layer, false otherwise
		 */
		bool isVisibleInAnyLayer() const { return isVisibleInAnyLayer_; }

		/**
		 * \brief Add a visible instance for the shape
		 * \param layerIdx The layer index
		 * \param binIdx The bin index (lod * numLayers + layer)
		 */
		void addVisibleInstance(uint32_t layerIdx, uint32_t binIdx) {
			// Total visibility count of the shape across all layers and LODs
			numVisibleInstances_ += 1;
			// toggle visibility for this layer
			tmp_layerVisibility_[layerIdx] = true;
			// Finally bin the shape into the (lod, layer) bin
			mapped_binCount_[binIdx] += 1;
		}

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
		BoundingShape &shape() const { return *shape_.get(); }

		/**
		 * @param mode The sort mode to set
		 */
		void setInstanceSortMode(SortMode mode) { instanceSortMode_ = mode; }

		/**
		 * @return The sort mode used for this indexed shape
		 */
		SortMode instanceSortMode() const { return instanceSortMode_; }

		/**
		 * @return The LOD thresholds for this indexed shape
		 */
		const Vec3f &lodThresholds() const { return lodThresholds_; }

	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<Camera> sortCamera_;
		ref_ptr<BoundingShape> shape_;
		SortMode instanceSortMode_ = SortMode::FRONT_TO_BACK;
		Vec4i lodShift_ = Vec4i::zero();
		Vec3f lodThresholds_ = Vec3f(40.0f, 80.0f, 160.0f);
		uint32_t numLODs_;

		// The index of this shape in the camera's indexed shape list.
		// NOTE: The camera's indexed shape list does not contain instances, only unique shapes.
		// Here we limit to max 65536 shapes per camera for sorting key packing,
		// however instances are not counted as individual shapes.
		uint16_t indexedShapeIdx_ = 0;
		// The base index for this shape's instances in the camera's instance list.
		// Larger arrays may be created to hold all instances of all shapes for a camera,
		// this index points to the starting offset for this shape's instances.
		uint32_t shapeBase_ = 0;

		// The array of visible instances which is the main concern in LOD culling.
		// This array holds the instance IDs for all instances of this shape,
		// flattened as layer-major order, i.e. all instances for layer 0,
		// followed by all instances for layer 1, etc.
		// Size = numInstances * numLayers
		ref_ptr<ShaderInput> instanceIDs_;

		// Per bin (lod, layer) data flattened as binIdx = lod * numLayers + layer.
		// Each bin represents a combination of LOD level and layer index, the counter
		// indicates how many instances of this shape are visible in that bin.
		ref_ptr<ShaderInput> binCount_; // size = numLODs * numLayers
		ref_ptr<ShaderInput> binBase_;  // size = numLODs * numLayers
		// Below are mapped data pointers that are only accessible during traversal,
		// i.e. we do not hold the memory here and write directly into output buffers during traversal.
		// binBase is the prefix sum of binCounts, i.e. the starting offset for each bin
		// which is recomputed each frame. Each shape takes a contiguous region in the
		// output instance buffer in the range [binBase, binBase + binCount).
		uint32_t *mapped_binCount_ = nullptr; // size = numLODs * numLayers
		uint32_t *mapped_binBase_ = nullptr;  // size = numLODs * numLayers

		// The total number of visible instances for this shape across all layers and LODs.
		// Recomputed each frame during traversal.
		uint32_t numVisibleInstances_ = 0;
		// True if the shape is visible in any layer.
		// TODO: reconsider, if for thread safety use atomic
		bool isVisibleInAnyLayer_ = false;
		// note: this flag is currently only used by LODState CPU path, and the spatial index traversal
		//       both are currently bound to the same thread, so we do not need atomic updates here.
		// TODO: Reconsider this
		std::vector<bool> visible_;
		// per-layer visibility flag, instance count, and base instance
		// used during traversal only.
		// TODO: REMOVE THIS
		std::vector<bool> tmp_layerVisibility_;

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
