#ifndef REGEN_RADIX_SORT_GPU_H
#define REGEN_RADIX_SORT_GPU_H

#include <regen/states/state.h>
#include "compute-pass.h"

namespace regen {
	/**
	 * \brief Radix sort state.
	 * This state is used to sort a key buffer using radix sort. Keys must be represented
	 * as unsigned integers (for floats use a bit representation).
	 * The output will be an index buffer (the value array) with the keys sorted in ascending order.
	 */
	class RadixSort_GPU : public State {
	public:
		/**
		 * @brief Constructor for RadixSort
		 * @param numKeys
		 */
		explicit RadixSort_GPU(uint32_t numKeys, uint32_t numLayers = 1);

		~RadixSort_GPU() override = default;

		/**
		 * Set whether visible instances are compacted before sorting.
		 * @param useCompaction True to use compaction, false otherwise.
		 */
		void setUseCompaction(bool useCompaction) { useCompaction_ = useCompaction; }

		/**
		 * Set the output buffer.
		 * This is optional, if not done before createResources(), the output buffer will be created.
		 * The buffer must be a uint array with numKey elements.
		 * @param values The output buffer.
		 */
		void setOutputBuffer(const ref_ptr<SSBO> &values, bool isDoubleBuffered);

		/**
		 * Set the number of bits per radix pass.
		 * 4Bit is a common value, and used by default.
		 * @param bits The number of bits.
		 */
		void setRadixBits(uint32_t bits);

		/**
		 * Change work group size for sorting passes.
		 * 256 is a good value and used by default.
		 * Must be a power of two.
		 * @param size The work group size.
		 */
		void setSortGroupSize(uint32_t size);

		/**
		 * Change work group size for scan passes.
		 * 512 is a good value and used by default.
		 * Must be a power of two.
		 * @param size The work group size.
		 */
		void setScanGroupSize(uint32_t size);

		/**
		 * The key buffer used for sorting, has numKeys elements.
		 * It must be filled with the keys to be sorted externally,
		 * usually it should be done in a compute pass.
		 * @retur The key buffer.
		 */
		auto& keyBuffer() { return keyBuffer_; }

		/**
		 * The sorted index buffer.
		 * This will remain the same across different frames.
		 * @return The sorted index buffer.
		 */
		ref_ptr<SSBO>& valueBuffer();

		/**
		 * Create the resources needed for the radix sort, i.e. buffers
		 * and shaders.
		 */
		void createResources();

		// Override
		void enable(RenderState *rs) override;

	protected:
		const uint32_t numKeys_;
		const uint32_t numLayers_;
		uint32_t outputIdx_ = 0u;
		uint32_t radixBits_ = 4u;
		uint32_t numBuckets_ = 1u << radixBits_;
		uint32_t sortGroupSize_ = 256;
		uint32_t scanGroupSize_ = 512;

		ref_ptr<SSBO> globalHistogramBuffer_;
		ref_ptr<SSBO> keyBuffer_;
		ref_ptr<SSBO> valueBuffer_[2];
		ref_ptr<SSBO> userValueBuffer_;
		bool useSingleValueBuffer_ = false;
		bool useCompaction_ = false;

		ref_ptr<ComputePass> radixHistogramPass_;
		ref_ptr<ComputePass> radixScatterPass_;
		ref_ptr<State> prefixScan_;
		int32_t histogramReadIndex_ = 0u;
		int32_t histogramBitOffsetIndex_ = 0u;
		int32_t scatterReadIndex_ = 0u;
		int32_t scatterWriteIndex_ = 0u;
		int32_t scatterBitOffsetIndex_ = 0u;

		void sort(RenderState *rs);

		void sortContiguous(RenderState *rs);

		void printInstanceMap(RenderState *rs);

		void printHistogram(RenderState *rs);
	};
} // namespace

#endif /* REGEN_RADIX_SORT_GPU_H */
