/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef REGEN_BEZIER_H
#define REGEN_BEZIER_H

namespace regen::math {
	/**
	 * Bezier curve.
	 * @tparam T the type of the control points.
	 */
	template<typename T>
	struct Bezier {
		T p0, p1, p2, p3;

		/**
		 * Sample the curve at t.
		 * @param t the time [0, 1].
		 * @return the point.
		 */
		T sample(float t) {
			float u = 1.0f - t;
			float tt = t * t;
			float uu = u * u;
			float uuu = uu * u;
			float ttt = tt * t;
			T p = p0 * uuu; // (1-t)^3 * p0
			p += p1 * 3 * uu * t; // 3 * (1-t)^2 * t * p1
			p += p2 * 3 * u * tt; // 3 * (1-t) * t^2 * p2
			p += p3 * ttt; // t^3 * p3
			return p;
		}

		/**
		 * Sample the tangent at t.
		 * @param t the time [0, 1].
		 * @return the tangent.
		 */
		T tangent(float t) {
			float u = 1.0f - t;
			float tt = t * t;
			float uu = u * u;
			T tangent = p0 * (-3 * uu) + p1 * (3 * uu - 6 * u * t) + p2 * (6 * u * t - 3 * tt) + p3 * (3 * tt);
			tangent.normalize();
			return tangent;
		}

		/**
		 * Compute the length of the curve.
		 * @param numSamples the number of samples.
		 */
		float length(int numSamples) {
			float length = 0.0f;
			float step = 1.0f / static_cast<float>(numSamples);
			float pos = 0.0f;
			T prev = sample(0.0f);
			T next;
			for (int i = 1; i <= numSamples; i++) {
				next = sample(pos);
				length += (next - prev).length();
				prev = next;
				pos += step;
			}
			return length;
		}

		/**
		 * Compute the (rough) length of the curve.
		 */
		float length1() {
			return 0.5f * (
					(p3 - p0).length() +
					(p0 - p1).length() +
					(p2 - p1).length() +
					(p3 - p2).length());
		}
	};
}


#endif //REGEN_BEZIER_H
