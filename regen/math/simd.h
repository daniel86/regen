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
	static constexpr int8_t RegisterMask = 0xFF; // 8 bits for AVX
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

	inline __m256i cvttps_epi32(const __m256 &a) { return _mm256_cvttps_epi32(a); }

	inline int movemask_ps(const __m256 &v) { return _mm256_movemask_ps(v); }

#elif REGEN_SIMD_MODE == SSE
	static constexpr int8_t RegisterMask = 0x0F; // 4 bits for SSE
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

	inline float hsum_ps(__m128 v) {
		__m128 shuf = _mm_movehdup_ps(v);  // (v1, v1, v3, v3)
		__m128 sums = _mm_add_ps(v, shuf);
		shuf = _mm_movehl_ps(shuf, sums); // (v2 + v3, v3, -, -)
		sums = _mm_add_ss(sums, shuf);
		return _mm_cvtss_f32(sums);
	}

	inline __m128 cmp_lt(const __m128 &a, const __m128 &b)  { return _mm_cmplt_ps(a, b); }
	inline __m128 cmp_gt(const __m128 &a, const __m128 &b)  { return _mm_cmplt_ps(b, a); }
	inline __m128 cmp_eq(const __m128 &a, const __m128 &b)  { return _mm_cmpeq_ps(a, b); }
	inline __m128 cmp_neq(const __m128 &a, const __m128 &b) {
		__m128 eq = _mm_cmpeq_ps(a, b);
		return _mm_andnot_ps(eq, _mm_castsi128_ps(_mm_set1_epi32(-1)));  // ~eq & all_ones
	}
	inline __m128 cmp_or(const __m128 &a, const __m128 &b) { return _mm_or_ps(a, b); }

	inline __m128i cvttps_epi32(const __m128 &a) { return _mm_cvttps_epi32(a); }

	inline int movemask_ps(const __m128 &v) { return _mm_movemask_ps(v); }

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

	inline int movemask_ps(const float &v) { return (v != 0.0f) ? 1 : 0; }
#endif
}

namespace regen {
	/**
	 * SIMD-accelerated float value using SSE registers.
	 * This is a simple structure that holds a single float value in a SIMD register.
	 */
	struct BatchOf_float {
		simd::Register c;

		BatchOf_float() = default;

		explicit BatchOf_float(float v) {
			c = regen::simd::set1_ps(v);
		}

		explicit BatchOf_float(simd::Register v) : c(v) {}

		/**
		 * Load batch from an array of 4 unaligned floats.
		 * @param src Pointer to array of 4 floats.
		 */
		inline void load_unaligned(const float *src) {
			c = regen::simd::loadu_ps(src);
		}

		/**
		 * Load batch from an array of 4 aligned floats.
		 * @param src Pointer to array of 4 floats.
		 */
		inline void load_aligned(const float *src) {
			c = regen::simd::load_ps(src);
		}

		/**
		 * Load a single float value into the batch.
		 */
		inline void load(float v) {
			c = regen::simd::set1_ps(v);
		}

		BatchOf_float operator+(const BatchOf_float &other) const {
			return BatchOf_float{regen::simd::add_ps(c, other.c)};
		}

		BatchOf_float operator-(const BatchOf_float &other) const {
			return BatchOf_float{regen::simd::sub_ps(c, other.c)};
		}

		BatchOf_float operator/(const BatchOf_float &other) const {
			return BatchOf_float{regen::simd::div_ps(c, other.c)};
		}

		BatchOf_float operator*(const BatchOf_float &other) const {
			return BatchOf_float{regen::simd::mul_ps(c, other.c)};
		}

		void operator+=(const BatchOf_float &other) {
			c = regen::simd::add_ps(c, other.c);
		}

		void operator-=(const BatchOf_float &other) {
			c = regen::simd::sub_ps(c, other.c);
		}

		void operator*=(const BatchOf_float &other) {
			c = regen::simd::mul_ps(c, other.c);
		}

		void operator/=(const BatchOf_float &other) {
			c = regen::simd::div_ps(c, other.c);
		}

		BatchOf_float cmp_lt(const BatchOf_float &other) const {
			return BatchOf_float{regen::simd::cmp_lt(c, other.c)};
		}
	};

	/**
	 * SIMD-accelerated 2D vector using SoA layout and SSE registers.
	 * Useful for performing vector operations on multiple entities (e.g., boids) in parallel.
	 */
	struct BatchOf_Vec2f {
		/** x/y components, each stored as __m128 representing 4 floats */
		regen::simd::Register x, y;

		/** Default constructor. Leaves content uninitialized. */
		BatchOf_Vec2f() = default;

		/**
		 * Constructor that initializes the batch with SIMD registers.
		 */
		BatchOf_Vec2f(regen::simd::Register x_, regen::simd::Register y_)
			: x(x_), y(y_) {}

		/**
		 * Constructor that initializes the batch with a single Vec2f value.
		 * @param v The Vec2f value to initialize the batch with.
		 */
		explicit BatchOf_Vec2f(const Vec2f &v) {
			x = regen::simd::set1_ps(v.x);
			y = regen::simd::set1_ps(v.y);
		}

		/**
		 * Load a single Vec2f value into the batch.
		 */
		inline void load(const Vec2f &v) {
			x = regen::simd::set1_ps(v.x);
			y = regen::simd::set1_ps(v.y);
		}
	};

	/**
	 * SIMD-accelerated integer value using SSE registers.
	 * This is a simple structure that holds a single integer value in a SIMD register.
	 */
	struct BatchOf_int32 {
		simd::Register_i c;

		BatchOf_int32() = default;

		explicit BatchOf_int32(int32_t v) {
			c = regen::simd::set1_epi32(v);
		}
	};

	/**
	 * SIMD-accelerated 3D integer vector using SSE registers.
	 * This is a simple structure that holds three integer values (x, y, z) in SIMD registers.
	 */
	class BatchOf_Vec3i {
	public:
		/** x/y/z components, each stored as __m128i representing 4 integers */
		regen::simd::Register_i x, y, z;

		/** Default constructor. Leaves content uninitialized. */
		BatchOf_Vec3i() = default;

		/**
		 * Constructor that initializes the batch with SIMD registers.
		 */
		BatchOf_Vec3i(regen::simd::Register_i x_, regen::simd::Register_i y_, regen::simd::Register_i z_)
			: x(x_), y(y_), z(z_) {}

		/**
		 * Constructor that initializes the batch with a single Vec3i value.
		 * @param v The Vec3i value to initialize the batch with.
		 */
		explicit BatchOf_Vec3i(const Vec3i &v) {
			x = regen::simd::set1_epi32(v.x);
			y = regen::simd::set1_epi32(v.y);
			z = regen::simd::set1_epi32(v.z);
		}

		/**
		 * Computes the minimum of this batch and another Vec3iBatch.
		 * @param other Another Vec3iBatch to compare with.
		 * @return A new Vec3iBatch containing the minimum values.
		 */
		inline BatchOf_Vec3i min(const BatchOf_Vec3i &other) const {
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
	class BatchOf_Vec3f {
	public:
		/** x/y/z components, each stored as __m128 representing 4 floats */
		regen::simd::Register x, y, z;

		/** Default constructor. Leaves content uninitialized. */
		BatchOf_Vec3f() = default;

		explicit BatchOf_Vec3f(const Vec3f &v) {
			x = regen::simd::set1_ps(v.x);
			y = regen::simd::set1_ps(v.y);
			z = regen::simd::set1_ps(v.z);
		}

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
		inline BatchOf_Vec3f &operator+=(const BatchOf_Vec3f &other) {
			x = regen::simd::add_ps(x, other.x);
			y = regen::simd::add_ps(y, other.y);
			z = regen::simd::add_ps(z, other.z);
			return *this;
		}

		/**
		 * Adds a constant BatchOf_float to each element in the batch.
		 * @param other BatchOf_float to add.
		 * @return Reference to self.
		 */
		inline BatchOf_Vec3f &operator+=(const BatchOf_float &other) {
			x = regen::simd::add_ps(x, other.c);
			y = regen::simd::add_ps(y, other.c);
			z = regen::simd::add_ps(z, other.c);
			return *this;
		}

		/**
		 * Adds a constant vector to each element in the batch.
		 * @param v Constant Vec3f to add.
		 * @return Reference to self.
		 */
		inline BatchOf_Vec3f &operator+=(const Vec3f &v) {
			x = regen::simd::add_ps(x, regen::simd::set1_ps(v.x));
			y = regen::simd::add_ps(y, regen::simd::set1_ps(v.y));
			z = regen::simd::add_ps(z, regen::simd::set1_ps(v.z));
			return *this;
		}

		/**
		 * Adds another Vec3fBatch to this batch.
		 * @param other Batch to add.
		 */
		inline void operator+(const BatchOf_Vec3f &other) {
			x = regen::simd::add_ps(x, other.x);
			y = regen::simd::add_ps(y, other.y);
			z = regen::simd::add_ps(z, other.z);
		}

		/**
		 * Adds a constant vector to each element in the batch.
		 * @param v Constant Vec3f to add.
		 */
		inline void operator+(const Vec3f &v) {
			x = regen::simd::add_ps(x, regen::simd::set1_ps(v.x));
			y = regen::simd::add_ps(y, regen::simd::set1_ps(v.y));
			z = regen::simd::add_ps(z, regen::simd::set1_ps(v.z));
		}

		/**
		 * Subtracts another Vec3fBatch4 from this batch.
		 * @param other Batch to subtract.
		 * @return Reference to self.
		 */
		inline BatchOf_Vec3f &operator-=(const BatchOf_Vec3f &other) {
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
		inline BatchOf_Vec3f &operator-=(const Vec3f &v) {
			x = regen::simd::sub_ps(x, regen::simd::set1_ps(v.x));
			y = regen::simd::sub_ps(y, regen::simd::set1_ps(v.y));
			z = regen::simd::sub_ps(z, regen::simd::set1_ps(v.z));
			return *this;
		}

		/**
		 * Multiplies this batch by another Vec3fBatch4.
		 * @param other Batch to multiply by.
		 * @return New Vec3fBatch4 result.
		 */
		inline BatchOf_Vec3f operator*(const BatchOf_Vec3f &other) const {
			return {
				regen::simd::mul_ps(x, other.x),
				regen::simd::mul_ps(y, other.y),
				regen::simd::mul_ps(z, other.z)};
		}

		/**
		 * Multiplies this batch by a BatchOf_float.
		 * @param other Batch to multiply by.
		 * @return New BatchOf_Vec3f result.
		 */
		inline BatchOf_Vec3f operator*(const BatchOf_float &other) const {
			return {
				regen::simd::mul_ps(x, other.c),
				regen::simd::mul_ps(y, other.c),
				regen::simd::mul_ps(z, other.c)};
		}

		/**
		 * Multiplies each component of this batch by a constant BatchOf_float.
		 * @param other BatchOf_float to multiply by.
		 * @return Reference to self.
		 */
		inline void operator*=(const BatchOf_float &other) {
			x = regen::simd::mul_ps(x, other.c);
			y = regen::simd::mul_ps(y, other.c);
			z = regen::simd::mul_ps(z, other.c);
		}


		/**
		 * Returns the result of subtracting another batch from this batch.
		 * @param other Batch to subtract.
		 * @return New Vec3fBatch4 result.
		 */
		inline BatchOf_Vec3f operator-(const BatchOf_Vec3f &other) const {
			return {
				regen::simd::sub_ps(x, other.x),
				regen::simd::sub_ps(y, other.y),
				regen::simd::sub_ps(z, other.z)};
		}

		/**
		 * Returns the result of dividing this batch by a constant BatchOf_float.
		 * @param other BatchOf_float to divide by.
		 * @return New Vec3fBatch result.
		 */
		inline BatchOf_Vec3f operator/(const BatchOf_float &other) const {
			return {
				regen::simd::div_ps(x, other.c),
				regen::simd::div_ps(y, other.c),
				regen::simd::div_ps(z, other.c)};
		}

		/**
		 * Divides each component of this batch by a constant BatchOf_float.
		 * @param other BatchOf_float to divide by.
		 * @return Reference to self.
		 */
		inline void operator/=(const BatchOf_float &other) {
			x = regen::simd::div_ps(x, other.c);
			y = regen::simd::div_ps(y, other.c);
			z = regen::simd::div_ps(z, other.c);
		}

		/**
		 * Computes the length squared of each vector in the batch.
		 * @return __m128 containing the length squared for each vector.
		 */
		inline BatchOf_float lengthSquared() const {
			// Calculate length squared for each component
			regen::simd::Register x2 = regen::simd::mul_ps(x, x);
			regen::simd::Register y2 = regen::simd::mul_ps(y, y);
			regen::simd::Register z2 = regen::simd::mul_ps(z, z);
			// Sum the squares
			return BatchOf_float{regen::simd::add_ps(regen::simd::add_ps(x2, y2), z2)};
		}

		/**
		 * Clamps each component of the batch to a maximum value.
		 * @param maxValue The maximum value to clamp each component to.
		 * @return New Vec3fBatch with clamped values.
		 */
		inline BatchOf_Vec3f max(const BatchOf_float &maxValue) const {
			return {
				regen::simd::max_ps(x, maxValue.c),
				regen::simd::max_ps(y, maxValue.c),
				regen::simd::max_ps(z, maxValue.c)};
		}

		inline Vec3f hsum() const {
			return {
				regen::simd::hsum_ps(x),
				regen::simd::hsum_ps(y),
				regen::simd::hsum_ps(z) };
		}

		/**
		 * Truncates each component of the batch to an integer value.
		 * @return New Vec3iBatch with truncated integer values.
		 */
		inline BatchOf_Vec3i floor() const {
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
		BatchOf_Vec3f(
				regen::simd::Register x_,
				regen::simd::Register y_,
				regen::simd::Register z_)
				: x(x_), y(y_), z(z_) {}
	};

	/**
	 * SIMD-accelerated quaternion using SSE registers.
	 * This class represents a quaternion with w, x, y, z components stored in SIMD registers.
	 */
	class BatchOf_Quaternion {
	public:
		/** w/x/y/z components, each stored as batch-of representing 4 or 8 floats */
		regen::simd::Register w, x, y, z;

		/** Default constructor. Leaves content uninitialized. */
		BatchOf_Quaternion() = default;

		/**
		 * Constructor that initializes the batch with a single Quaternion value.
		 * @param q The Quaternion value to initialize the batch with.
		 */
		explicit BatchOf_Quaternion(const Quaternion &q) {
			w = regen::simd::set1_ps(q.w);
			x = regen::simd::set1_ps(q.x);
			y = regen::simd::set1_ps(q.y);
			z = regen::simd::set1_ps(q.z);
		}

		/**
		 * Constructor that initializes the batch with SIMD registers.
		 */
		BatchOf_Quaternion(
				regen::simd::Register w_,
				regen::simd::Register x_,
				regen::simd::Register y_,
				regen::simd::Register z_) {
			w = w_;
			x = x_;
			y = y_;
			z = z_;
		}

		/**
		 * Adds another quaternion batch to this batch.
		 * @param b Another quaternion batch to add.
		 * @return New quaternion batch result.
		 */
		inline BatchOf_Quaternion operator*(const BatchOf_Quaternion &b) const {
			return {
				regen::simd::sub_ps(
					regen::simd::sub_ps(
						regen::simd::mul_ps(w, b.w),
						regen::simd::mul_ps(x, b.x)),
					regen::simd::add_ps(
						regen::simd::mul_ps(y, b.z),
						regen::simd::mul_ps(z, b.y))),
				regen::simd::add_ps(
					regen::simd::add_ps(
						regen::simd::mul_ps(w, b.x),
						regen::simd::mul_ps(x, b.w)),
					regen::simd::sub_ps(
						regen::simd::mul_ps(y, b.y),
						regen::simd::mul_ps(z, b.z))),
				regen::simd::add_ps(
					regen::simd::add_ps(
						regen::simd::mul_ps(w, b.y),
						regen::simd::mul_ps(y, b.w)),
					regen::simd::add_ps(
						regen::simd::mul_ps(z, b.x),
						regen::simd::mul_ps(x, b.z))),
				regen::simd::add_ps(
					regen::simd::add_ps(
						regen::simd::mul_ps(w, b.z),
						regen::simd::mul_ps(z, b.w)),
					regen::simd::sub_ps(
						regen::simd::mul_ps(x, b.y),
						regen::simd::mul_ps(y, b.x)))};
		}
	};

	/**
	 * Aligned vector type for SIMD operations.
	 * Uses AlignedAllocator to ensure proper alignment for SIMD registers.
	 */
	template<typename T> using vectorSIMD = std::vector<T, AlignedAllocator<T, 32>>;
}

// NOLINTEND(portability-simd-intrinsics)

#endif /* KNOWROB_SIMD_H_ */
