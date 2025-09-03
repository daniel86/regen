#ifndef REGEN_PREFIX_SCAN_H
#define REGEN_PREFIX_SCAN_H

#include <regen/states/state.h>
#include "regen/buffer/ssbo.h"
#include "compute-pass.h"

namespace regen {
	/**
	 * \brief Prefix scan state.
	 * This state is used to compute a prefix sum (scan) over a buffer.
	 * The input buffer must be a uint array with a fixed number of elements.
	 * The output will be written into the input buffer, and it will represent the prefix sum of the input buffer.
	 *
	 * The state can handle situations where the size of the histogram changes dynamically, but at some cost.
	 * For this reason, it is best to flag the histogram as having a constant size if possible.
	 */
	class PrefixScan : public State {
	public:
		enum Mode {
			SERIAL,
			PARALLEL,
			HIERARCHICAL
		};

		/**
		 * @brief Constructor for PrefixScan
		 * @param histogram The histogram buffer
		 */
		explicit PrefixScan(const ref_ptr<SSBO> &histogram, uint32_t numLayers = 1);

		~PrefixScan() override = default;

		/**
		 * Set whether the histogram has a constant size which
		 * allows some optimizations.
		 */
		void setHasHistogramConstantSize(bool hasConstantSize) {
			hasHistogramConstantSize_ = hasConstantSize;
		}

		/**
		 * Change work group size for scan passes.
		 * 512 is a good value and used by default.
		 * Must be a power of two.
		 * @param size The work group size.
		 */
		void setScanGroupSize(uint32_t size) { scanGroupSize_ = size; }

		/**
		 * Create the resources needed for the scan, i.e. buffers
		 * and shaders.
		 */
		void createResources();

		// Override
		void enable(RenderState *rs) override;

		void printHistogram(RenderState *rs);

	protected:
		const uint32_t numLayers_;
		uint32_t scanGroupSize_ = 512;
		Mode scanMode_ = Mode::PARALLEL;

		ref_ptr<SSBO> globalHistogramBuffer_;
		ref_ptr<SSBO> blockOffsetsBuffer_;

		ref_ptr<ComputePass> computeGlobalOffsets_h_;
		ref_ptr<ComputePass> computeGlobalOffsets_p_;
		ref_ptr<ComputePass> computeGlobalOffsets_s_;
		ref_ptr<ComputePass> computeLocalOffsets_;
		ref_ptr<ComputePass> distributeOffsets_;

		StateConfigurer computeGlobalOffsets_p_cfg_;
		StateConfigurer computeGlobalOffsets_h_cfg_;

		bool hasHistogramConstantSize_ = false;
		bool hasInitializedUniforms_ = false;
		uint32_t currentHistogramSize_ = 0u;
		uint32_t currentNumBlocks_ = 0u;
		int32_t histogramSizeLoc_s_ = 0u;
		int32_t histogramSizeLoc_p_ = 0u;
		int32_t histogramSizeLoc_h_l_ = 0u;
		int32_t histogramSizeLoc_h_d_ = 0u;
		int32_t numBlocksLoc_h_g_ = 0u;
		int32_t numBlocksLoc_h_l_ = 0u;
		int32_t numBlocksLoc_h_d_ = 0u;

		uint32_t getNumDesiredInvocations() const;

		PrefixScan::Mode getScanMode(uint32_t numDesiredInvocations);

		void createSerialPass();
		void updateSerialPass();

		void createParallelPass(uint32_t parallelScanInvocations);
		void updateParallelPass(uint32_t parallelScanInvocations);

		void createHierarchicalPass();
		void updateHierarchicalPass();

		void updateHistogramSize(uint32_t newHistogramSize);

		void scan(RenderState *rs);
		void scan1(RenderState *rs);
		void scan2(RenderState *rs);
	};
} // namespace

#endif /* REGEN_PREFIX_SCAN_H */
