/*
 * math.h
 *
 *  Created on: 01.04.2013
 *      Author: daniel
 */

#ifndef MATH_H_
#define MATH_H_

#include <math.h>
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
		/**
		 * Check if floating point values are equal.
		 */
		static inline GLboolean isApprox(const GLfloat &a, const GLfloat &b, GLfloat delta = 1e-6) {
			return abs(a - b) <= delta;
		}

		/**
		 * linearly interpolate between two values.
		 */
		template<class T>
		static inline T mix(T x, T y, GLdouble a) { return x * (1.0 - a) + y * a; }

		/**
		 * constrain a value to lie between two further values.
		 */
		static inline GLfloat clamp(GLfloat x, GLfloat min, GLfloat max) {
			if (x > max) return max;
			else if (x < min) return min;
			else return x;
		}

		static inline GLfloat smoothstep(GLfloat edge0, GLfloat edge1, GLfloat x) {
			// Scale, bias and saturate x to 0..1 range
			x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
			// Evaluate polynomial
			return x * x * (3 - 2 * x);
		}

		static inline GLfloat smootherstep(GLfloat edge0, GLfloat edge1, GLfloat x) {
			// Scale, and clamp x to 0..1 range
			x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
			// Evaluate polynomial
			return x * x * x * (x * (x * 6 - 15) + 10);
		}

		/**
		 * linearly interpolate between two values.
		 */
		template<class T>
		static inline T lerp(const T &x, const T &y, GLdouble a) { return x * (1.0 - a) + y * a; }

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
		static inline float random() {
			// Seed for the random number engine
			static std::random_device rd;
			// Mersenne Twister engine
			static std::mt19937 gen(rd());
			// Uniform distribution between 0 and 1
			static std::uniform_real_distribution<GLfloat> dis(0.0, 1.0);
			return dis(gen);
		}
	}
} // namespace

#endif /* MATH_H_ */
