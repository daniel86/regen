#ifndef REGEN_QUADRIC_H
#define REGEN_QUADRIC_H

#include <regen/math/vector.h>
#include <regen/math/matrix.h>

namespace regen {
	class Quadric {
	public:
		double a[10] = {0}; // 10 unique components of the symmetric 4x4 matrix

		Quadric() = default;

		// Construct from plane ax + by + cz + d = 0
		Quadric(double a_, double b_, double c_, double d_) {
			set(a_, b_, c_, d_);
		}

		void set(double a_, double b_, double c_, double d_) {
			double p[4] = {a_, b_, c_, d_};
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

		// Evaluate error at a point
		double evaluate(const Vec3f &v) const {
			double x = v.x, y = v.y, z = v.z;
			return a[0] * x * x + 2 * a[1] * x * y + 2 * a[2] * x * z + 2 * a[3] * x +
				   a[4] * y * y + 2 * a[5] * y * z + 2 * a[6] * y +
				   a[7] * z * z + 2 * a[8] * z +
				   a[9];
		}
	};
}

#endif //REGEN_QUADRIC_H
