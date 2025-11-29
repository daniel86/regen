#ifndef REGEN_SIMD_H_
#define REGEN_SIMD_H_

#include <regen/math/vector.h>
#include <regen/utility/aligned-allocator.h>

// NOTE: Check for REGEN_HAS_SIMD, if it is not defined, the SIMD operations will be disabled
//       and the code here will fall back to scalar operations.
// NOLINTBEGIN(portability-simd-intrinsics)
#if defined(__AVX__)
	#include <immintrin.h> // AVX
	#define REGEN_SIMD_MODE AVX
	#define REGEN_SIMD_WIDTH 8
	#define REGEN_HAS_SIMD
#elif defined(__SSE__)
	#include <xmmintrin.h> // SSE
	#define REGEN_SIMD_MODE SSE
	#define REGEN_SIMD_WIDTH 4
	#define REGEN_HAS_SIMD
#else
	#define REGEN_SIMD_MODE NONE
	#define REGEN_SIMD_WIDTH 1
#endif

namespace regen::simd {
	static constexpr int32_t RegisterWidth = REGEN_SIMD_WIDTH;

	template<typename MaskType>
	static int nextBitIndex(MaskType &mask) {
		int bitIndex = __builtin_ctz(mask);
		mask &= (mask - 1);
		return bitIndex;
	}

#if REGEN_SIMD_MODE == AVX
	static constexpr int8_t RegisterMask = 0xFF; // 8 bits for AVX
	using Register = __m256; // 8 floats
	using Register_i = __m256i; // 8 integers

	inline __m256 set1_ps(float v) { return _mm256_set1_ps(v); }
	inline __m256i set1_epi32(int32_t v) { return _mm256_set1_epi32(v); }
	inline __m256i set1_epi16(uint16_t v) { return _mm256_set1_epi16(v); }
	inline __m256i set1_epi64(int64_t v) { return _mm256_set1_epi64x(v); }
	inline __m256i set1_epi64u(uint64_t v) { return _mm256_set1_epi64x(v); }

	inline __m256 setzero_ps() { return _mm256_setzero_ps(); }
	inline __m256i setzero_si256() { return _mm256_setzero_si256(); }

	inline __m256 load_ps(const float *p) { return _mm256_load_ps(p); }
	inline __m256 loadu_ps(const float *p) { return _mm256_loadu_ps(p); }

	inline __m256i load_si256(const uint16_t *p) {
		return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
	}
	inline __m256i load_si256(const uint32_t *p) {
		return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
	}
	inline __m256i load_si256(const uint64_t *p) {
		return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
	}
	inline __m256i load_si256(const int32_t *p) {
		return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
	}

	inline __m256i loadu_si256(const uint16_t *p) {
		return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
	}
	inline __m256i loadu_si256(const uint32_t *p) {
		return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
	}
	inline __m256i loadu_si256(const uint64_t *p) {
		return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
	}
	inline __m256i loadu_si256(const int32_t *p) {
		return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
	}

	inline __m256 epi_to_ps(const __m256i &v) { return _mm256_castsi256_ps(v); }

	inline __m256 i32gather_ps(const float *p, const __m256i &indices) {
		return _mm256_i32gather_ps(p, indices, sizeof(float));
	}

	inline void storeu_ps(float *p, const __m256 &v) { _mm256_storeu_ps(p, v); }
	inline void store_ps(float *p, const __m256 &v) { _mm256_store_ps(p, v); }

	inline void storeu_epi32(int32_t *p, const __m256i &v) {
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v);
	}
	inline void storeu_epi32(uint32_t *p, const __m256i &v) {
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v);
	}
	inline void store_epi32(int32_t *p, const __m256i &v) {
		_mm256_store_si256(reinterpret_cast<__m256i*>(p), v);
	}
	inline void store_epi32(uint32_t *p, const __m256i &v) {
		_mm256_store_si256(reinterpret_cast<__m256i*>(p), v);
	}

	inline __m256 add_ps(const __m256 &a, const __m256 &b) { return _mm256_add_ps(a, b); }
	inline __m256 sub_ps(const __m256 &a, const __m256 &b) { return _mm256_sub_ps(a, b); }
	inline __m256 mul_ps(const __m256 &a, const __m256 &b) { return _mm256_mul_ps(a, b); }
	inline __m256 div_ps(const __m256 &a, const __m256 &b) { return _mm256_div_ps(a, b); }

	/**
	 * Fused multiply-add: (a * b) + c
	 */
	inline __m256 mul_add_ps(const __m256 &a, const __m256 &b, const __m256 &c) {
		return _mm256_fmadd_ps(a, b, c);
	}

	inline __m256i add_epi32(const __m256i &a, const __m256i &b) { return _mm256_add_epi32(a, b); }
	inline __m256i sub_epi32(const __m256i &a, const __m256i &b) { return _mm256_sub_epi32(a, b); }
	inline __m256i mul_epi32(const __m256i &a, const __m256i &b) { return _mm256_mullo_epi32(a, b); }

	inline __m256 min_ps(const __m256 &a, const __m256 &b) { return _mm256_min_ps(a, b); }
	inline __m256 max_ps(const __m256 &a, const __m256 &b) { return _mm256_max_ps(a, b); }
	inline __m256 sqrt_ps(const __m256 &a) { return _mm256_sqrt_ps(a); }

	inline __m256i min_epi32(const __m256i &a, const __m256i &b) { return _mm256_min_epi32(a, b); }
	inline __m256i max_epi32(const __m256i &a, const __m256i &b) { return _mm256_max_epi32(a, b); }

	/**
	 * Horizontal sum of all elements in an __m256
	 */
	inline float hsum_ps(__m256 v) {
		__m128 vlow  = _mm256_castps256_ps128(v);        // low 128
		__m128 vhigh = _mm256_extractf128_ps(v, 1);   // high 128
		__m128 sum   = _mm_add_ps(vlow, vhigh);       // add low and high parts
		__m128 shuf  = _mm_movehdup_ps(sum);             // (sum.y, sum.y, sum.w, sum.w)
		__m128 sums  = _mm_add_ps(sum, shuf);
		shuf         = _mm_movehl_ps(shuf, sums);     // high half of sums
		sums         = _mm_add_ss(sums, shuf);
		return _mm_cvtss_f32(sums);
	}

	inline __m256 rcp_ps(const __m256 &a) { return _mm256_rcp_ps(a); }

	inline __m256 cmp_lt(const __m256 &a, const __m256 &b) {
		return _mm256_cmp_ps(a, b, _CMP_LT_OQ);
	}
	inline __m256 cmp_gt(const __m256 &a, const __m256 &b) {
		return _mm256_cmp_ps(a, b, _CMP_GT_OQ);
	}
	inline __m256 cmp_eq(const __m256 &a, const __m256 &b) {
		return _mm256_cmp_ps(a, b, _CMP_EQ_OQ);
	}
	inline __m256 cmp_neq(const __m256 &a, const __m256 &b) {
		return _mm256_cmp_ps(a, b, _CMP_NEQ_OQ);
	}
	inline __m256 cmp_or(const __m256 &a, const __m256 &b) {
		return _mm256_or_ps(a, b);
	}
	inline __m256 cmp_and(const __m256 &a, const __m256 &b) {
		return _mm256_and_ps(a, b);
	}
	inline __m256 cmp_and_not(const __m256 &a, const __m256 &b) {
		return _mm256_andnot_ps(a, b);
	}

	inline __m256i cvttps_epi32(const __m256 &a) { return _mm256_cvttps_epi32(a); }

	inline int movemask_ps(const __m256 &v) { return _mm256_movemask_ps(v); }

	inline __m256 blendv_ps(const __m256 &a, const __m256 &b, const __m256 &mask) {
		return _mm256_blendv_ps(a, b, mask);
	}

#elif REGEN_SIMD_MODE == SSE
	static constexpr int8_t RegisterMask = 0x0F; // 4 bits for SSE
	using Register = __m128; // 4 floats
	using Register_i = __m128i; // 4 integers

	inline __m128 set1_ps(float v) { return _mm_set1_ps(v); }
	inline __m128i set1_epi32(int32_t v) { return _mm_set1_epi32(v); }

	inline __m128 setzero_ps() { return _mm_setzero_ps(); }
	inline __m128i setzero_si256() { return _mm_setzero_si128(); }

	inline __m128 load_ps(const float *p) { return _mm_load_ps(p); }
	inline __m128 loadu_ps(const float *p) { return _mm_loadu_ps(p); }

	inline __m128i loadu_si256(const uint32_t *p) {
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(indices));
	}

	inline __m128 epi_to_ps(const __m128i &v) { return _mm_castsi128_ps(v); }

	inline __m128 i32gather_ps(const float *p, const __m128i &indices) {
		return _mm_i32gather_ps(p, indices, sizeof(float));
	}

	inline void storeu_ps(float *p, const __m128 &v) { _mm_storeu_ps(p, v); }

	inline __m128 add_ps(const __m128 &a, const __m128 &b) { return _mm_add_ps(a, b); }
	inline __m128 sub_ps(const __m128 &a, const __m128 &b) { return _mm_sub_ps(a, b); }
	inline __m128 mul_ps(const __m128 &a, const __m128 &b) { return _mm_mul_ps(a, b); }
	inline __m128 div_ps(const __m128 &a, const __m128 &b) { return _mm_div_ps(a, b); }

	inline __m128i add_epi32(const __m128i &a, const __m128i &b) { return _mm_add_epi32(a, b); }
	inline __m128i sub_epi32(const __m128i &a, const __m128i &b) { return _mm_sub_epi32(a, b); }
	inline __m128i mul_epi32(const __m128i &a, const __m128i &b) { return _mm_mullo_epi32(a, b); }

	inline __m128 min_ps(const __m128 &a, const __m128 &b) { return _mm_min_ps(a, b); }
	inline __m128 max_ps(const __m128 &a, const __m128 &b) { return _mm_max_ps(a, b); }
	inline __m128 sqrt_ps(const __m128 &a) { return _mm_sqrt_ps(a); }

	inline __m128i min_epi32(const __m128i &a, const __m128i &b) { return _mm_min_epi32(a, b); }
	inline __m128i max_epi32(const __m128i &a, const __m128i &b) { return _mm_max_epi32(a, b); }

	inline float hsum_ps(__m128 v) {
		__m128 shuf = _mm_movehdup_ps(v);  // (v1, v1, v3, v3)
		__m128 sums = _mm_add_ps(v, shuf);
		shuf = _mm_movehl_ps(shuf, sums); // (v2 + v3, v3, -, -)
		sums = _mm_add_ss(sums, shuf);
		return _mm_cvtss_f32(sums);
	}

	inline __m128 rcp_ps(const __m128 &a) { return _mm_rcp_ps(a); }

	inline __m128 cmp_lt(const __m128 &a, const __m128 &b)  { return _mm_cmplt_ps(a, b); }
	inline __m128 cmp_gt(const __m128 &a, const __m128 &b)  { return _mm_cmplt_ps(b, a); }
	inline __m128 cmp_eq(const __m128 &a, const __m128 &b)  { return _mm_cmpeq_ps(a, b); }
	inline __m128 cmp_neq(const __m128 &a, const __m128 &b) {
		__m128 eq = _mm_cmpeq_ps(a, b);
		return _mm_andnot_ps(eq, _mm_castsi128_ps(_mm_set1_epi32(-1)));  // ~eq & all_ones
	}
	inline __m128 cmp_or(const __m128 &a, const __m128 &b) { return _mm_or_ps(a, b); }
	inline __m128 cmp_and(const __m128 &a, const __m128 &b) { return _mm_and_ps(a, b); }

	inline __m128i cvttps_epi32(const __m128 &a) { return _mm_cvttps_epi32(a); }

	inline int movemask_ps(const __m128 &v) { return _mm_movemask_ps(v); }

	inline __m128 blendv_ps(const __m128 &a, const __m128 &b, const __m128 &mask) {
		return _mm_blendv_ps(a, b, mask);
	}

#else // Fallback to scalar operations
	static constexpr int8_t RegisterMask = 0x01; // 1 bit for scalar
	using Register = float; // scalar

	inline float set1_ps(float v) { return v; }
	inline int32_t set1_epi32(int32_t v) { return v; }

	inline float load_ps(const float *p) { return *p; }
	inline float loadu_ps(const float *p) { return *p; }

	inline int loadu_si256(const uint32_t *p) {
		return static_cast<int>(*reinterpret_cast<const float *>(p));
	}

	inline void storeu_ps(float *p, const float &v) { *p = v; }

	inline float add_ps(const float &a, const float &b) { return a + b; }
	inline float sub_ps(const float &a, const float &b) { return a - b; }
	inline float mul_ps(const float &a, const float &b) { return a * b; }
	inline float div_ps(const float &a, const float &b) { return a / b; }

	inline int add_epi32(const int &a, const int &b) { return a + b; }
	inline int sub_epi32(const int &a, const int &b) { return a - b; }
	inline int mul_epi32(const int &a, const int &b) { return a * b; }

	inline float min_ps(const float &a, const float &b) { return a < b ? a : b; }
	inline float max_ps(const float &a, const float &b) { return a > b ? a : b; }
	inline float sqrt_ps(const float &a) { return std::sqrt(a); }

	inline float cmp_lt(const float &a, const float &b)  { return a < b ? 1.0f : 0.0f; }
	inline float cmp_gt(const float &a, const float &b)  { return a > b ? 1.0f : 0.0f; }
	inline float cmp_eq(const float &a, const float &b)  { return a == b ? 1.0f : 0.0f; }
	inline float cmp_neq(const float &a, const float &b) { return a != b ? 1.0f : 0.0f; }
	inline float cmp_or(const float &a, const float &b) { return a || b ? 1.0f : 0.0f; }
	inline float cmp_and(const float &a, const float &b) { return a && b ? 1.0f : 0.0f; }

	inline int movemask_ps(const float &v) { return (v != 0.0f) ? 1 : 0; }
#endif
}

namespace regen {
	/**
	 * SIMD batch of float values.
	 * Supports basic arithmetic and comparison operations.
	 */
	struct BatchOf_float {
		// Underlying SIMD register
		simd::Register c;

		/**
		 * Construct a batch from a single scalar float value.
		 * All elements in the batch will be set to this value.
		 * @param v The scalar float value to set.
		 * @return A BatchOf_float with all elements set to v.
		 */
		static BatchOf_float fromScalar(float v) {
			return BatchOf_float{simd::set1_ps(v)};
		}

		/**
		 * Load batch from an array of aligned floats.
		 * @param src Pointer to array of floats.
		 * @return A BatchOf_float loaded from the aligned array.
		 */
		static BatchOf_float loadAligned(const float *src) {
			return BatchOf_float{simd::load_ps(src)};
		}

		/**
		 * Load batch from an array of unaligned floats.
		 * @param src Pointer to array of floats.
		 * @return A BatchOf_float loaded from the unaligned array.
		 */
		static BatchOf_float loadUnaligned(const float *src) {
			return BatchOf_float{simd::loadu_ps(src)};
		}

		/**
		 * Gather load from an array of floats using SIMD indices.
		 * @param basePtr Pointer to the base of the float array.
		 * @param indices SIMD register containing indices to gather.
		 * @return A BatchOf_float loaded from the gathered indices.
		 */
		static BatchOf_float loadGather(const float *basePtr, simd::Register_i indices) {
			return BatchOf_float{simd::i32gather_ps(basePtr, indices)};
		}

		/**
		 * @return A batch where all elements are set to 1.0f.
		 */
		static BatchOf_float allOnes() {
			return BatchOf_float{simd::setzero_ps()}.cmp_lt(1.0f);
		}

		/**
		 * @return A batch where all elements are set to 0.0f.
		 */
		static BatchOf_float allZeros() {
			return BatchOf_float{simd::setzero_ps()};
		}

		/**
		 * Store the batch to an array of aligned floats.
		 * @param dst Pointer to destination array.
		 */
		void storeAligned(float *dst) const {
			simd::store_ps(dst, c);
		}

		/**
		 * Store the batch to an array of unaligned floats.
		 * @param dst Pointer to destination array.
		 */
		void storeUnaligned(float *dst) const {
			simd::storeu_ps(dst, c);
		}

		/**
		 * Addition operator overloads.
		 * @param other The other batch to add.
		 * @return A new BatchOf_float containing the result.
		 */
		BatchOf_float operator+(const BatchOf_float &other) const {
			return BatchOf_float{simd::add_ps(c, other.c)};
		}
		BatchOf_float operator+(float scalar) const {
			return BatchOf_float{simd::add_ps(c, simd::set1_ps(scalar))};
		}
		void operator+=(const BatchOf_float &other) {
			c = simd::add_ps(c, other.c);
		}
		void operator+=(float scalar) {
			c = simd::add_ps(c, simd::set1_ps(scalar));
		}

		/**
		 * Subtraction operator overloads.
		 * @param other The other batch to subtract.
		 * @return A new BatchOf_float containing the result.
		 */
		BatchOf_float operator-(const BatchOf_float &other) const {
			return BatchOf_float{simd::sub_ps(c, other.c)};
		}
		BatchOf_float operator-(float scalar) const {
			return BatchOf_float{simd::sub_ps(c, simd::set1_ps(scalar))};
		}
		void operator-=(const BatchOf_float &other) {
			c = simd::sub_ps(c, other.c);
		}
		void operator-=(float scalar) {
			c = simd::sub_ps(c, simd::set1_ps(scalar));
		}

		/**
		 * Division operator overloads.
		 * @param other The other batch to divide by.
		 * @return A new BatchOf_float containing the result.
		 */
		BatchOf_float operator/(const BatchOf_float &other) const {
			return BatchOf_float{simd::div_ps(c, other.c)};
		}
		BatchOf_float operator/(float scalar) const {
			return BatchOf_float{simd::div_ps(c, simd::set1_ps(scalar))};
		}
		void operator/=(const BatchOf_float &other) {
			c = simd::div_ps(c, other.c);
		}
		void operator/=(float scalar) {
			c = simd::div_ps(c, simd::set1_ps(scalar));
		}

		/**
		 * Multiplication operator overloads.
		 * @param other The other batch to multiply by.
		 * @return A new BatchOf_float containing the result.
		 */
		BatchOf_float operator*(const BatchOf_float &other) const {
			return BatchOf_float{simd::mul_ps(c, other.c)};
		}
		BatchOf_float operator*(float scalar) const {
			return BatchOf_float{simd::mul_ps(c, simd::set1_ps(scalar))};
		}
		void operator*=(const BatchOf_float &other) {
			c = simd::mul_ps(c, other.c);
		}
		void operator*=(float scalar) {
			c = simd::mul_ps(c, simd::set1_ps(scalar));
		}

		/**
		 * Comparison operator overloads.
		 * @param other The other batch to compare with.
		 * @return A new BatchOf_float containing the comparison results.
		 */
		template <typename T>
		BatchOf_float operator<(const T &other) const { return cmp_lt(other); }
		template <typename T>
		BatchOf_float operator>(const T &other) const { return cmp_gt(other); }
		template <typename T>
		BatchOf_float operator>=(const T &other) const { return other.cmp_lt(*this); }
		template <typename T>
		BatchOf_float operator<=(const T &other) const { return other.cmp_gt(*this); }

		void operator&=(const BatchOf_float &other) {
			c = simd::cmp_and(c, other.c);
		}
		void operator|=(const BatchOf_float &other) {
			c = simd::cmp_or(c, other.c);
		}
		template <typename T>
		BatchOf_float operator&&(const T &other) const { return cmp_and(other); }
		template <typename T>
		BatchOf_float operator||(const T &other) const { return cmp_or(other); }

		/**
		 * Compute the minimum of two batches element-wise.
		 * @param a The first batch.
		 * @param b The second batch.
		 * @return A BatchOf_float containing the minimum values.
		 */
		static BatchOf_float min(const BatchOf_float &a, const BatchOf_float &b) {
			return BatchOf_float{simd::min_ps(a.c, b.c)};
		}

		/**
		 * Compute the maximum of two batches element-wise.
		 * @param a The first batch.
		 * @param b The second batch.
		 * @return A BatchOf_float containing the maximum values.
		 */
		static BatchOf_float max(const BatchOf_float &a, const BatchOf_float &b) {
			return BatchOf_float{simd::max_ps(a.c, b.c)};
		}

		/**
		 * Load a single float value into the batch.
		 */
		void setScalar(float v) { c = simd::set1_ps(v); }

		/**
		 * Check if each element in the batch is positive.
		 * @return A BatchOf_float where each element is 1.0f if the corresponding element in this batch is positive, else 0.0f.
		 */
		BatchOf_float isPositive() const {
			return BatchOf_float{simd::cmp_gt(c, simd::setzero_ps())};
		}

		/**
		 * Check if each element in the batch is negative.
		 * @return A BatchOf_float where each element is 1.0f if the corresponding element in this batch is negative, else 0.0f.
		 */
		BatchOf_float isNegative() const {
			return BatchOf_float{simd::cmp_lt(c, simd::setzero_ps())};
		}

		/**
		 * @return True if all elements in the batch are zero.
		 */
		bool isAllZero() const {
			return simd::movemask_ps(c) == 0;
		}

		/**
		 * Compute the absolute value of each element in the batch.
		 * @return A BatchOf_float where each element is the absolute value of the corresponding element in this batch.
		 */
		BatchOf_float abs() const {
			// 32-bit integer with all bits 1 except the sign bit,
			// then reinterpret the integer bits as floats
			simd::Register mask = simd::epi_to_ps(simd::set1_epi32(0x7FFFFFFF));
			// AND the mask with the float values to clear the sign bits
			return BatchOf_float{simd::cmp_and(this->c, mask)};
		}

		/**
		 * Compute the reciprocal of each element in the batch.
		 * This can be more efficient than division.
		 * @return A BatchOf_float where each element is the reciprocal of the corresponding element in this batch.
		 */
		BatchOf_float reciprocal() const {
			// First approximation (12-bit accurate)
			simd::Register y = simd::rcp_ps(c);
			// Newton-Raphson: y = y * (2 - x*y)
			simd::Register xy = simd::mul_ps(c, y);
			simd::Register two_minus_xy = simd::sub_ps(simd::set1_ps(2.0f), xy);
			return BatchOf_float{simd::mul_ps(y, two_minus_xy)};
		}

		/**
		 * Less-than comparison.
		 * @param other The other batch to compare with.
		 * @return A BatchOf_float where each element is 1.0f if the corresponding element in this batch is less than the other batch, else 0.0f.
		 */
		BatchOf_float cmp_lt(const BatchOf_float &other) const {
			return BatchOf_float{simd::cmp_lt(c, other.c)};
		}

		/**
		 * Less-than comparison with a scalar float.
		 * @param other The scalar float to compare with.
		 * @return A BatchOf_float where each element is 1.0f if the corresponding element in this batch is less than the scalar, else 0.0f.
		 */
		BatchOf_float cmp_lt(float other) const {
			return BatchOf_float{simd::cmp_lt(c, simd::set1_ps(other))};
		}

		/**
		 * Greater-than comparison.
		 * @param other The other batch to compare with.
		 * @return A BatchOf_float where each element is 1.0f if the corresponding element in this batch is greater than the other batch, else 0.0f.
		 */
		BatchOf_float cmp_gt(const BatchOf_float &other) const {
			return BatchOf_float{simd::cmp_gt(c, other.c)};
		}

		/**
		 * Greater-than comparison with a scalar float.
		 * @param other The scalar float to compare with.
		 * @return A BatchOf_float where each element is 1.0f if the corresponding element in this batch is greater than the scalar, else 0.0f.
		 */
		BatchOf_float cmp_gt(float other) const {
			return BatchOf_float{simd::cmp_gt(c, simd::set1_ps(other))};
		}

		/**
		 * Compute the logical AND of two comparison masks.
		 * @param other The other comparison mask batch.
		 * @return A BatchOf_float representing the logical AND of the two masks.
		 */
		BatchOf_float cmp_and(const BatchOf_float &other) const {
			return BatchOf_float{simd::cmp_and(c, other.c)};
		}

		/**
		 * Compute the logical AND of this comparison mask with a scalar float mask.
		 * @param other The scalar float mask.
		 * @return A BatchOf_float representing the logical AND of the mask with the scalar.
		 */
		BatchOf_float cmp_and(float other) const {
			return BatchOf_float{simd::cmp_and(c, simd::set1_ps(other))};
		}

		/**
		 * Compute the logical OR of two comparison masks.
		 * @param other The other comparison mask batch.
		 * @return A BatchOf_float representing the logical OR of the two masks.
		 */
		BatchOf_float cmp_or(const BatchOf_float &other) const {
			return BatchOf_float{simd::cmp_or(c, other.c)};
		}

		/**
		 * Compute the logical OR of this comparison mask with a scalar float mask.
		 * @param other The scalar float mask.
		 * @return A BatchOf_float representing the logical OR of the mask with the scalar.
		 */
		BatchOf_float cmp_or(float other) const {
			return BatchOf_float{simd::cmp_or(c, simd::set1_ps(other))};
		}

		/**
		 * Convert the comparison mask to a float batch.
		 * Each element in the returned batch will be 1.0f if the corresponding element in this batch is non-zero, else 0.0f.
		 * @return A BatchOf_float representing the mask as floats.
		 */
		BatchOf_float maskToFloat() const {
			return { simd::cmp_and(c, simd::set1_ps(1.0f)) };
		}

		/**
		 * Convert the comparison mask to an 8-bit bitmask.
		 * Each bit in the returned byte corresponds to an element in the batch.
		 * @return An 8-bit bitmask representing the comparison results.
		 */
		uint8_t toBitmask8() const {
			return static_cast<uint8_t>(simd::movemask_ps(c) & 0xFF);
		}

		/**
		 * Blend two batches based on a mask.
		 * For each element, if the corresponding element in the mask has its sign bit set,
		 * the element from 'other' is selected; otherwise, the element from this batch is retained.
		 * @param other The other batch to blend with.
		 * @param mask The mask batch determining which elements to select from 'other'.
		 * @return A blended BatchOf_float.
		 */
		BatchOf_float blend(const BatchOf_float &other, const BatchOf_float &mask) const {
			// select elements from 'other' where the sign bit of 'mask' is set
			return BatchOf_float{ simd::blendv_ps(c, other.c, mask.c)};
		}
	};

	/**
	 * SIMD batch of int values.
	 */
	struct BatchOf_int32 {
		simd::Register_i c;

		/**
		 * Load batch from an array of unaligned int32_t.
		 * @param v Pointer to array of int32_t.
		 * @return A BatchOf_int32 loaded from the unaligned array.
		 */
		template <typename IntType>
		static BatchOf_int32 loadAligned(const IntType *v) {
			return BatchOf_int32{simd::load_si256(v)};
		}

		/**
		 * Load batch from an array of unaligned int32_t.
		 * @param v Pointer to array of int32_t.
		 * @return A BatchOf_int32 loaded from the unaligned array.
		 */
		template <typename IntType>
		static BatchOf_int32 loadUnaligned(const IntType *v) {
			return BatchOf_int32{simd::loadu_si256(v)};
		}

		/**
		 * Constructor that initializes the batch with a single scalar int32_t value.
		 * All elements in the batch will be set to this value.
		 * @param v The scalar int32_t value to set.
		 * @return A BatchOf_int32 with all elements set to v.
		 */
		template <typename IntType>
		static BatchOf_int32 fromScalar(IntType v) {
			return BatchOf_int32{simd::set1_epi32(v)};
		}

		/**
		 * Fill an array of aligned int32_t with a specified value using SIMD.
		 * @param data Pointer to the aligned int32_t array to fill.
		 * @param numElements Number of elements to fill.
		 * @param value The int32_t value to fill the array with.
		 */
		template <typename IntType>
		static void fillAligned(IntType* __restrict data, size_t numElements, IntType value) {
			size_t i = 0;
			BatchOf_int32 v = fromScalar(value);
			for (; i + simd::RegisterWidth <= numElements; i += simd::RegisterWidth) {
				v.storeAligned(data + i);
			}
			// Tail loop (in case size is not divisible by 8)
			for (; i < numElements; ++i) { data[i] = value; }
		}

		/**
		 * Fill an array of unaligned int32_t with a specified value using SIMD.
		 * @param data Pointer to the unaligned int32_t array to fill.
		 * @param numElements Number of elements to fill.
		 * @param value The int32_t value to fill the array with.
		 */
		template <typename IntType>
		static void fillUnaligned(IntType* __restrict data, size_t numElements, IntType value) {
			size_t i = 0;
			BatchOf_int32 v = fromScalar(value);
			for (; i + simd::RegisterWidth <= numElements; i += simd::RegisterWidth) {
				v.storeUnaligned(data + i);
			}
			// Tail loop (in case size is not divisible by 8)
			for (; i < numElements; ++i) { data[i] = value; }
		}

		static BatchOf_int32 castFloatBatch(const BatchOf_float &v) {
			return BatchOf_int32{_mm256_castps_si256(v.c)};
		}

		/**
		 * Compute the element-wise minimum of two batches.
		 * @param a The first batch.
		 * @param b The second batch.
		 * @return A BatchOf_int32 containing the element-wise minimums.
		 */
		static BatchOf_int32 min(const BatchOf_int32 &a, const BatchOf_int32 &b) {
			return BatchOf_int32{simd::min_epi32(a.c, b.c)};
		}

		/**
		 * Compute the element-wise maximum of two batches.
		 * @param a The first batch.
		 * @param b The second batch.
		 * @return A BatchOf_int32 containing the element-wise maximums.
		 */
		static BatchOf_int32 max(const BatchOf_int32 &a, const BatchOf_int32 &b) {
			return BatchOf_int32{simd::max_epi32(a.c, b.c)};
		}

		/**
		 * Store the batch to an array of aligned int32_t.
		 * @param v Pointer to destination array.
		 */
		void storeAligned(int32_t *v) const {
			simd::store_epi32(v, c);
		}

		/**
		 * Store the batch to an array of aligned int32_t.
		 * @param v Pointer to destination array.
		 */
		void storeAligned(uint32_t *v) const {
			simd::store_epi32(v, c);
		}

		/**
		 * Store the batch to an array of unaligned int32_t.
		 * @param v Pointer to destination array.
		 */
		void storeUnaligned(int32_t *v) const {
			simd::storeu_epi32(v, c);
		}

		/**
		 * Store the batch to an array of unaligned uint32_t.
		 * @param v Pointer to destination array.
		 */
		void storeUnaligned(uint32_t *v) const {
			simd::storeu_epi32(v, c);
		}

		BatchOf_int32 operator*(const BatchOf_int32 &other) const {
			return BatchOf_int32{simd::mul_epi32(c, other.c)};
		}

		BatchOf_int32 operator*(int32_t scalar) const {
			return BatchOf_int32{simd::mul_epi32(c, simd::set1_epi32(scalar))};
		}

		BatchOf_int32 operator+(const BatchOf_int32 &other) const {
			return BatchOf_int32{simd::add_epi32(c, other.c)};
		}

		BatchOf_int32 operator+(int32_t scalar) const {
			return BatchOf_int32{simd::add_epi32(c, simd::set1_epi32(scalar))};
		}

		BatchOf_int32 operator-(const BatchOf_int32 &other) const {
			return BatchOf_int32{simd::sub_epi32(c, other.c)};
		}

		BatchOf_int32 operator-(int32_t scalar) const {
			return BatchOf_int32{simd::sub_epi32(c, simd::set1_epi32(scalar))};
		}

		BatchOf_int32 operator&(const BatchOf_int32 &other) const {
			return BatchOf_int32{_mm256_and_si256(c, other.c)};
		}

		static BatchOf_int32 allZeros() {
			return BatchOf_int32{simd::setzero_si256()};
		}

		/**
		 * Set an array of aligned int32_t to zero using SIMD.
		 * @param data Pointer to the aligned int32_t array to zero.
		 * @param size Number of elements to set to zero.
		 */
		template <typename IntType>
		static void zeroAligned(IntType* __restrict data, size_t size) {
			size_t i = 0;
			BatchOf_int32 zero = allZeros();
			for (; i + simd::RegisterWidth <= size; i += simd::RegisterWidth) {
				zero.storeAligned(data + i);
			}
			// Tail loop (in case size is not divisible by 8)
			for (; i < size; ++i) { data[i] = 0; }
		}
	};

	/**
	 * SIMD batch of vec2 values.
	 */
	struct BatchOf_Vec2f {
		BatchOf_float x, y;

		/**
		 * Constructor that initializes the batch with a single Vec2f value.
		 * @param v The Vec2f value to initialize the batch with.
		 */
		static BatchOf_Vec2f fromScalar(const Vec2f &v) {
			return BatchOf_Vec2f{
				BatchOf_float{simd::set1_ps(v.x)},
				BatchOf_float{simd::set1_ps(v.y)}};
		}

		/**
		 * Load a single Vec2f value into the batch.
		 */
		void loadScalar(const Vec2f &v) {
			x.c = simd::set1_ps(v.x);
			y.c = simd::set1_ps(v.y);
		}
	};

	/**
	 * SIMD batch of vec3 integer values.
	 */
	class BatchOf_Vec3i {
	public:
		BatchOf_int32 x, y, z;

		/**
		 * Constructor that initializes the batch with a single Vec3i value.
		 * @param v The Vec3i value to initialize the batch with.
		 */
		static BatchOf_Vec3i fromScalar(const Vec3i &v) {
			BatchOf_Vec3i batch; // NOLINT(cppcoreguidelines-pro-type-member-init)
			batch.x.c = simd::set1_epi32(v.x);
			batch.y.c = simd::set1_epi32(v.y);
			batch.z.c = simd::set1_epi32(v.z);
			return batch;
			return BatchOf_Vec3i{
				BatchOf_int32{simd::set1_epi32(v.x)},
				BatchOf_int32{simd::set1_epi32(v.y)},
				BatchOf_int32{simd::set1_epi32(v.z)}};
		}

		/**
		 * Computes the minimum of this batch and another Vec3iBatch.
		 * @param other Another Vec3iBatch to compare with.
		 * @return A new Vec3iBatch containing the minimum values.
		 */
		BatchOf_Vec3i min(const BatchOf_Vec3i &other) const {
			return BatchOf_Vec3i{
				BatchOf_int32{simd::min_epi32(x.c, other.x.c)},
				BatchOf_int32{simd::min_epi32(y.c, other.y.c)},
				BatchOf_int32{simd::min_epi32(z.c, other.z.c)}};
		}

		BatchOf_Vec3i operator-(const BatchOf_Vec3i &other) const {
			return BatchOf_Vec3i{
				BatchOf_int32{simd::sub_epi32(x.c, other.x.c)},
				BatchOf_int32{simd::sub_epi32(y.c, other.y.c)},
				BatchOf_int32{simd::sub_epi32(z.c, other.z.c)}};
		}

		BatchOf_Vec3i operator-(const BatchOf_int32 &other) const {
			return BatchOf_Vec3i{
				BatchOf_int32{simd::sub_epi32(x.c, other.c)},
				BatchOf_int32{simd::sub_epi32(y.c, other.c)},
				BatchOf_int32{simd::sub_epi32(z.c, other.c)}};
		}

		BatchOf_Vec3i operator-(const Vec3i& other) const {
			return BatchOf_Vec3i{
				BatchOf_int32{simd::sub_epi32(x.c, simd::set1_epi32(other.x))},
				BatchOf_int32{simd::sub_epi32(y.c, simd::set1_epi32(other.y))},
				BatchOf_int32{simd::sub_epi32(z.c, simd::set1_epi32(other.z))}};
		}

		BatchOf_Vec3i operator-(int32_t scalar) const {
			return BatchOf_Vec3i{
				BatchOf_int32{simd::sub_epi32(x.c, simd::set1_epi32(scalar))},
				BatchOf_int32{simd::sub_epi32(y.c, simd::set1_epi32(scalar))},
				BatchOf_int32{simd::sub_epi32(z.c, simd::set1_epi32(scalar))}};
		}
	};

	/**
	 * SIMD batch of vec3 float values.
	 */
	class BatchOf_Vec3f {
	public:
		BatchOf_float x, y, z;

		/**
		 * Constructor that initializes the batch with a single Vec3f value.
		 * @param v The Vec3f value to initialize the batch with.
		 */
		static BatchOf_Vec3f fromScalar(const Vec3f &v) {
			return BatchOf_Vec3f{
				BatchOf_float{simd::set1_ps(v.x)},
				BatchOf_float{simd::set1_ps(v.y)},
				BatchOf_float{simd::set1_ps(v.z)}};
		}

		/**
		 * Loads a batch of Vec3f from separate arrays.
		 * @param xs Pointer to x-components array.
		 * @param ys Pointer to y-components array.
		 * @param zs Pointer to z-components array.
		 * @return A BatchOf_Vec3f loaded from the arrays.
		 */
		static BatchOf_Vec3f loadAligned(const float *xs, const float *ys, const float *zs) {
			return BatchOf_Vec3f{
				BatchOf_float{simd::load_ps(xs)},
				BatchOf_float{simd::load_ps(ys)},
				BatchOf_float{simd::load_ps(zs)}};
		}

		/**
		 * Loads a batch of Vec3f from separate unaligned arrays.
		 * @param xs Pointer to x-components array.
		 * @param ys Pointer to y-components array.
		 * @param zs Pointer to z-components array.
		 * @return A BatchOf_Vec3f loaded from the unaligned arrays.
		 */
		static BatchOf_Vec3f loadUnaligned(const float *xs, const float *ys, const float *zs) {
			return BatchOf_Vec3f{
				BatchOf_float{simd::loadu_ps(xs)},
				BatchOf_float{simd::loadu_ps(ys)},
				BatchOf_float{simd::loadu_ps(zs)}};
		}

		/**
		 * Loads a batch of Vec3f from separate arrays using indices.
		 * This is useful for gathering data from non-contiguous memory locations.
		 * @param xs Pointer to x-components array.
		 * @param ys Pointer to y-components array.
		 * @param zs Pointer to z-components array.
		 * @param indices SIMD register containing indices to gather.
		 * @return A BatchOf_Vec3f loaded from the arrays using the provided indices.
		 */
		static BatchOf_Vec3f loadGather(const float *xs, const float *ys, const float *zs,
		                                 const simd::Register_i &indices) {
			return BatchOf_Vec3f{
				BatchOf_float{simd::i32gather_ps(xs, indices)},
				BatchOf_float{simd::i32gather_ps(ys, indices)},
				BatchOf_float{simd::i32gather_ps(zs, indices)}};
		}

		/**
		 * Load batch from separate arrays of x, y, z components (SoA layout).
		 * @param xs Pointer to 4 floats representing x-components.
		 * @param ys Pointer to 4 floats representing y-components.
		 * @param zs Pointer to 4 floats representing z-components.
		 */
		void setUnaligned(const float *xs, const float *ys, const float *zs) {
			x.c = simd::loadu_ps(xs);
			y.c = simd::loadu_ps(ys);
			z.c = simd::loadu_ps(zs);
		}

		/**
		 * Load batch from separate aligned arrays of x, y, z components (SoA layout).
		 * @param xs Pointer to 4 floats representing x-components.
		 * @param ys Pointer to 4 floats representing y-components.
		 * @param zs Pointer to 4 floats representing z-components.
		 */
		void setAligned(const float *xs, const float *ys, const float *zs) {
			x.c = simd::load_ps(xs);
			y.c = simd::load_ps(ys);
			z.c = simd::load_ps(zs);
		}

		/**
		 * Loads a batch of Vec3f from separate arrays using indices.
		 * This is useful for gathering data from non-contiguous memory locations.
		 * @param xs Pointer to x-components array.
		 * @param ys Pointer to y-components array.
		 * @param zs Pointer to z-components array.
		 * @param indices SIMD register containing indices to gather.
		 */
		void setGathered(const float *xs, const float *ys, const float *zs, const simd::Register_i &indices) {
			x.c = simd::i32gather_ps(xs, indices);
			y.c = simd::i32gather_ps(ys, indices);
			z.c = simd::i32gather_ps(zs, indices);
		}

		/**
		 * Adds another Vec3fBatch4 to this batch.
		 * @param other Batch to add.
		 * @return Reference to self.
		 */
		BatchOf_Vec3f &operator+=(const BatchOf_Vec3f &other) {
			x = (x + other.x);
			y = (y + other.y);
			z = (z + other.z);
			return *this;
		}

		/**
		 * Adds a constant BatchOf_float to each element in the batch.
		 * @param other BatchOf_float to add.
		 * @return Reference to self.
		 */
		BatchOf_Vec3f &operator+=(const BatchOf_float &other) {
			x = (x + other);
			y = (y + other);
			z = (z + other);
			return *this;
		}

		/**
		 * Adds a constant vector to each element in the batch.
		 * @param v Constant Vec3f to add.
		 * @return Reference to self.
		 */
		BatchOf_Vec3f &operator+=(const Vec3f &v) {
			x = (x + BatchOf_float{simd::set1_ps(v.x)});
			y = (y + BatchOf_float{simd::set1_ps(v.y)});
			z = (z + BatchOf_float{simd::set1_ps(v.z)});
			return *this;
		}

		/**
		 * Adds another Vec3fBatch to this batch.
		 * @param other Batch to add.
		 */
		void operator+(const BatchOf_Vec3f &other) {
			x = (x + other.x);
			y = (y + other.y);
			z = (z + other.z);
		}

		/**
		 * Adds a constant vector to each element in the batch.
		 * @param v Constant Vec3f to add.
		 */
		void operator+(const Vec3f &v) {
			x = (x + BatchOf_float{simd::set1_ps(v.x)});
			y = (y + BatchOf_float{simd::set1_ps(v.y)});
			z = (z + BatchOf_float{simd::set1_ps(v.z)});
		}

		/**
		 * Subtracts another Vec3fBatch4 from this batch.
		 * @param other Batch to subtract.
		 * @return Reference to self.
		 */
		BatchOf_Vec3f &operator-=(const BatchOf_Vec3f &other) {
			x = (x - other.x);
			y = (y - other.y);
			z = (z - other.z);
			return *this;
		}

		/**
		 * Subtracts a constant vector from each element in the batch.
		 * @param v Constant Vec3f to subtract.
		 * @return Reference to self.
		 */
		BatchOf_Vec3f &operator-=(const Vec3f &v) {
			x = (x - BatchOf_float{simd::set1_ps(v.x)});
			y = (y - BatchOf_float{simd::set1_ps(v.y)});
			z = (z - BatchOf_float{simd::set1_ps(v.z)});
			return *this;
		}

		/**
		 * Multiplies this batch by another Vec3fBatch4.
		 * @param other Batch to multiply by.
		 * @return New Vec3fBatch4 result.
		 */
		BatchOf_Vec3f operator*(const BatchOf_Vec3f &other) const {
			return BatchOf_Vec3f{
				(x * other.x),
				(y * other.y),
				(z * other.z)};
		}

		/**
		 * Multiplies this batch by a BatchOf_float.
		 * @param other Batch to multiply by.
		 * @return New BatchOf_Vec3f result.
		 */
		BatchOf_Vec3f operator*(const BatchOf_float &other) const {
			return BatchOf_Vec3f{
				(x * other),
				(y * other),
				(z * other)};
		}

		/**
		 * Multiplies each component of this batch by a constant BatchOf_float.
		 * @param other BatchOf_float to multiply by.
		 * @return Reference to self.
		 */
		void operator*=(const BatchOf_float &other) {
			x = (x * other);
			y = (y * other);
			z = (z * other);
		}


		/**
		 * Returns the result of subtracting another batch from this batch.
		 * @param other Batch to subtract.
		 * @return New Vec3fBatch4 result.
		 */
		BatchOf_Vec3f operator-(const BatchOf_Vec3f &other) const {
			return BatchOf_Vec3f{
				BatchOf_float{x - other.x},
				BatchOf_float{y - other.y},
				BatchOf_float{z - other.z}};
		}

		/**
		 * Returns the result of dividing this batch by a constant BatchOf_float.
		 * @param other BatchOf_float to divide by.
		 * @return New Vec3fBatch result.
		 */
		BatchOf_Vec3f operator/(const BatchOf_float &other) const {
			return BatchOf_Vec3f{
				BatchOf_float{x / other},
				BatchOf_float{y / other},
				BatchOf_float{z / other}};
		}

		/**
		 * Divides each component of this batch by a constant BatchOf_float.
		 * @param other BatchOf_float to divide by.
		 * @return Reference to self.
		 */
		void operator/=(const BatchOf_float &other) {
			x = (x / other);
			y = (y / other);
			z = (z / other);
		}

		/**
		 * Computes the length squared of each vector in the batch.
		 * @return __m128 containing the length squared for each vector.
		 */
		BatchOf_float lengthSquared() const {
			return x*x + y*y + z*z;
		}

		/**
		 * Clamps each component of the batch to a maximum value.
		 * @param maxValue The maximum value to clamp each component to.
		 * @return New Vec3fBatch with clamped values.
		 */
		BatchOf_Vec3f max(const BatchOf_float &maxValue) const {
			return BatchOf_Vec3f{
				BatchOf_float::max(x, maxValue),
				BatchOf_float::max(y, maxValue),
				BatchOf_float::max(z, maxValue)};
		}

		/**
		 * Computes the horizontal sum of each component in the batch.
		 * @return Vec3f containing the horizontal sums of x, y, and z components.
		 */
		Vec3f hsum() const {
			return {
				simd::hsum_ps(x.c),
				simd::hsum_ps(y.c),
				simd::hsum_ps(z.c) };
		}

		/**
		 * Truncates each component of the batch to an integer value.
		 * @return New Vec3iBatch with truncated integer values.
		 */
		BatchOf_Vec3i floor() const {
			// Convert to integer by truncating the decimal part
			simd::Register_i ix = simd::cvttps_epi32(x.c);
			simd::Register_i iy = simd::cvttps_epi32(y.c);
			simd::Register_i iz = simd::cvttps_epi32(z.c);
			return BatchOf_Vec3i{
				BatchOf_int32{ix},
				BatchOf_int32{iy},
				BatchOf_int32{iz} };
		}
	};

	/**
	 * SIMD batch of quaternion float values.
	 */
	class BatchOf_Quaternion {
	public:
		BatchOf_float w, x, y, z;

		/**
		 * Multiplies this batch of quaternions by another batch of quaternions.
		 * @param b The other batch of quaternions to multiply with.
		 * @return A new BatchOf_Quaternion representing the product.
		 */
		BatchOf_Quaternion operator*(const BatchOf_Quaternion &b) const {
			return BatchOf_Quaternion{
				((w * b.w) - (x * b.x)) - ((y * b.z) + (z * b.y)),
				((w * b.x) + (x * b.w)) + ((y * b.y) - (z * b.z)),
				((w * b.y) + (y * b.w)) + ((z * b.x) + (x * b.z)),
				((w * b.z) + (z * b.w)) + ((x * b.y) - (y * b.x))};
		}
	};

	/**
	 * Aligned vector type for SIMD operations.
	 * Uses AlignedAllocator to ensure proper alignment for SIMD registers.
	 */
	template<typename T> using vectorSIMD = std::vector<T, AlignedAllocator<T, 32>>;
}

// NOLINTEND(portability-simd-intrinsics)

#endif /* REGEN_SIMD_H_ */
