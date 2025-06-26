#ifndef KNOWROB_SIMD_H_
#define KNOWROB_SIMD_H_

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
#if REGEN_SIMD_MODE == AVX
	using Register = __m256; // 8 floats
	using Register_i = __m256i; // 8 integers

	inline __m256 set1_ps(float v) { return _mm256_set1_ps(v); }
	inline __m256i set1_epi32(int32_t v) { return _mm256_set1_epi32(v); }

	inline __m256 load_ps(const float *p) { return _mm256_load_ps(p); }
	inline __m256 loadu_ps(const float *p) { return _mm256_loadu_ps(p); }

	inline __m256i loadu_si256(const uint32_t *p) {
		return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
	}
	inline __m256i loadu_si256(const int32_t *p) {
		return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
	}

	inline __m256 i32gather_ps(const float *p, const __m256i &indices) {
		return _mm256_i32gather_ps(p, indices, sizeof(float));
	}

	inline void storeu_ps(float *p, const __m256 &v) { _mm256_storeu_ps(p, v); }
	inline void storeu_epi32(int32_t *p, const __m256i &v) {
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v);
	}

	inline __m256 add_ps(const __m256 &a, const __m256 &b) { return _mm256_add_ps(a, b); }
	inline __m256 sub_ps(const __m256 &a, const __m256 &b) { return _mm256_sub_ps(a, b); }
	inline __m256 mul_ps(const __m256 &a, const __m256 &b) { return _mm256_mul_ps(a, b); }
	inline __m256 div_ps(const __m256 &a, const __m256 &b) { return _mm256_div_ps(a, b); }

	inline __m256i add_epi32(const __m256i &a, const __m256i &b) { return _mm256_add_epi32(a, b); }
	inline __m256i sub_epi32(const __m256i &a, const __m256i &b) { return _mm256_sub_epi32(a, b); }
	inline __m256i mul_epi32(const __m256i &a, const __m256i &b) { return _mm256_mullo_epi32(a, b); }

	inline __m256 min_ps(const __m256 &a, const __m256 &b) { return _mm256_min_ps(a, b); }
	inline __m256 max_ps(const __m256 &a, const __m256 &b) { return _mm256_max_ps(a, b); }
	inline __m256 sqrt_ps(const __m256 &a) { return _mm256_sqrt_ps(a); }

	inline __m256i min_epi32(const __m256i &a, const __m256i &b) { return _mm256_min_epi32(a, b); }
	inline __m256i max_epi32(const __m256i &a, const __m256i &b) { return _mm256_max_epi32(a, b); }

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

	inline __m256i cvttps_epi32(const __m256 &a) { return _mm256_cvttps_epi32(a); }

	inline int movemask_ps(const __m256 &v) { return _mm256_movemask_ps(v); }

#elif REGEN_SIMD_MODE == SSE
	using Register = __m128; // 4 floats
	using Register_i = __m128i; // 4 integers

	inline __m128 set1_ps(float v) { return _mm_set1_ps(v); }
	inline __m128i set1_epi32(int32_t v) { return _mm_set1_epi32(v); }

	inline __m128 load_ps(const float *p) { return _mm_load_ps(p); }
	inline __m128 loadu_ps(const float *p) { return _mm_loadu_ps(p); }

	inline __m128i loadu_si256(const uint32_t *p) {
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(indices));
	}

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

	inline __m128 cmp_lt(const __m128 &a, const __m128 &b)  { return _mm_cmplt_ps(a, b); }
	inline __m128 cmp_gt(const __m128 &a, const __m128 &b)  { return _mm_cmplt_ps(b, a); }
	inline __m128 cmp_eq(const __m128 &a, const __m128 &b)  { return _mm_cmpeq_ps(a, b); }
	inline __m128 cmp_neq(const __m128 &a, const __m128 &b) {
		__m128 eq = _mm_cmpeq_ps(a, b);
		return _mm_andnot_ps(eq, _mm_castsi128_ps(_mm_set1_epi32(-1)));  // ~eq & all_ones
	}

	inline __m128i cvttps_epi32(const __m128 &a) { return _mm_cvttps_epi32(a); }

	inline int movemask_ps(const __m128 &v) { return _mm_movemask_ps(v); }

#else // Fallback to scalar operations
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

	inline int movemask_ps(const float &v) { return (v != 0.0f) ? 1 : 0; }
#endif
}

namespace regen {
	/**
	 * SIMD-accelerated 3D vector using SSE registers.
	 * This is a simple structure that holds three float values (x, y, z) in SIMD registers.
	 */
	struct Vec3fSIMD {
		regen::simd::Register x, y, z;

		Vec3fSIMD() = default;

		explicit Vec3fSIMD(const Vec3f &v) {
			x = regen::simd::set1_ps(v.x);
			y = regen::simd::set1_ps(v.y);
			z = regen::simd::set1_ps(v.z);
		}
	};

	/**
	 * SIMD-accelerated 3D integer vector using SSE registers.
	 * This is a simple structure that holds three integer values (x, y, z) in SIMD registers.
	 */
	struct Vec3iSIMD {
		regen::simd::Register_i x, y, z;

		Vec3iSIMD() = default;

		explicit Vec3iSIMD(const Vec3i &v) {
			x = regen::simd::set1_epi32(v.x);
			y = regen::simd::set1_epi32(v.y);
			z = regen::simd::set1_epi32(v.z);
		}
	};

	/**
	 * SIMD-accelerated float value using SSE registers.
	 * This is a simple structure that holds a single float value in a SIMD register.
	 */
	struct floatSIMD {
		simd::Register c;

		floatSIMD() = default;

		explicit floatSIMD(float v) {
			c = regen::simd::set1_ps(v);
		}
	};

	/**
	 * SIMD-accelerated integer value using SSE registers.
	 * This is a simple structure that holds a single integer value in a SIMD register.
	 */
	struct intSIMD {
		simd::Register_i c;

		intSIMD() = default;

		explicit intSIMD(int32_t v) {
			c = regen::simd::set1_epi32(v);
		}
	};

	class Vec3iBatch {
	public:
		/** x/y/z components, each stored as __m128i representing 4 integers */
		regen::simd::Register_i x, y, z;

		/** Default constructor. Leaves content uninitialized. */
		Vec3iBatch() = default;

		/**
		 * Constructor that initializes the batch with SIMD registers.
		 */
		Vec3iBatch(regen::simd::Register_i x_, regen::simd::Register_i y_, regen::simd::Register_i z_)
			: x(x_), y(y_), z(z_) {}

		/**
		 * Computes the minimum of this batch and another Vec3iBatch.
		 * @param other Another Vec3iBatch to compare with.
		 * @return A new Vec3iBatch containing the minimum values.
		 */
		inline Vec3iBatch min(const Vec3iBatch &other) const {
			return {
				regen::simd::min_epi32(x, other.x),
				regen::simd::min_epi32(y, other.y),
				regen::simd::min_epi32(z, other.z)};
		}

		/**
		 * Computes the minimum of this batch and a Vec3iSIMD.
		 * @param other A Vec3iSIMD to compare with.
		 * @return A new Vec3iBatch containing the minimum values.
		 */
		inline Vec3iBatch min(const Vec3iSIMD &other) const {
			return {
				regen::simd::min_epi32(x, other.x),
				regen::simd::min_epi32(y, other.y),
				regen::simd::min_epi32(z, other.z)};
		}
	};

	/**
	 * SIMD-accelerated batch of 3D vectors using SoA layout and SSE registers.
	 * Useful for performing vector operations on multiple entities (e.g., boids) in parallel.
	 */
	class Vec3fBatch {
	public:
		/** x/y/z components, each stored as __m128 representing 4 floats */
		regen::simd::Register x, y, z;

		/** Default constructor. Leaves content uninitialized. */
		Vec3fBatch() = default;

		/**
		 * Load batch from an array of 4 unaligned Vec3f values (AoS layout).
		 * @param src Pointer to array of 4 Vec3f.
		 */
		inline void load_unaligned(const Vec3f *src) {
			float x_[4], y_[4], z_[4];
			for (int i = 0; i < 4; ++i) {
				x_[i] = src[i].x;
				y_[i] = src[i].y;
				z_[i] = src[i].z;
			}
			x = regen::simd::loadu_ps(x_);
			y = regen::simd::loadu_ps(y_);
			z = regen::simd::loadu_ps(z_);
		}

		/**
		 * Load batch from separate arrays of x, y, z components (SoA layout).
		 * @param xs Pointer to 4 floats representing x-components.
		 * @param ys Pointer to 4 floats representing y-components.
		 * @param zs Pointer to 4 floats representing z-components.
		 */
		inline void load_unaligned(const float *xs, const float *ys, const float *zs) {
			x = regen::simd::loadu_ps(xs);
			y = regen::simd::loadu_ps(ys);
			z = regen::simd::loadu_ps(zs);
		}

		/**
		 * Loads a batch of Vec3f from an array of 4 aligned Vec3f values (AoS layout).
		 * This is more efficient than unaligned load if the data is guaranteed to be aligned.
		 * @param src Pointer to array of 4 aligned Vec3f.
		 */
		inline void load_aligned(const float *xs, const float *ys, const float *zs) {
			x = regen::simd::load_ps(xs);
			y = regen::simd::load_ps(ys);
			z = regen::simd::load_ps(zs);
		}

		/**
		 * Loads a batch of Vec3f from separate arrays using indices.
		 * This is useful for gathering data from non-contiguous memory locations.
		 * @param xs Pointer to x-components array.
		 * @param ys Pointer to y-components array.
		 * @param zs Pointer to z-components array.
		 * @param indices SIMD register containing indices to gather.
		 */
		inline void load(const float *xs, const float *ys, const float *zs,
		                 const regen::simd::Register_i &indices) {
			x = regen::simd::i32gather_ps(xs, indices);
			y = regen::simd::i32gather_ps(ys, indices);
			z = regen::simd::i32gather_ps(zs, indices);
		}

		/**
		 * Stores the batch back to an array of 4 Vec3f (AoS layout).
		 * @param dst Pointer to output array of 4 Vec3f.
		 */
		inline void store(Vec3f *dst) const {
			alignas(16) float x_[4], y_[4], z_[4];
			regen::simd::storeu_ps(x_, x);
			regen::simd::storeu_ps(y_, y);
			regen::simd::storeu_ps(z_, z);
			for (int i = 0; i < 4; ++i) {
				dst[i].x = x_[i];
				dst[i].y = y_[i];
				dst[i].z = z_[i];
			}
		}

		/**
		 * Adds another Vec3fBatch4 to this batch.
		 * @param other Batch to add.
		 * @return Reference to self.
		 */
		inline Vec3fBatch &operator+=(const Vec3fBatch &other) {
			x = regen::simd::add_ps(x, other.x);
			y = regen::simd::add_ps(y, other.y);
			z = regen::simd::add_ps(z, other.z);
			return *this;
		}

		/**
		 * Adds a Vec3fSIMD to each vector of this batch.
		 * @param other Vec3fSIMD to add.
		 * @return Reference to self.
		 */
		inline Vec3fBatch &operator+=(const Vec3fSIMD &other) {
			x = regen::simd::add_ps(x, other.x);
			y = regen::simd::add_ps(y, other.y);
			z = regen::simd::add_ps(z, other.z);
			return *this;
		}

		/**
		 * Adds a constant vector to each element in the batch.
		 * @param v Constant Vec3f to add.
		 * @return Reference to self.
		 */
		inline Vec3fBatch &operator+=(const Vec3f &v) {
			x = regen::simd::add_ps(x, regen::simd::set1_ps(v.x));
			y = regen::simd::add_ps(y, regen::simd::set1_ps(v.y));
			z = regen::simd::add_ps(z, regen::simd::set1_ps(v.z));
			return *this;
		}

		/**
		 * Subtracts another Vec3fBatch4 from this batch.
		 * @param other Batch to subtract.
		 * @return Reference to self.
		 */
		inline Vec3fBatch &operator-=(const Vec3fBatch &other) {
			x = regen::simd::sub_ps(x, other.x);
			y = regen::simd::sub_ps(y, other.y);
			z = regen::simd::sub_ps(z, other.z);
			return *this;
		}

		/**
		 * Subtracts a Vec3fSIMD from each vector of this batch.
		 * @param other Vec3fSIMD to subtract.
		 * @return Reference to self.
		 */
		inline Vec3fBatch &operator-=(const Vec3fSIMD &other) {
			x = regen::simd::sub_ps(x, other.x);
			y = regen::simd::sub_ps(y, other.y);
			z = regen::simd::sub_ps(z, other.z);
			return *this;
		}

		/**
		 * Subtracts a constant vector from each element in the batch.
		 * @param v Constant Vec3f to subtract.
		 * @return Reference to self.
		 */
		inline Vec3fBatch &operator-=(const Vec3f &v) {
			x = regen::simd::sub_ps(x, regen::simd::set1_ps(v.x));
			y = regen::simd::sub_ps(y, regen::simd::set1_ps(v.y));
			z = regen::simd::sub_ps(z, regen::simd::set1_ps(v.z));
			return *this;
		}

		/**
		 * Returns the result of subtracting another batch from this batch.
		 * @param other Batch to subtract.
		 * @return New Vec3fBatch4 result.
		 */
		inline Vec3fBatch operator-(const Vec3fBatch &other) const {
			return {
				regen::simd::sub_ps(x, other.x),
				regen::simd::sub_ps(y, other.y),
				regen::simd::sub_ps(z, other.z)};
		}

		/**
		 * Returns the result of subtracting a Vec3fSIMD from this batch.
		 * @param other Vec3fSIMD to subtract.
		 * @return New Vec3fBatch4 result.
		 */
		inline Vec3fBatch operator-(const Vec3fSIMD &other) const {
			return {
				regen::simd::sub_ps(x, other.x),
				regen::simd::sub_ps(y, other.y),
				regen::simd::sub_ps(z, other.z)};
		}

		/**
		 * Returns the result of dividing this batch by another Vec3fBatch.
		 * @param other Batch to divide by.
		 * @return New Vec3fBatch result.
		 */
		inline Vec3fBatch operator/(const Vec3fSIMD &other) const {
			return {
				regen::simd::div_ps(x, other.x),
				regen::simd::div_ps(y, other.y),
				regen::simd::div_ps(z, other.z)};
		}

		/**
		 * Returns the result of dividing this batch by a constant floatSIMD.
		 * @param other floatSIMD to divide by.
		 * @return New Vec3fBatch result.
		 */
		inline Vec3fBatch operator/(const floatSIMD &other) const {
			return {
				regen::simd::div_ps(x, other.c),
				regen::simd::div_ps(y, other.c),
				regen::simd::div_ps(z, other.c)};
		}

		/**
		 * Computes the length squared of each vector in the batch.
		 * @return __m128 containing the length squared for each vector.
		 */
		inline simd::Register lengthSquared() const {
			// Calculate length squared for each component
			regen::simd::Register x2 = regen::simd::mul_ps(x, x);
			regen::simd::Register y2 = regen::simd::mul_ps(y, y);
			regen::simd::Register z2 = regen::simd::mul_ps(z, z);
			// Sum the squares
			return regen::simd::add_ps(regen::simd::add_ps(x2, y2), z2);
		}

		/**
		 * Clamps each component of the batch to a maximum value.
		 * @param maxValue The maximum value to clamp each component to.
		 * @return New Vec3fBatch with clamped values.
		 */
		inline Vec3fBatch max(const floatSIMD &maxValue) const {
			return {
				regen::simd::max_ps(x, maxValue.c),
				regen::simd::max_ps(y, maxValue.c),
				regen::simd::max_ps(z, maxValue.c)};
		}

		/**
		 * Truncates each component of the batch to an integer value.
		 * @return New Vec3iBatch with truncated integer values.
		 */
		inline Vec3iBatch floor() const {
			// Convert to integer by truncating the decimal part
			regen::simd::Register_i ix = regen::simd::cvttps_epi32(x);
			regen::simd::Register_i iy = regen::simd::cvttps_epi32(y);
			regen::simd::Register_i iz = regen::simd::cvttps_epi32(z);
			return { ix, iy, iz };
		}

	private:
		/**
		 * Internal constructor used for operations on __m128 directly.
		 * @param x_ __m128 for x component.
		 * @param y_ __m128 for y component.
		 * @param z_ __m128 for z component.
		 */
		Vec3fBatch(
				regen::simd::Register x_,
				regen::simd::Register y_,
				regen::simd::Register z_)
				: x(x_), y(y_), z(z_) {}
	};

	/**
	 * Aligned vector type for SIMD operations.
	 * Uses AlignedAllocator to ensure proper alignment for SIMD registers.
	 */
	template<typename T> using vectorSIMD = std::vector<T, AlignedAllocator<T, 32>>;
}

// NOLINTEND(portability-simd-intrinsics)

#endif /* KNOWROB_SIMD_H_ */
