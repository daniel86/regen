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
			const ref_ptr <Camera> &sortCamera,
			const ref_ptr <Camera> &lodCamera,
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
		 * \brief Check if the shape is visible in any layer
		 * \return True if the shape is visible in any layer, false otherwise
		 */
		bool isVisibleInAnyLayer() const { return isVisibleInAnyLayer_.load(std::memory_order_relaxed); }

		/**
		 * \brief Get the instance IDs shader input
		 * \return The instance IDs shader input
		 */
		const ref_ptr<ShaderInput>& instanceIDs() const { return instanceIDs_; }

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
		 * \brief Get the sort camera
		 * \return The sort camera
		 */
		auto &sortCamera() const { return sortCamera_; }

		/**
		 * \brief Get the LOD camera
		 * \return The LOD camera
		 */
		auto &lodCamera() const { return lodCamera_; }

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
		ref_ptr<Camera> sortCamera_;
		ref_ptr<Camera> lodCamera_;
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

		// Draw command data used to build indirect draw buffers.
		// The number of draw command for a shape is numLODs * numLayers:
		// Each bin represents a combination of LOD level and layer index, the counter
		// indicates how many instances of this shape are visible in that bin.
		ref_ptr<ShaderInput> drawBinCount_; // size = numLODs * numLayers
		ref_ptr<ShaderInput> drawBinBase_;  // size = numLODs * numLayers
		// Below are mapped data pointers that are only accessible during traversal,
		// i.e. we do not hold the memory here and write directly into output buffers during traversal.
		// binBase is the prefix sum of binCounts, i.e. the starting offset for each bin
		// which is recomputed each frame. Each shape takes a contiguous region in the
		// output instance buffer in the range [binBase, binBase + binCount).
		uint32_t *mapped_drawBinCount_ = nullptr; // size = numLODs * numLayers
		uint32_t *mapped_drawBinBase_ = nullptr;  // size = numLODs * numLayers

		// True if the shape is visible in any layer.
		std::atomic<uint8_t> isVisibleInAnyLayer_ = {0};

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
