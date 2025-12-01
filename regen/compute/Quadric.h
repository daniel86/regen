#ifndef REGEN_QUADRIC_H
#define REGEN_QUADRIC_H

#include <regen/compute/vector.h>
#include <regen/compute/matrix.h>

namespace regen {
	/**
	 * \brief A quadric surface defined by a symmetric 4x4 matrix.
	 */
	class Quadric {
	public:
		float a[10] = {0}; // 10 unique components of the symmetric 4x4 matrix

		Quadric() = default;

		// Construct from plane ax + by + cz + d = 0
		Quadric(float a_, float b_, float c_, float d_) {
			set(a_, b_, c_, d_);
		}

		/**
		 * Set the quadric coefficients.
		 */
		void set(float a_, float b_, float c_, float d_) {
			float p[4] = {a_, b_, c_, d_};
			int index = 0;
			for (int i = 0; i < 4; ++i)
				for (int j = i; j < 4; ++j)
					a[index++] = p[i] * p[j];
		}

		// Add two quadrics
		Quadric operator+(const Quadric &q) const {
			Quadric result;
			for (int i = 0; i < 10; ++i)
				result.a[i] = a[i] + q.a[i];
			return result;
		}

		Quadric &operator+=(const Quadric &q) {
			for (int i = 0; i < 10; ++i)
				a[i] += q.a[i];
			return *this;
		}

		/**
		 * @return the quadric matrix.
		 */
		Mat3f toMatrix() const {
			return {
				a[0], a[1], a[2],
				a[1], a[4], a[5],
				a[2], a[5], a[7]
			};
		}

		/**
		 * @param v a point in 3D space.
		 * @return the quadric value at the given point.
		 */
		double evaluate(const Vec3f &v) const {
			float x = v.x, y = v.y, z = v.z;
			return a[0] * x * x + 2 * a[1] * x * y + 2 * a[2] * x * z + 2 * a[3] * x +
				   a[4] * y * y + 2 * a[5] * y * z + 2 * a[6] * y +
				   a[7] * z * z + 2 * a[8] * z +
				   a[9];
		}
	};
}

#endif //REGEN_QUADRIC_H
