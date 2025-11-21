#ifndef REGEN_MATRIX_H_
#define REGEN_MATRIX_H_

#include <regen/math/vector.h>

namespace regen {
	/**
	 * \brief A 3x3 matrix.
	 */
	struct Mat3f {
		using BaseType = float;
		static constexpr int NumComponents = 9;

		float x[9]; /**< Matrix coefficients. */

		/**
		 * Construct a 3x3 matrix from a scalar value.
		 * @param v the scalar value.
		 * @return the matrix.
		 */
		static constexpr Mat3f create(float v) {
			return Mat3f{
					v, v, v,
					v, v, v,
					v, v, v};
		}

		/**
		 * Access a single coefficient.
		 * @param i row index.
		 * @param j column index.
		 * @return the coefficient.
		 */
		constexpr float operator()(int i, int j) const {
			return x[i * 3 + j];
		}

		/**
		 * @param v a scalar.
		 * @return this matrix multiplied by scalar.
		 */
		constexpr Mat3f operator*(const float &v) const {
			const Mat3f &a = *this;
			return Mat3f{
					a(0, 0) * v, // i=0, j=0
					a(0, 1) * v, // i=0, j=1
					a(0, 2) * v, // i=0, j=2

					a(1, 0) * v, // i=1, j=0
					a(1, 1) * v, // i=1, j=1
					a(1, 2) * v, // i=1, j=2

					a(2, 0) * v, // i=2, j=0
					a(2, 1) * v, // i=2, j=1
					a(2, 2) * v  // i=2, j=2
			};
		}

		/**
		 * @param b another matrix.
		 * @return this matrix minus other matrix.
		 */
		constexpr Mat3f operator-(const Mat3f &b) const {
			const Mat3f &a = *this;
			return Mat3f{
					a(0, 0) - b(0, 0), // i=0, j=0
					a(0, 1) - b(0, 1), // i=0, j=1
					a(0, 2) - b(0, 2), // i=0, j=2

					a(1, 0) - b(1, 0), // i=1, j=0
					a(1, 1) - b(1, 1), // i=1, j=1
					a(1, 2) - b(1, 2), // i=1, j=2

					a(2, 0) - b(2, 0), // i=2, j=0
					a(2, 1) - b(2, 1), // i=2, j=1
					a(2, 2) - b(2, 2)  // i=2, j=2
			};
		}

		/**
		 * @param b another matrix.
		 * @return this matrix plus other matrix.
		 */
		constexpr Mat3f operator+(const Mat3f &b) const {
			const Mat3f &a = *this;
			return Mat3f{
					a(0, 0) + b(0, 0), // i=0, j=0
					a(0, 1) + b(0, 1), // i=0, j=1
					a(0, 2) + b(0, 2), // i=0, j=2

					a(1, 0) + b(1, 0), // i=1, j=0
					a(1, 1) + b(1, 1), // i=1, j=1
					a(1, 2) + b(1, 2), // i=1, j=2

					a(2, 0) + b(2, 0), // i=2, j=0
					a(2, 1) + b(2, 1), // i=2, j=1
					a(2, 2) + b(2, 2)  // i=2, j=2
			};
		}

		/**
		 * Matrix-Matrix addition.
		 * @param b another matrix.
		 */
		constexpr void operator+=(const Mat3f &b) {
			Mat3f &a = *this;
			a.x[0] += b.x[0];
			a.x[1] += b.x[1];
			a.x[2] += b.x[2];
			a.x[3] += b.x[3];
			a.x[4] += b.x[4];
			a.x[5] += b.x[5];
			a.x[6] += b.x[6];
			a.x[7] += b.x[7];
			a.x[8] += b.x[8];
		}

		/**
		 * Matrix-Matrix subtraction.
		 * @param b another matrix.
		 */
		constexpr void operator-=(const Mat3f &b) {
			Mat3f &a = *this;
			a.x[0] -= b.x[0];
			a.x[1] -= b.x[1];
			a.x[2] -= b.x[2];
			a.x[3] -= b.x[3];
			a.x[4] -= b.x[4];
			a.x[5] -= b.x[5];
			a.x[6] -= b.x[6];
			a.x[7] -= b.x[7];
			a.x[8] -= b.x[8];
		}

		/**
		 * @return the identity matrix.
		 */
		static constexpr const Mat3f &identity() {
			static constexpr Mat3f Identity = Mat3f{
					1.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 1.0f};
			return Identity;
		}

		/**
		 * @return the determinant of this matrix.
		 */
		constexpr float determinant() const {
			return x[0] * (x[4] * x[8] - x[5] * x[7]) -
				   x[1] * (x[3] * x[8] - x[5] * x[6]) +
				   x[2] * (x[3] * x[7] - x[4] * x[6]);
		}

		/**
		 * @return the inverse of this matrix.
		 */
		constexpr bool inverse(Mat3f &inv) const {
			const auto det = determinant();
			if (fabs(det) < 1e-8) {
				return false;
			}
			const float inv_det = 1.0f / det;
			inv.x[0] = (x[4] * x[8] - x[5] * x[7]) * inv_det;
			inv.x[1] = (x[2] * x[7] - x[1] * x[8]) * inv_det;
			inv.x[2] = (x[1] * x[5] - x[2] * x[4]) * inv_det;
			inv.x[3] = (x[5] * x[6] - x[3] * x[8]) * inv_det;
			inv.x[4] = (x[0] * x[8] - x[2] * x[6]) * inv_det;
			inv.x[5] = (x[2] * x[3] - x[0] * x[5]) * inv_det;
			inv.x[6] = (x[3] * x[7] - x[4] * x[6]) * inv_det;
			inv.x[7] = (x[1] * x[6] - x[0] * x[7]) * inv_det;
			inv.x[8] = (x[0] * x[4] - x[1] * x[3]) * inv_det;
			return true;
		}
	};

	/**
	 * \brief A 4x4 matrix.
	 */
	struct Mat4f {
		using BaseType = float;
		static constexpr int NumComponents = 16;

		float x[16]; /**< Matrix coefficients. */

		/**
		 * Construct a 4x4 matrix from a scalar value.
		 * @param v the scalar value.
		 * @return the matrix.
		 */
		static constexpr Mat4f create(float v) {
			return Mat4f{
					v, v, v, v,
					v, v, v, v,
					v, v, v, v,
					v, v, v, v};
		}

		/**
		 * @return the identity matrix.
		 */
		static constexpr const Mat4f &identity() {
			static constexpr Mat4f Identity = Mat4f{
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f};
			return Identity;
		}

		/**
		 * Set the identity matrix.
		 */
		constexpr void setIdentity() {
			x[0] = 1.0f;
			x[1] = 0.0f;
			x[2] = 0.0f;
			x[3] = 0.0f;
			x[4] = 0.0f;
			x[5] = 1.0f;
			x[6] = 0.0f;
			x[7] = 0.0f;
			x[8] = 0.0f;
			x[9] = 0.0f;
			x[10] = 1.0f;
			x[11] = 0.0f;
			x[12] = 0.0f;
			x[13] = 0.0f;
			x[14] = 0.0f;
			x[15] = 1.0f;
		}

		/**
		 * Matrix that maps vectors from [-1,1] to [0,1].
		 * @return the bias matrix.
		 */
		static constexpr const Mat4f &bias() {
			static constexpr Mat4f BiasMatrix{
					0.5f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.5f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.5f, 0.0f,
					0.5f, 0.5f, 0.5f, 1.0f};
			return BiasMatrix;
		}

		/**
		 * Access a single coefficient.
		 * @param i row index.
		 * @param j column index.
		 * @return the coefficient.
		 */
		constexpr float operator()(int i, int j) const {
			return x[i * 4 + j];
		}

		/**
		 * Matrix-Vector multiplication.
		 * @param v the vector.
		 * @return transformed vector.
		 */
		constexpr Vec4f operator*(const Vec4f &v) const {
			return Vec4f{
					v.x * x[0] + v.y * x[1] + v.z * x[2] + v.w * x[3],
					v.x * x[4] + v.y * x[5] + v.z * x[6] + v.w * x[7],
					v.x * x[8] + v.y * x[9] + v.z * x[10] + v.w * x[11],
					v.x * x[12] + v.y * x[13] + v.z * x[14] + v.w * x[15]};
		}

		/**
		 * Transposed Matrix-Vector multiplication.
		 * @param v the vector.
		 * @return transformed vector.
		 */
		constexpr Vec4f operator^(const Vec4f &v) const {
			return Vec4f{
					v.x * x[0] + v.y * x[4] + v.z * x[8] + v.w * x[12],
					v.x * x[1] + v.y * x[5] + v.z * x[9] + v.w * x[13],
					v.x * x[2] + v.y * x[6] + v.z * x[10] + v.w * x[14],
					v.x * x[3] + v.y * x[7] + v.z * x[11] + v.w * x[15]};
		}

		/**
		 * Matrix-Vector multiplication.
		 * @param v the vector.
		 * @return transformed vector.
		 */
		constexpr Vec4f operator*(const Vec3f &v) const {
			return Vec4f{
					v.x * x[0] + v.y * x[1] + v.z * x[2] + x[3],
					v.x * x[4] + v.y * x[5] + v.z * x[6] + x[7],
					v.x * x[8] + v.y * x[9] + v.z * x[10] + x[11],
					v.x * x[12] + v.y * x[13] + v.z * x[14] + x[15]};
		}

		/**
		 * Transposed Matrix-Vector multiplication.
		 * @param v the vector.
		 * @return transformed vector.
		 */
		constexpr Vec4f operator^(const Vec3f &v) const {
			return Vec4f{
					v.x * x[0] + v.y * x[4] + v.z * x[8] + x[12],
					v.x * x[1] + v.y * x[5] + v.z * x[9] + x[13],
					v.x * x[2] + v.y * x[6] + v.z * x[10] + x[14],
					v.x * x[3] + v.y * x[7] + v.z * x[11] + x[15]};
		}

		/**
		 * Matrix-Matrix multiplication.
		 * @param b another matrix.
		 * @return the matrix product.
		 */
		constexpr Mat4f operator*(const Mat4f &b) const {
			//(AB)_ij = sum A_ik*B_kj
			const Mat4f &a = *this;
			return Mat4f{
					a.x[0] * b.x[0] + a.x[1] * b.x[4] + a.x[2] * b.x[8] + a.x[3] * b.x[12], // i=0, j=0
					a.x[0] * b.x[1] + a.x[1] * b.x[5] + a.x[2] * b.x[9] + a.x[3] * b.x[13], // i=0, j=1
					a.x[0] * b.x[2] + a.x[1] * b.x[6] + a.x[2] * b.x[10] + a.x[3] * b.x[14], // i=0, j=2
					a.x[0] * b.x[3] + a.x[1] * b.x[7] + a.x[2] * b.x[11] + a.x[3] * b.x[15], // i=0, j=3

					a.x[4] * b.x[0] + a.x[5] * b.x[4] + a.x[6] * b.x[8] + a.x[7] * b.x[12], // i=1, j=0
					a.x[4] * b.x[1] + a.x[5] * b.x[5] + a.x[6] * b.x[9] + a.x[7] * b.x[13], // i=1, j=1
					a.x[4] * b.x[2] + a.x[5] * b.x[6] + a.x[6] * b.x[10] + a.x[7] * b.x[14], // i=1, j=2
					a.x[4] * b.x[3] + a.x[5] * b.x[7] + a.x[6] * b.x[11] + a.x[7] * b.x[15], // i=1, j=3

					a.x[8] * b.x[0] + a.x[9] * b.x[4] + a.x[10] * b.x[8] + a.x[11] * b.x[12], // i=2, j=0
					a.x[8] * b.x[1] + a.x[9] * b.x[5] + a.x[10] * b.x[9] + a.x[11] * b.x[13], // i=2, j=1
					a.x[8] * b.x[2] + a.x[9] * b.x[6] + a.x[10] * b.x[10] + a.x[11] * b.x[14], // i=2, j=2
					a.x[8] * b.x[3] + a.x[9] * b.x[7] + a.x[10] * b.x[11] + a.x[11] * b.x[15], // i=2, j=3

					a.x[12] * b.x[0] + a.x[13] * b.x[4] + a.x[14] * b.x[8] + a.x[15] * b.x[12], // i=3, j=0
					a.x[12] * b.x[1] + a.x[13] * b.x[5] + a.x[14] * b.x[9] + a.x[15] * b.x[13], // i=3, j=1
					a.x[12] * b.x[2] + a.x[13] * b.x[6] + a.x[14] * b.x[10] + a.x[15] * b.x[14], // i=3, j=2
					a.x[12] * b.x[3] + a.x[13] * b.x[7] + a.x[14] * b.x[11] + a.x[15] * b.x[15]  // i=3, j=3
			};
		}

		/**
		 * Matrix-Matrix multiplication.
		 * @param b another matrix.
		 */
		constexpr void operator*=(const Mat4f &b) {
			multiplyl(b);
		}

		/**
		 * Matrix-Matrix multiplication. this is left side.
		 * @param b another matrix.
		 */
		constexpr void multiplyl(const Mat4f &b) {
			Vec4f _x0;
			//(AB)_ij = sum A_ik*B_kj
			_x0.x = x[0];
			_x0.y = x[1];
			_x0.z = x[2];
			_x0.w = x[3];
			x[0] = _x0.x * b.x[0] + _x0.y * b.x[4] + _x0.z * b.x[8] + _x0.w * b.x[12];
			x[1] = _x0.x * b.x[1] + _x0.y * b.x[5] + _x0.z * b.x[9] + _x0.w * b.x[13];
			x[2] = _x0.x * b.x[2] + _x0.y * b.x[6] + _x0.z * b.x[10] + _x0.w * b.x[14];
			x[3] = _x0.x * b.x[3] + _x0.y * b.x[7] + _x0.z * b.x[11] + _x0.w * b.x[15];

			_x0.x = x[4];
			_x0.y = x[5];
			_x0.z = x[6];
			_x0.w = x[7];
			x[4] = _x0.x * b.x[0] + _x0.y * b.x[4] + _x0.z * b.x[8] + _x0.w * b.x[12];
			x[5] = _x0.x * b.x[1] + _x0.y * b.x[5] + _x0.z * b.x[9] + _x0.w * b.x[13];
			x[6] = _x0.x * b.x[2] + _x0.y * b.x[6] + _x0.z * b.x[10] + _x0.w * b.x[14];
			x[7] = _x0.x * b.x[3] + _x0.y * b.x[7] + _x0.z * b.x[11] + _x0.w * b.x[15];

			_x0.x = x[8];
			_x0.y = x[9];
			_x0.z = x[10];
			_x0.w = x[11];
			x[8] = _x0.x * b.x[0] + _x0.y * b.x[4] + _x0.z * b.x[8] + _x0.w * b.x[12];
			x[9] = _x0.x * b.x[1] + _x0.y * b.x[5] + _x0.z * b.x[9] + _x0.w * b.x[13];
			x[10] = _x0.x * b.x[2] + _x0.y * b.x[6] + _x0.z * b.x[10] + _x0.w * b.x[14];
			x[11] = _x0.x * b.x[3] + _x0.y * b.x[7] + _x0.z * b.x[11] + _x0.w * b.x[15];

			_x0.x = x[12];
			_x0.y = x[13];
			_x0.z = x[14];
			_x0.w = x[15];
			x[12] = _x0.x * b.x[0] + _x0.y * b.x[4] + _x0.z * b.x[8] + _x0.w * b.x[12];
			x[13] = _x0.x * b.x[1] + _x0.y * b.x[5] + _x0.z * b.x[9] + _x0.w * b.x[13];
			x[14] = _x0.x * b.x[2] + _x0.y * b.x[6] + _x0.z * b.x[10] + _x0.w * b.x[14];
			x[15] = _x0.x * b.x[3] + _x0.y * b.x[7] + _x0.z * b.x[11] + _x0.w * b.x[15];
		}

		/**
		 * Matrix-Matrix multiplication. this is right side.
		 * @param b another matrix.
		 */
		constexpr void multiplyr(const Mat4f &b) {
			Vec4f _x0;
			//(AB)_ij = sum A_ik*B_kj
			_x0.x = x[0];
			_x0.y = x[4];
			_x0.z = x[8];
			_x0.w = x[12];
			x[0] = b.x[0] * _x0.x + b.x[1] * _x0.y + b.x[2] * _x0.z + b.x[3] * _x0.w;
			x[4] = b.x[4] * _x0.x + b.x[5] * _x0.y + b.x[6] * _x0.z + b.x[7] * _x0.w;
			x[8] = b.x[8] * _x0.x + b.x[9] * _x0.y + b.x[10] * _x0.z + b.x[11] * _x0.w;
			x[12] = b.x[12] * _x0.x + b.x[13] * _x0.y + b.x[14] * _x0.z + b.x[15] * _x0.w;

			_x0.x = x[1];
			_x0.y = x[5];
			_x0.z = x[9];
			_x0.w = x[13];
			x[1] = b.x[0] * _x0.x + b.x[1] * _x0.y + b.x[2] * _x0.z + b.x[3] * _x0.w;
			x[5] = b.x[4] * _x0.x + b.x[5] * _x0.y + b.x[6] * _x0.z + b.x[7] * _x0.w;
			x[9] = b.x[8] * _x0.x + b.x[9] * _x0.y + b.x[10] * _x0.z + b.x[11] * _x0.w;
			x[13] = b.x[12] * _x0.x + b.x[13] * _x0.y + b.x[14] * _x0.z + b.x[15] * _x0.w;

			_x0.x = x[2];
			_x0.y = x[6];
			_x0.z = x[10];
			_x0.w = x[14];
			x[2] = b.x[0] * _x0.x + b.x[1] * _x0.y + b.x[2] * _x0.z + b.x[3] * _x0.w;
			x[6] = b.x[4] * _x0.x + b.x[5] * _x0.y + b.x[6] * _x0.z + b.x[7] * _x0.w;
			x[10] = b.x[8] * _x0.x + b.x[9] * _x0.y + b.x[10] * _x0.z + b.x[11] * _x0.w;
			x[14] = b.x[12] * _x0.x + b.x[13] * _x0.y + b.x[14] * _x0.z + b.x[15] * _x0.w;

			_x0.x = x[3];
			_x0.y = x[7];
			_x0.z = x[11];
			_x0.w = x[15];
			x[3] = b.x[0] * _x0.x + b.x[1] * _x0.y + b.x[2] * _x0.z + b.x[3] * _x0.w;
			x[7] = b.x[4] * _x0.x + b.x[5] * _x0.y + b.x[6] * _x0.z + b.x[7] * _x0.w;
			x[11] = b.x[8] * _x0.x + b.x[9] * _x0.y + b.x[10] * _x0.z + b.x[11] * _x0.w;
			x[15] = b.x[12] * _x0.x + b.x[13] * _x0.y + b.x[14] * _x0.z + b.x[15] * _x0.w;
		}

		/**
		 * Matrix-Matrix addition.
		 * @param b another matrix.
		 */
		constexpr void operator+=(const Mat4f &b) {
			Mat4f &a = *this;
			a.x[0] += b.x[0];
			a.x[1] += b.x[1];
			a.x[2] += b.x[2];
			a.x[3] += b.x[3];
			a.x[4] += b.x[4];
			a.x[5] += b.x[5];
			a.x[6] += b.x[6];
			a.x[7] += b.x[7];
			a.x[8] += b.x[8];
			a.x[9] += b.x[9];
			a.x[10] += b.x[10];
			a.x[11] += b.x[11];
			a.x[12] += b.x[12];
			a.x[13] += b.x[13];
			a.x[14] += b.x[14];
			a.x[15] += b.x[15];
		}

		/**
		 * Matrix-Matrix subtraction.
		 * @param b another matrix.
		 */
		constexpr void operator-=(const Mat4f &b) {
			Mat4f &a = *this;
			a.x[0] -= b.x[0];
			a.x[1] -= b.x[1];
			a.x[2] -= b.x[2];
			a.x[3] -= b.x[3];
			a.x[4] -= b.x[4];
			a.x[5] -= b.x[5];
			a.x[6] -= b.x[6];
			a.x[7] -= b.x[7];
			a.x[8] -= b.x[8];
			a.x[9] -= b.x[9];
			a.x[10] -= b.x[10];
			a.x[11] -= b.x[11];
			a.x[12] -= b.x[12];
			a.x[13] -= b.x[13];
			a.x[14] -= b.x[14];
			a.x[15] -= b.x[15];
		}

		/**
		 * @param v a scalar.
		 * @return this matrix multiplied by scalar.
		 */
		constexpr Mat4f operator*(const float &v) const {
			const Mat4f &a = *this;
			return Mat4f{
					a(0, 0) * v, // i=0, j=0
					a(0, 1) * v, // i=0, j=1
					a(0, 2) * v, // i=0, j=2
					a(0, 3) * v, // i=0, j=3

					a(1, 0) * v, // i=1, j=0
					a(1, 1) * v, // i=1, j=1
					a(1, 2) * v, // i=1, j=2
					a(1, 3) * v, // i=1, j=3

					a(2, 0) * v, // i=2, j=0
					a(2, 1) * v, // i=2, j=1
					a(2, 2) * v, // i=2, j=2
					a(2, 3) * v, // i=2, j=3

					a(3, 0) * v, // i=3, j=0
					a(3, 1) * v, // i=3, j=1
					a(3, 2) * v, // i=3, j=2
					a(3, 3) * v  // i=3, j=3
			};
		}

		/**
		 * @param b another matrix.
		 * @return this matrix minus other matrix.
		 */
		constexpr Mat4f operator-(const Mat4f &b) const {
			const Mat4f &a = *this;
			return Mat4f{
					a(0, 0) - b(0, 0), // i=0, j=0
					a(0, 1) - b(0, 1), // i=0, j=1
					a(0, 2) - b(0, 2), // i=0, j=2
					a(0, 3) - b(0, 3), // i=0, j=3

					a(1, 0) - b(1, 0), // i=1, j=0
					a(1, 1) - b(1, 1), // i=1, j=1
					a(1, 2) - b(1, 2), // i=1, j=2
					a(1, 3) - b(1, 3), // i=1, j=3

					a(2, 0) - b(2, 0), // i=2, j=0
					a(2, 1) - b(2, 1), // i=2, j=1
					a(2, 2) - b(2, 2), // i=2, j=2
					a(2, 3) - b(2, 3), // i=2, j=3

					a(3, 0) - b(3, 0), // i=3, j=0
					a(3, 1) - b(3, 1), // i=3, j=1
					a(3, 2) - b(3, 2), // i=3, j=2
					a(3, 3) - b(3, 3)  // i=3, j=3
			};
		}

		/**
		 * @param b another matrix.
		 * @return this matrix plus other matrix.
		 */
		constexpr Mat4f operator+(const Mat4f &b) const {
			const Mat4f &a = *this;
			return Mat4f{
					a(0, 0) + b(0, 0), // i=0, j=0
					a(0, 1) + b(0, 1), // i=0, j=1
					a(0, 2) + b(0, 2), // i=0, j=2
					a(0, 3) + b(0, 3), // i=0, j=3

					a(1, 0) + b(1, 0), // i=1, j=0
					a(1, 1) + b(1, 1), // i=1, j=1
					a(1, 2) + b(1, 2), // i=1, j=2
					a(1, 3) + b(1, 3), // i=1, j=3

					a(2, 0) + b(2, 0), // i=2, j=0
					a(2, 1) + b(2, 1), // i=2, j=1
					a(2, 2) + b(2, 2), // i=2, j=2
					a(2, 3) + b(2, 3), // i=2, j=3

					a(3, 0) + b(3, 0), // i=3, j=0
					a(3, 1) + b(3, 1), // i=3, j=1
					a(3, 2) + b(3, 2), // i=3, j=2
					a(3, 3) + b(3, 3)  // i=3, j=3
			};
		}

		/**
		 * @return the matrix determinant.
		 * @see http://en.wikipedia.org/wiki/Determinant
		 */
		constexpr float determinant() const {
			return
					x[0] * x[5] * x[10] * x[15] -
					x[0] * x[5] * x[11] * x[14] +
					x[0] * x[6] * x[11] * x[13] -
					x[0] * x[6] * x[9] * x[15] +
					//
					x[0] * x[7] * x[9] * x[14] -
					x[0] * x[7] * x[10] * x[13] -
					x[1] * x[6] * x[11] * x[12] +
					x[1] * x[6] * x[8] * x[15] -
					//
					x[1] * x[7] * x[8] * x[14] +
					x[1] * x[7] * x[10] * x[12] -
					x[1] * x[4] * x[10] * x[15] +
					x[1] * x[4] * x[11] * x[14] +
					//
					x[2] * x[7] * x[8] * x[13] -
					x[2] * x[7] * x[9] * x[12] +
					x[2] * x[4] * x[9] * x[15] -
					x[2] * x[4] * x[11] * x[13] +
					//
					x[2] * x[5] * x[11] * x[12] -
					x[2] * x[5] * x[8] * x[15] -
					x[3] * x[4] * x[9] * x[14] +
					x[3] * x[4] * x[10] * x[13] -
					//
					x[3] * x[5] * x[10] * x[12] +
					x[3] * x[5] * x[8] * x[14] -
					x[3] * x[6] * x[8] * x[13] +
					x[3] * x[6] * x[9] * x[12];
		}

		/**
		 * Slow but generic inverse computation using the determinant.
		 * @return the inverse matrix.
		 */
		constexpr Mat4f inverse() const {
			// Compute the reciprocal determinant
			const float det = determinant();
			if (det == 0.0f) {
				// Matrix not invertible.
				return identity();
			}

			const float inv_det = 1.0f / det;
			const float x_10_15_11_14 = x[10] * x[15] - x[11] * x[14];
			const float x_11_12_8_15  = x[11] * x[12] - x[8] * x[15];
			const float x_8_13_9_12   = x[8] * x[13] - x[9] * x[12];
			const float x_9_14_10_13  = x[9] * x[14] - x[10] * x[13];
			const float x_8_14  = x[8] * x[14];
			const float x_10_12 = x[10] * x[12];
			const float x_11_13 = x[11] * x[13];
			const float x_9_15  = x[9] * x[15];

			Mat4f res;
			res.x[0] = inv_det * (
					x[5] * x_10_15_11_14 +
					x[6] * (x_11_13 - x_9_15) +
					x[7] * x_9_14_10_13);

			res.x[1] = -inv_det * (
					x[1] * x_10_15_11_14 +
					x[2] * (x_11_13 - x_9_15) +
					x[3] * x_9_14_10_13);

			res.x[2] = inv_det * (
					x[1] * (x[6] * x[15] - x[7] * x[14]) +
					x[2] * (x[7] * x[13] - x[5] * x[15]) +
					x[3] * (x[5] * x[14] - x[6] * x[13]));

			res.x[3] = -inv_det * (
					x[1] * (x[6] * x[11] - x[7] * x[10]) +
					x[2] * (x[7] * x[9] - x[5] * x[11]) +
					x[3] * (x[5] * x[10] - x[6] * x[9]));

			res.x[4] = -inv_det * (
					x[4] * x_10_15_11_14 +
					x[6] * x_11_12_8_15 +
					x[7] * (x_8_14 - x_10_12));

			res.x[5] = inv_det * (
					x[0] * x_10_15_11_14 +
					x[2] * x_11_12_8_15 +
					x[3] * (x_8_14 - x_10_12));

			res.x[6] = -inv_det * (
					x[0] * (x[6] * x[15] - x[7] * x[14]) +
					x[2] * (x[7] * x[12] - x[4] * x[15]) +
					x[3] * (x[4] * x[14] - x[6] * x[12]));

			res.x[7] = inv_det * (
					x[0] * (x[6] * x[11] - x[7] * x[10]) +
					x[2] * (x[7] * x[8] - x[4] * x[11]) +
					x[3] * (x[4] * x[10] - x[6] * x[8]));

			res.x[8] = inv_det * (
					x[4] * (x_9_15 - x_11_13) +
					x[5] * x_11_12_8_15 +
					x[7] * x_8_13_9_12);

			res.x[9] = -inv_det * (
					x[0] * (x_9_15 - x_11_13) +
					x[1] * x_11_12_8_15 +
					x[3] * x_8_13_9_12);

			res.x[10] = inv_det * (
					x[0] * (x[5] * x[15] - x[7] * x[13]) +
					x[1] * (x[7] * x[12] - x[4] * x[15]) +
					x[3] * (x[4] * x[13] - x[5] * x[12]));

			res.x[11] = -inv_det * (
					x[0] * (x[5] * x[11] - x[7] * x[9]) +
					x[1] * (x[7] * x[8] - x[4] * x[11]) +
					x[3] * (x[4] * x[9] - x[5] * x[8]));

			res.x[12] = -inv_det * (
					x[4] * x_9_14_10_13 +
					x[5] * (x_10_12 - x_8_14) +
					x[6] * x_8_13_9_12);

			res.x[13] = inv_det * (
					x[0] * x_9_14_10_13 +
					x[1] * (x_10_12 - x_8_14) +
					x[2] * x_8_13_9_12);

			res.x[14] = -inv_det * (
					x[0] * (x[5] * x[14] - x[6] * x[13]) +
					x[1] * (x[6] * x[12] - x[4] * x[14]) +
					x[2] * (x[4] * x[13] - x[5] * x[12]));

			res.x[15] = inv_det * (
					x[0] * (x[5] * x[10] - x[6] * x[9]) +
					x[1] * (x[6] * x[8] - x[4] * x[10]) +
					x[2] * (x[4] * x[9] - x[5] * x[8]));

			return res;
		}

		/**
		 * @return the transpose matrix.
		 * @see http://en.wikipedia.org/wiki/Transpose
		 */
		constexpr Mat4f transpose() const {
			Mat4f ret;
			for (uint32_t i = 0; i < 4; ++i) {
				for (uint32_t j = 0; j < 4; ++j) {
					ret.x[j * 4 + i] = x[i * 4 + j];
				}
			}
			return ret;
		}

		////////////////////////////
		////////////////////////////

		/**
		 * @param v input vector.
		 * @return vector multiplied with matrix, ignoring the translation.
		 */
		constexpr Vec3f rotateVector(const Vec3f &v) const {
			return ((*this) * Vec4f::create(v, 0.0f)).xyz();
		}

		/**
		 * @param v input vector.
		 * @return vector multiplied with matrix.
		 */
		constexpr Vec3f transformVector(const Vec3f &v) const {
			return ((*this) * Vec4f::create(v, 1.0f)).xyz();
		}

		/**
		 * @param v input vector.
		 * @return vector multiplied with matrix.
		 */
		constexpr Vec4f transformVector(const Vec4f &v) const {
			return (*this) * v;
		}

		///////////////////
		///////////////////

		/**
		 * Add translation component.
		 * @param translation the translation vector.
		 */
		constexpr void translate(const Vec3f &translation) {
			x[12] += translation.x;
			x[13] += translation.y;
			x[14] += translation.z;
		}

		/**
		 * Set translation component.
		 * @param translation the translation vector.
		 */
		constexpr void setPosition(const Vec3f &translation) {
			x[12] = translation.x;
			x[13] = translation.y;
			x[14] = translation.z;
		}

		/**
		 * @return the translation vector.
		 */
		constexpr const Vec3f &position() const {
			return *((Vec3f *) &x[12]);
		}

		/**
		 * Computes a translation matrix.
		 * @param v translation value.
		 * @return the translation matrix.
		 */
		static constexpr Mat4f translationMatrix(const Vec3f &v) {
			return Mat4f{
					1.0, 0.0, 0.0, 0.0,
					0.0, 1.0, 0.0, 0.0,
					0.0, 0.0, 1.0, 0.0,
					v.x, v.y, v.z, 1.0};
		}

		/**
		 * Computes a translation matrix.
		 * @param v translation value.
		 * @return the translation matrix.
		 */
		static constexpr Mat4f translationMatrix_transposed(const Vec3f &v) {
			return Mat4f{
					1.0, 0.0, 0.0, v.x,
					0.0, 1.0, 0.0, v.y,
					0.0, 0.0, 1.0, v.z,
					0.0, 0.0, 0.0, 1.0};
		}

		/**
		 * Scale this matrix.
		 * @param scale the scale factors.
		 */
		constexpr void scale(const Vec3f &scale) {
			x[0] *= scale.x;
			x[1] *= scale.y;
			x[2] *= scale.z;

			x[4] *= scale.x;
			x[5] *= scale.y;
			x[6] *= scale.z;

			x[8] *= scale.x;
			x[9] *= scale.y;
			x[10] *= scale.z;

			x[12] *= scale.x;
			x[13] *= scale.y;
			x[14] *= scale.z;
		}

		/**
		 * Apply scaling to a vector.
		 * @param v the vector to scale.
		 */
		constexpr void applyScaling(Vec3f &v) const {
			v.x *= x[0];
			v.y *= x[5];
			v.z *= x[10];
		}

		/**
		 * @return the scaling vector.
		 */
		constexpr Vec3f scaling() const {
			return {
					Vec3f(x[0], x[1], x[2]).length(),
					Vec3f(x[4], x[5], x[6]).length(),
					Vec3f(x[8], x[9], x[10]).length()};
		}

		/**
		 * Computes a scaling matrix.
		 * @param v scale factor for each dimension.
		 * @return the scaling matrix.
		 */
		static constexpr Mat4f scaleMatrix(const Vec3f &v) {
			return Mat4f{
					v.x, 0.0, 0.0, 0.0,
					0.0, v.y, 0.0, 0.0,
					0.0, 0.0, v.z, 0.0,
					0.0, 0.0, 0.0, 1.0};
		}

		/**
		 * Computes a rotation matrix along x,y,z Axis.
		 * @param x rotation of x axis.
		 * @param y rotation of y axis.
		 * @param z rotation of z axis.
		 * @return the rotation matrix.
		 */
		static constexpr Mat4f rotationMatrix(float x, float y, float z) {
			const float cx = cosf(x), sx = sinf(x);
			const float cy = cosf(y), sy = sinf(y);
			const float cz = cosf(z), sz = sinf(z);
			const float sxsy = sx * sy;
			const float cxsy = cx * sy;
			return Mat4f{
					cy * cz, sxsy * cz + cx * sz, -cxsy * cz + sx * sz, 0.0f,
					-cy * sz, -sxsy * sz + cx * cz, cxsy * sz + sx * cz, 0.0f,
					sy, -sx * cy, cx * cy, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f};
		}

		/**
		 * Get the rotation of this matrix as euler angles pitch, yaw, roll
		 * in radians.
		 * @return the rotation vector.
		 */
		constexpr Vec3f rotation() const {
			Vec3f euler;
			if (x[8] < 1) {
				if (x[8] > -1) {
					euler.x = -asinf(x[8]);
					euler.z = atan2f(x[9], x[10]);
					euler.y = atan2f(x[4], x[0]);
				} else {
					euler.x = -M_PI / 2;
					euler.z = -atan2f(x[6], x[5]);
					euler.y = 0;
				}
			} else {
				euler.x = M_PI / 2;
				euler.z = atan2f(x[6], x[5]);
				euler.y = 0;
			}
			return euler;
		}

		/**
		 * Computes a transformation matrix with rotation, translation and scaling.
		 * @param rot rotation of x/y/z axis.
		 * @param translation translation vector.
		 * @param scale scale factor for x/y/z components.
		 * @return the transformation matrix.
		 */
		static constexpr Mat4f transformationMatrix(
				const Vec3f &rot, const Vec3f &translation, const Vec3f &scale) {
			const float cx = cosf(rot.x), sx = sinf(rot.x);
			const float cy = cosf(rot.y), sy = sinf(rot.y);
			const float cz = cosf(rot.z), sz = sinf(rot.z);
			const float sxsy = sx * sy;
			const float cxsy = cx * sy;
			return Mat4f{
					-scale.x * cy * cz, -scale.y * (sxsy * cz + cx * sz), scale.z * (cxsy * cz + sx * sz),
					translation.x,
					scale.x * cy * sz, scale.y * (sxsy * sz + cx * cz), -scale.z * (cxsy * sz + sx * cz), translation.y,
					-scale.x * sy, scale.y * sx * cy, -scale.z * cx * cy, translation.z,
					0.0f, 0.0f, 0.0f, 1.0f};
		}

		/**
		 * Computes a transformation matrix with rotation and translation.
		 * @param rot rotation of x/y/z axis.
		 * @param translation translation vector.
		 * @return the transformation matrix.
		 */
		static constexpr Mat4f transformationMatrix(
				const Vec3f &rot, const Vec3f &translation) {
			const float cx = cosf(rot.x), sx = sinf(rot.x);
			const float cy = cosf(rot.y), sy = sinf(rot.y);
			const float cz = cosf(rot.z), sz = sinf(rot.z);
			const float sxsy = sx * sy;
			const float cxsy = cx * sy;
			return Mat4f{
					-cy * cz, -(sxsy * cz + cx * sz), (cxsy * cz + sx * sz), translation.x,
					cy * sz, (sxsy * sz + cx * cz), -(cxsy * sz + sx * cz), translation.y,
					-sy, sx * cy, -cx * cy, translation.z,
					0.0f, 0.0f, 0.0f, 1.0f};
		}

		///////////////////
		///////////////////

		/**
		 * Quick computation of look at matrix inverse.
		 * @return the inverse matrix.
		 */
		constexpr Mat4f lookAtInverse() const {
			return Mat4f{
					x[0], x[4], x[8], 0.0f,
					x[1], x[5], x[9], 0.0f,
					x[2], x[6], x[10], 0.0f,
					-(x[12] * x[0]) - (x[13] * x[1]) - (x[14] * x[2]),
					-(x[12] * x[4]) - (x[13] * x[5]) - (x[14] * x[6]),
					-(x[12] * x[8]) - (x[13] * x[9]) - (x[14] * x[10]),
					1.0f};
		}

		/**
		 * Quick computation of projection matrix inverse.
		 * @return the inverse matrix.
		 */
		constexpr Mat4f projectionInverse() const {
			return Mat4f{
					1.0f / x[0], 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f / x[5], 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 1.0f / x[14],
					0.0f, 0.0f, -1.0f, x[10] / x[14]};
		}

		/**
		 * Computes matrix to increase x/y precision of orthogonal projection.
		 * @param minX lower bound for x component.
		 * @param maxX upper bound for x component.
		 * @param minY lower bound for y component.
		 * @param maxY upper bound for y component.
		 * @return the crop matrix.
		 */
		static constexpr Mat4f cropMatrix(
				float minX, float maxX,
				float minY, float maxY) {
			const float scaleX = 2.0f / (maxX - minX);
			const float scaleY = 2.0f / (maxY - minY);
			return Mat4f{
					scaleX, 0.0f, 0.0f, 0.0f,
					0.0f, scaleY, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					-0.5f * (maxX + minX) * scaleX, -0.5f * (maxY + minY) * scaleY, 0.0f, 1.0f};
		}

		/**
		 * Compute a parallel projection matrix.
		 * @param l specifies the coordinates for the left vertical clipping plane.
		 * @param r specifies the coordinates for the right vertical clipping plane.
		 * @param b specifies the coordinates for the bottom horizontal clipping plane.
		 * @param t specifies the coordinates for the top horizontal clipping plane.
		 * @param n Specify the distances to the nearer depth clipping planes.
		 * @param f Specify the distances to the farther depth clipping planes.
		 * @return the parallel projection matrix.
		 * @note Equivalent to glOrtho.
		 */
		static constexpr Mat4f orthogonalMatrix(
				float l, float r,
				float b, float t,
				float n, float f) {
			const float rl = (r - l);
			const float tb = (t - b);
			const float fn = (f - n);
			return {
					2.0f / rl, 0.0f, 0.0f, 0.0f,
					0.0f, 2.0f / tb, 0.0f, 0.0f,
					0.0f, 0.0f, -2.0f / fn, 0.0f,
					-(r + l) / rl, -(t + b) / tb, -(f + n) / fn, 1.0f
			};
		}

		/**
		 * Compute the inverse of a parallel projection matrix.
		 * @return the inverse matrix.
		 */
		constexpr Mat4f orthogonalInverse() const {
			return {
					1.0f / x[0], 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f / x[5], 0.0f, 0.0f,
					0.0f, 0.0f, -1.0f / x[10], 0.0f,
					0.0f, 0.0f, x[14] / x[10], 1.0f
			};
		}

		/**
		 * Compute a perspective projection matrix.
		 * @param fovDegree specifies the field of view angle, in degrees, in the y direction.
		 * @param aspect specifies the aspect ratio that determines the field of view in the x direction.
		 * @param n specifies the distance from the viewer to the near clipping plane (always positive).
		 * @param f specifies the distance from the viewer to the far clipping plane (always positive).
		 * @return the perspective projection matrix.
		 * @note Equivalent to gluPerspective.
		 */
		static constexpr Mat4f projectionMatrix(
				float fovDegree, float aspect, float n, float f) {
			const float _x = fovDegree * math::DEG_TO_RAD * 0.5f;
			const float ff = cosf(_x) / sinf(_x);
			const float nf = 1.0f / (n - f);
			return Mat4f{
					ff / aspect, 0.0f, 0.0f, 0.0f,
					0.0f, ff, 0.0f, 0.0f,
					0.0f, 0.0f, (f + n) * nf, -1.0f,
					0.0f, 0.0f, 2.0f * f * n * nf, 0.0f};
		}

		/**
		 * Compute a projection matrix.
		 * @param left specifies the coordinates for the left vertical clipping planes.
		 * @param right specifies the coordinates for the right vertical clipping planes.
		 * @param bottom specifies the coordinates for the bottom horizontal clipping planes.
		 * @param top specifies the coordinates for the top horizontal clipping planes.
		 * @param n specifies the distances to the near depth clipping planes.
		 * @param f specifies the distances to the far depth clipping planes.
		 * @return the projection matrix.
		 * @note Equivalent to glFrustum.
		 */
		static constexpr Mat4f frustumMatrix(
				float left, float right,
				float bottom, float top,
				float n, float f) {
			const float rl = (right - left);
			const float tb = (top - bottom);
			const float fn = (f - n);
			return Mat4f{
					(2.0f * n) / rl, 0.0f, 0.0f, 0.0f,
					0.0f, (2.0f * n) / tb, 0.0f, 0.0f,
					(right + left) / rl, (top + bottom) / tb, -(f + n) / fn,
					-1.0f,
					0.0f, 0.0f, -(2.0f * f * n) / fn, 0.0f};
		}

		/**
		 * Compute view transformation matrix.
		 * @param pos specifies the position of the eye point.
		 * @param dir specifies the look at direction.
		 * @param up specifies the direction of the up vector. must be normalized.
		 * @return the view transformation matrix.
		 * @note Equivalent to gluLookAt.
		 */
		static constexpr Mat4f lookAtMatrix(
				const Vec3f &pos, const Vec3f &dir, const Vec3f &up) {
			Vec3f t = -pos;
			Vec3f f = dir;
			f.normalize();
			Vec3f s = f.cross(up);
			s.normalize();
			Vec3f u = s.cross(f);
			return Mat4f{
					s.x, u.x, -f.x, 0.0f,
					s.y, u.y, -f.y, 0.0f,
					s.z, u.z, -f.z, 0.0f,
					s.dot(t), u.dot(t), (-f).dot(t), 1.0f};
		}

		/**
		 * Reflects vectors along an arbitrary plane.
		 * @param p a point on the plane.
		 * @param n plane normal vector.
		 * @return The reflection matrix.
		 */
		static constexpr Mat4f reflectionMatrix(
				const Vec3f &p, const Vec3f &n) {
			const float a = 2.0f * n.dot(p);
			return Mat4f{
					1.0f - 2.0f * n.x * n.x, -2.0f * n.x * n.y, -2.0f * n.x * n.z, a * n.x,
					-2.0f * n.y * n.x, 1.0f - 2.0f * n.y * n.y, -2.0f * n.y * n.z, a * n.y,
					-2.0f * n.z * n.x, -2.0f * n.z * n.y, 1.0f - 2.0f * n.z * n.z, a * n.z,
					0.0f, 0.0f, 0.0f, 1.0f};
		}

		/**
		 * Compute view transformation matrices.
		 * @param pos cube center position.
		 * @return 6 view transformation matrices, one for each cube face.
		 * @note you have to call delete[] when you are done using the returned pointer.
		 */
		static constexpr Mat4f *cubeLookAtMatrices(const Vec3f &pos) {
			auto *views = new Mat4f[6];
			cubeLookAtMatrices(pos, views);
			return views;
		}

		/**
		 * @return Array of cube normal vectors.
		 */
		static constexpr const Vec3f *cubeDirections() {
			static constexpr Vec3f DirVectors[6] = {
					Vec3f(1.0f, 0.0f, 0.0f),
					Vec3f(-1.0f, 0.0f, 0.0f),
					Vec3f(0.0f, 1.0f, 0.0f),
					Vec3f(0.0f, -1.0f, 0.0f),
					Vec3f(0.0f, 0.0f, 1.0f),
					Vec3f(0.0f, 0.0f, -1.0f)};
			return DirVectors;
		}

		/**
		 * @return Array of up vectors in cube map space.
		 */
		static constexpr const Vec3f *cubeUpVectors() {
			static constexpr Vec3f UpVectors[6] = {
					Vec3f(0.0f, -1.0f, 0.0f),
					Vec3f(0.0f, -1.0f, 0.0f),
					Vec3f(0.0f, 0.0f, 1.0f),
					Vec3f(0.0f, 0.0f, -1.0f),
					Vec3f(0.0f, -1.0f, 0.0f),
					Vec3f(0.0f, -1.0f, 0.0f)};
			return UpVectors;
		}

		/**
		 * Compute view transformation matrices.
		 * @param pos cube center position.
		 * @param views matrix array.
		 * @return 6 view transformation matrices, one for each cube face.
		 * @note you have to call delete[] when you are done using the returned pointer.
		 */
		static constexpr void cubeLookAtMatrices(const Vec3f &pos, Mat4f *views) {
			const Vec3f *dir = cubeDirections();
			const Vec3f *up = cubeUpVectors();
			for (uint32_t i = 0; i < 6; ++i) views[i] = Mat4f::lookAtMatrix(pos, dir[i], up[i]);
		}

		/**
		 * Compute view transformation matrices with cube center at origin point (0,0,0).
		 * @return 6 view transformation matrices, one for each cube face.
		 */
		static constexpr const Mat4f *cubeLookAtMatrices() {
			static Mat4f *views = nullptr;
			if (views == nullptr) {
				views = Mat4f::cubeLookAtMatrices(Vec3f::zero());
			}
			return views;
		}
	};

	std::istream &operator>>(std::istream &in, Mat3f &v);

	std::istream &operator>>(std::istream &in, Mat4f &v);

	std::ostream &operator<<(std::ostream &os, const Mat3f &m);

	std::ostream &operator<<(std::ostream &os, const Mat4f &m);

	// Vector traits
	template<> struct VecTraits<Mat3f> { using BaseType = float; };
	template<> struct VecTraits<Mat4f> { using BaseType = float; };
} // namespace

#endif /* _MATRIX_H_ */
