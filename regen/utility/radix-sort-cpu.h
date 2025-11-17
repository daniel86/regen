#ifndef REGEN_RADIX_SORT_CPU_H
#define REGEN_RADIX_SORT_CPU_H

#include <vector>
#include <cstdint>
#include "regen/math/simd.h"

#define REGEN_RADIX_SORT_CPU_USE_SIMD

namespace regen {
	/**
	 * @brief CPU-based Radix Sort implementation
	 * This is a sequential implementation of the Radix Sort algorithm, running
	 * in a single thread on the CPU. However, this is still significantly faster
	 * compared to standard sorting algorithms like std::sort when sorting large.
	 * @tparam IndexType The index type
	 * @tparam KeyType The key type
	 * @tparam BITS_PER_PASS The number of bits per pass
	 * @tparam KEY_BITS_EFFECTIVE The number of effective (lower) bits in the key
	 */
	template <typename IndexType, typename KeyType, int BITS_PER_PASS, int KEY_BITS_EFFECTIVE>
	struct alignas(32) RadixSort_CPU_seq {
		static constexpr int KEY_TYPE_BITS = sizeof(KeyType) * 8;
		// e.g. 256 if BITS_PER_PASS=8
		static constexpr int BUCKETS = 1 << BITS_PER_PASS;
		// e.g. 8 passes if KEY_BITS_EFFECTIVE=64 and BITS_PER_PASS=8
		static constexpr int PASSES = static_cast<int>(KEY_BITS_EFFECTIVE / BITS_PER_PASS) +
			static_cast<int>(KEY_BITS_EFFECTIVE % BITS_PER_PASS != 0);
		// e.g. 8 if KeyType is uint32_t to fill 256-bit SIMD register
		static constexpr int KEYS_PER_SIMD_PASS = 256 / KEY_TYPE_BITS;
		// whether the sorting starts with the temporary buffer
		static constexpr bool START_WITH_TMP = ((PASSES % 2) == 1);

		std::vector<IndexType> tmp_indices_;
		std::vector<uint32_t> histogram_;
		alignas(32) KeyType tmpBins_[KEYS_PER_SIMD_PASS] = {0};
		alignas(32) int32_t tmpKeys32[8] = {0};

		/**
		 * @brief Constructor
		 * @param maxNumKeys The maximum number of keys to sort, actual number can be smaller
		 */
		explicit RadixSort_CPU_seq(size_t maxNumKeys) : histogram_(BUCKETS) {
			tmp_indices_.reserve(maxNumKeys);
		}

		/**
		 * @brief Sort the indices based on the keys
		 * @param indices The indices to sort
		 * @param keys The keys corresponding to the indices
		 */
		void sort(std::vector<IndexType> &indices, const std::vector<KeyType> &keys) {
			// The actual number of keys to sort for this sort.
			// This can be less than the reserved maximum number of keys.
			// NOTE: The size of keys and indices must not match, only requirement
			//       is that values in indices are valid indices into keys.
			const uint32_t numKeys = indices.size();
			tmp_indices_.resize(numKeys);

			IndexType *src, *dst;
			// Make sure the last pass writes to `indices` buffer
			if constexpr (START_WITH_TMP) {
				// swap data within the std::vector to avoid an extra copy
				indices.swap(tmp_indices_);
				src = tmp_indices_.data();
				dst = indices.data();
			} else {
				src = indices.data();
				dst = tmp_indices_.data();
			}
			// Perform the radix passes
			unrolledRadix<0,PASSES>(numKeys, src, dst, keys.data());
		}

		private:
		// Unroll radix passes at compile time
		template<int I, int N> constexpr void unrolledRadix(uint32_t numKeys,
					IndexType *&src, IndexType *&dst, const KeyType *keys) {
			if constexpr (I < N) {
				// Perform a single radix pass.
				// We do attempt to run some parts with SIMD if possible, the histogram
				// counting can benefit from it easily.
				// However, for huge gain we would need to load the histogram into
				// SIMD registers as well, which is only practical with small radix width.
				constexpr int SHIFT = I * BITS_PER_PASS;
				// Limit bit width for last pass.
				constexpr int BITS_THIS_PASS = (I+1 == N &&
					(KEY_BITS_EFFECTIVE % BITS_PER_PASS != 0)) ?
					(KEY_BITS_EFFECTIVE % BITS_PER_PASS) : BITS_PER_PASS;
				constexpr int MASK = (1 << BITS_THIS_PASS) - 1;
				std::ranges::fill(histogram_, 0);

				// 1. Histogram: Count how many keys fall into each bucket
				radixHistogram<SHIFT,MASK>(numKeys, src, keys);
				// 2. Prefix sum: Compute the starting index for each bucket
				size_t sum = 0;
				for (int i = 0; i < BUCKETS; ++i) {
					const uint32_t tmp = histogram_[i];
					histogram_[i] = sum;
					sum += tmp;
				}
				// 3. Scatter: Place each key in its corresponding bucket
				for (uint32_t i = 0u; i < numKeys; ++i) {
					const auto digit = (keys[src[i]] >> SHIFT) & MASK;
					dst[histogram_[digit]++] = src[i];
				}
				// Swap buffers
				std::swap(src, dst);
				// Recurse to the next pass
				unrolledRadix<I + 1, N>(numKeys, src, dst, keys);
			}
		}

		template <int SHIFT, int MASK>
		void radixHistogram(const uint32_t numKeys, IndexType *src, const KeyType *keys) {
			// Histogram: Count how many keys fall into each bucket
			// note: Depending on key type we need different API for SIMD unfortunately, as only
			//       256-bit registers are available with AVX, so we can only load
			//       e.g. 8 uint32_t keys or 4 uint64_t keys in one go.
			uint32_t keyIdx = 0;
#ifdef REGEN_RADIX_SORT_CPU_USE_SIMD
			{
				// Load constant: (BUCKETS-1) across SIMD lane of 256-bit register
				simd::Register_i mask;
				if constexpr (KEY_TYPE_BITS == 16) {
					mask = simd::set1_epi16(MASK);
				} else if constexpr (KEY_TYPE_BITS == 32) {
					mask = simd::set1_epi32(MASK);
				} else if constexpr (KEY_TYPE_BITS == 64) {
					mask = simd::set1_epi64u(MASK);
				} else {
					static_assert("Unsupported key size for SIMD radix sort");
					return;
				}

				while (keyIdx+KEYS_PER_SIMD_PASS <= numKeys) {
					simd::Register_i r0;

					if constexpr (KEY_TYPE_BITS == 16) {
						// Gather 8 keys manually, and promote to 32-bit
						for (int k = 0; k < 8; ++k) tmpKeys32[k] = static_cast<int32_t>(keys[src[keyIdx+k]]);
						r0 = _mm256_load_si256(reinterpret_cast<const __m256i*>(tmpKeys32));
						r0 = _mm256_and_si256(_mm256_srli_epi32(r0, SHIFT), mask);
						keyIdx += 8; // processed 8 keys, not 16!
					}
					else if constexpr (KEY_TYPE_BITS == 32) {
						simd::Register_i idx = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&src[keyIdx]));
						// Gather 8 scattered keys, and apply shift and mask to get bucket ids
						r0 = _mm256_i32gather_epi32(reinterpret_cast<const int*>(keys), idx, 4);
						r0 = _mm256_and_si256(_mm256_srli_epi32(r0, SHIFT), mask);
						keyIdx += KEYS_PER_SIMD_PASS;
					}
					else if constexpr (KEY_TYPE_BITS == 64) {
						// note: values have 32 bits, use __m128i to load only 4
						__m128i idx32 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&src[keyIdx]));
						// Gather 4 scattered keys, and apply shift and mask to get bucket ids
						r0 = _mm256_i32gather_epi64(reinterpret_cast<const long long*>(keys), idx32, 8);
						r0 = _mm256_and_si256(_mm256_srli_epi64(r0, SHIFT), mask);
						keyIdx += KEYS_PER_SIMD_PASS;
					}
					else {
						static_assert("Unsupported key size for SIMD radix sort");
						break;
					}
					// Store results into tmpBins_ and increment histogram
					_mm256_storeu_si256(reinterpret_cast<__m256i*>(tmpBins_), r0);
					for (auto x : tmpBins_) ++histogram_[x];
				}
			}
#endif
			for (; keyIdx < numKeys; ++keyIdx) {
				++histogram_[(keys[src[keyIdx]] >> SHIFT) & MASK];
			}
		}
	};
} // regen

#endif //REGEN_RADIX_SORT_CPU_H
