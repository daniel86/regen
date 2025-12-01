#ifndef REGEN_BEZIER_H
#define REGEN_BEZIER_H

#include <vector>

namespace regen::math {
	struct ArcLengthLUT {
		std::vector<float> s; // normalized arc length
		std::vector<float> t; // curve parameter
		float totalLength = 0.0f;
	};

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
		T sample(float t) const {
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
		 * Sample the curve at t using arc-length parameterization.
		 * @param lut the arc-length lookup table.
		 * @param t the time [0, 1].
		 * @return the point.
		 */
		T sample(const ArcLengthLUT &lut, float t) const {
			float tLUT = lookupParameter(lut, t);
			return sample(tLUT);
		}

		/**
		 * Sample the tangent at t.
		 * @param t the time [0, 1].
		 * @return the tangent.
		 */
		T tangent(float t) const {
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

		/**
		 * Approximate arc-length parameterization for a cubic Bézier.
		 * @param samples the number of samples.
		 * @return the lookup table.
		 */
		ArcLengthLUT buildArcLengthLUT(int samples = 100) {
			ArcLengthLUT lut;
			lut.s.resize(samples+1);
			lut.t.resize(samples+1);

			lut.s[0] = 0.0f;
			lut.t[0] = 0.0f;

			lut.totalLength = 0.0f;
			Vec2f prev = sample(0.0f);

			for (int i = 1; i <= samples; i++) {
				float ti = (float)i / samples;
				Vec2f p = sample(ti);
				lut.totalLength += (p - prev).length();
				lut.t[i] = ti;
				lut.s[i] = lut.totalLength;
				prev = p;
			}

			// normalize s to [0,1]
			for (int i = 0; i <= samples; i++) {
				lut.s[i] /= lut.totalLength;
			}

			return lut;
		}

		/**
		 * Lookup the curve parameter t for a given arc length fraction s.
		 * @param lut the arc-length lookup table.
		 * @param sQuery the arc length fraction [0, 1].
		 * @return the curve parameter t [0, 1].
		 */
		static float lookupParameter(const ArcLengthLUT& lut, float sQuery) {
			// Find the segment via binary search
			int low = 0;
			int high = static_cast<int>(lut.s.size()) - 1;
			while (low < high) {
				int mid = (low + high) / 2;
				if (lut.s[mid] < sQuery) {
					low = mid + 1;
				} else {
					high = mid;
				}
			}
			int i = std::max(0, low - 1);
			// Interpolate
			float alpha = (sQuery - lut.s[i]) / (lut.s[i+1] - lut.s[i]);
			return lut.t[i] + alpha * (lut.t[i+1] - lut.t[i]);
		}
	};
}


#endif //REGEN_BEZIER_H
