#ifndef REGEN_MATH_H_
#define REGEN_MATH_H_

#include <cmath>
#include <random>

// = 360.0/(2.0*pi)
#define RAD_TO_DEGREE 57.29577951308232
// = 2.0*pi/360.0
#define DEGREE_TO_RAD 0.0174532925199432

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace regen {
	namespace math {
		constexpr float DEG_TO_RAD = M_PI / 180.0;
		constexpr float RAD_TO_DEG = 180.0 / M_PI;

		/**
		 * Check if floating point values are equal.
		 */
		static inline bool isApprox(const float &a, const float &b, float delta = 1e-6) {
			return abs(a - b) <= delta;
		}

		/**
		 * linearly interpolate between two values.
		 */
		template<class T>
		static inline T mix(T x, T y, double a) { return x * (1.0 - a) + y * a; }

		/**
		 * constrain a value to lie between two further values.
		 */
		static inline float clamp(float x, float min, float max) {
			if (x > max) return max;
			else if (x < min) return min;
			else return x;
		}

		static inline float smoothstep(float edge0, float edge1, float x) {
			// Scale, bias and saturate x to 0..1 range
			x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
			// Evaluate polynomial
			return x * x * (3 - 2 * x);
		}

		static inline float smootherstep(float edge0, float edge1, float x) {
			// Scale, and clamp x to 0..1 range
			x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
			// Evaluate polynomial
			return x * x * x * (x * (x * 6 - 15) + 10);
		}

		/**
		 * linearly interpolate between two values.
		 */
		template<class T>
		static inline T lerp(const T &x, const T &y, double a) { return x * (1.0 - a) + y * a; }

		/**
		 * spherical linear interpolation between two vectors.
		 */
		template<class T>
		static inline T slerp(const T &x, const T &y, double a) {
			double dot = x.dot(y);
			dot = clamp(dot, -1.0, 1.0);
			double theta = acos(dot) * a;
			T relative = y - x * dot;
			relative.normalize();
			return x * cos(theta) + relative * sin(theta);
		}

		/**
		 * Produce a random number between 0 and 1.
		 */
		template <typename T>
		static inline T random() {
			// Seed for the random number engine
			static std::random_device rd;
			// Mersenne Twister engine
			static std::mt19937 gen(rd());
			// Uniform distribution between 0 and 1
			static std::uniform_real_distribution<T> dis(0.0, 1.0);
			return dis(gen);
		}

		/**
		 * Produce a random number between min and max.
		 * @tparam T numeric type
		 * @param min minimum value
		 * @param max maximum value
		 * @return a random number between min and max
		 */
		template <typename T>
		static inline T random(const T &min, const T &max) {
			return min + random<T>() * (max - min);
		}

		/**
		 * Produce a random number between 0 and max_int.
		 */
		static inline int randomInt() {
			// Seed for the random number engine
			static std::random_device rd;
			// Mersenne Twister engine
			static std::mt19937 gen(rd());
			// Uniform distribution between 0 and max_int
			static std::uniform_int_distribution<int> dis(0, std::numeric_limits<int>::max());
			return dis(gen);
		}

		/**
		 * Compute the next power of 2 greater than or equal to x.
		 * @param x the input value
		 * @return the next power of 2 greater than or equal to x
		 */
		static inline uint32_t nextPow2(uint32_t x) {
			if (x == 0) return 1;
			--x;
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;
			return ++x;
		}

		/**
		 * Check if x is a power of 2.
		 * @param x the input value
		 * @return true if x is a power of 2, false otherwise
		 */
		static inline bool isPowerOfTwo(uint32_t x) {
			return (x != 0) && ((x & (x - 1)) == 0);
		}

		/**
		 * @tparam T numeric type
		 * @return pi
		 */
		template<typename T> static inline T pi() {
			static T v = static_cast<T>(M_PI);
			return v;
		}

		/**
		 * @tparam T numeric type
		 * @return two times pi
		 */
		template<typename T> static inline T twoPi() {
			static T v = static_cast<T>(2.0 * M_PI);
			return v;
		}

		/**
		 * @tparam T numeric type
		 * @return pi/2
		 */
		template<typename T> static inline T halfPi() {
			static T v = static_cast<T>(M_PI * 0.5);
			return v;
		}
	}
} // namespace

#endif /* MATH_H_ */
