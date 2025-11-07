#ifndef REGEN_RADIX_SORT_CPU_H
#define REGEN_RADIX_SORT_CPU_H

#include <vector>
#include <cstdint>
#include <cstring>

namespace regen {
	/**
	 * @brief CPU-based Radix Sort implementation
	 * This is a sequential implementation of the Radix Sort algorithm, running
	 * in a single thread on the CPU. However, this is still significantly faster
	 * compared to standard sorting algorithms like std::sort when sorting large.
	 * @tparam IndexType The index type
	 * @tparam KeyType The key type
	 * @tparam BITS_PER_PASS The number of bits per pass
	 */
	template <typename IndexType, typename KeyType, int BITS_PER_PASS>
	struct RadixSort_CPU_seq {
		// e.g. 256 if BITS_PER_PASS=8
		static constexpr int BUCKETS = 1 << BITS_PER_PASS;
		// e.g. 8 passes if KEY_BITS=64 and BITS_PER_PASS=8
		static constexpr int PASSES = (sizeof(KeyType)*8) / BITS_PER_PASS;
		// whether the sorting starts with the temporary buffer
		static constexpr bool START_WITH_TMP = ((PASSES % 2) == 1);

		std::vector<IndexType> tmp_indices_;
		std::vector<uint32_t> histogram_;

		RadixSort_CPU_seq() : histogram_(BUCKETS) {}

		/**
		 * @brief Resize the internal buffers
		 * @param numKeys The number of keys
		 */
		void resize(size_t numKeys) {
			tmp_indices_.reserve(numKeys);
		}

		/**
		 * @brief Sort the indices based on the keys
		 * @param indices The indices to sort
		 * @param keys The keys to sort by
		 */
		void sort(std::vector<IndexType> &indices, const std::vector<KeyType> &keys) {
			const uint32_t numKeys = indices.size();
			const KeyType *src_keys = keys.data();
			tmp_indices_.resize(numKeys);

			IndexType *src, *dst;
			if constexpr (START_WITH_TMP) {
				src = tmp_indices_.data();
				dst = indices.data();
			} else {
				src = indices.data();
				dst = tmp_indices_.data();
			}

			for (int pass = 0; pass < PASSES; ++pass) {
				const int shift = pass * BITS_PER_PASS;
				std::ranges::fill(histogram_, 0);

				// Histogram: Count how many keys fall into each bucket
				for (uint32_t i = 0; i < numKeys; ++i) {
					++histogram_[(src_keys[src[i]] >> shift) & (BUCKETS - 1)];
				}

				// Prefix sum: Compute the starting index for each bucket
				size_t sum = 0;
				for (int i = 0; i < BUCKETS; ++i) {
					const uint32_t tmp = histogram_[i];
					histogram_[i] = sum;
					sum += tmp;
				}

				// Scatter: Place each key in its corresponding bucket
				for (uint32_t i = 0u; i < numKeys; ++i) {
					const auto digit = (src_keys[src[i]] >> shift) & (BUCKETS - 1);
					dst[histogram_[digit]++] = src[i];
				}

				// Swap buffers
				std::swap(src, dst);
			}
		}
	};
} // regen

#endif //REGEN_RADIX_SORT_CPU_H
