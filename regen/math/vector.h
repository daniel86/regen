#ifndef REGEN_VECTOR_H_
#define REGEN_VECTOR_H_

#include <GL/glew.h>

#include <iostream>
#include <cmath>
#include <cassert>
#include <list>

#include <regen/utility/string-util.h>
#include <regen/math/math.h>

namespace regen {
	/**
	 * \brief A 2D vector.
	 */
	template<typename T>
	class Vec2 {
	public:
		T x; /**< the x component. **/
		T y; /**< the y component. **/

		Vec2() : x(0), y(0) {}

		/** Set-component constructor. */
		Vec2(T _x, T _y) : x(_x), y(_y) {}

		/** @param _x value that is applied to all components. */
		Vec2(T _x) : x(_x), y(_x) {}

		/** copy constructor. */
		Vec2(const Vec2 &b) : x(b.x), y(b.y) {}

		/** copy operator. */
		inline void operator=(const Vec2 &b) {
			x = b.x;
			y = b.y;
		}

		/**
		 * @param b another vector
		 * @return true if all values are equal
		 */
		inline bool operator==(const Vec2 &b) const { return x == b.x && y == b.y; }

		/**
		 * @param b another vector
		 * @return false if all values are equal
		 */
		inline bool operator!=(const Vec2 &b) const { return !operator==(b); }

		/**
		 * Subscript operator.
		 */
		inline T &operator[](int i) {
			return ((T *) this)[i];
		}

		/**
		 * Subscript operator.
		 */
		inline const T &operator[](int i) const {
			return ((T *) this)[i];
		}

		/**
		 * @return vector with each component negated.
		 */
		inline Vec2 operator-() const { return Vec2(-x, -y); }

		/**
		 * @param b vector to add.
		 * @return the vector sum.
		 */
		inline Vec2 operator+(const Vec2 &b) const { return Vec2(x + b.x, y + b.y); }

		/**
		 * @param b vector to subtract.
		 * @return the vector difference.
		 */
		inline Vec2 operator-(const Vec2 &b) const { return Vec2(x - b.x, y - b.y); }

		/**
		 * @param b vector to multiply.
		 * @return the vector product.
		 */
		inline Vec2 operator*(const Vec2 &b) const { return Vec2(x * b.x, y * b.y); }

		/**
		 * @param b vector to divide.
		 * @return the vector product.
		 */
		inline Vec2 operator/(const Vec2 &b) const { return Vec2(x / b.x, y / b.y); }

		/**
		 * @param b scalar to multiply.
		 * @return the vector product.
		 */
		inline Vec2 operator*(const T &b) const { return Vec2(x * b, y * b); }

		/**
		 * @param b scalar to divide.
		 * @return the vector product.
		 */
		inline Vec2 operator/(const T &b) const { return Vec2(x / b, y / b); }

		/**
		 * @param b vector to add.
		 */
		inline void operator+=(const Vec2 &b) {
			x += b.x;
			y += b.y;
		}

		/**
		 * @param b vector to subtract.
		 */
		inline void operator-=(const Vec2 &b) {
			x -= b.x;
			y -= b.y;
		}

		/**
		 * @param b vector to multiply.
		 */
		inline void operator*=(const Vec2 &b) {
			x *= b.x;
			y *= b.y;
		}

		/**
		 * @param b vector to divide.
		 */
		inline void operator/=(const Vec2 &b) {
			x /= b.x;
			y /= b.y;
		}

		/**
		 * @param b scalar to multiply.
		 */
		inline void operator*=(const T &b) {
			x *= b;
			y *= b;
		}

		/**
		 * @param b scalar to divide.
		 */
		inline void operator/=(const T &b) {
			x /= b;
			y /= b;
		}

		/** @return minimum component reference. */
		inline const T &min() const {
			if (x < y) return x;
			else return y;
		}

		/** @return maximum component reference. */
		inline const T &max() const {
			if (x > y) return x;
			else return y;
		}

		/** Set maximum component. */
		inline void setMax(const Vec2 &b) {
			if (b.x > x) x = b.x;
			if (b.y > y) y = b.y;
		}

		/** Set minimum component. */
		inline void setMin(const Vec2 &b) {
			if (b.x < x) x = b.x;
			if (b.y < y) y = b.y;
		}

		/**
		 * @return vector length.
		 */
		inline GLfloat length() const { return sqrt(pow(x, 2) + pow(y, 2)); }

		/**
		 * @return squared vector length.
		 */
		inline GLfloat lengthSquared() const { return pow(x, 2) + pow(y, 2); }

		/**
		 * Normalize this vector.
		 */
		inline const Vec2& normalize() { *this /= length(); return *this; }

		/**
		 * @return normalized vector.
		 */
		inline Vec2 normalized() const { return Vec2(x, y) / length(); }

		/**
		 * Computes the dot product between two vectors.
		 * The dot product is equal to the acos of the angle
		 * between those vectors.
		 * @param b another vector.
		 * @return the dot product.
		 */
		inline T dot(const Vec2 &b) const {
			return x * b.x + y * b.y;
		}

		/**
		 * @return static zero vector.
		 */
		static const Vec2 &zero() {
			static Vec2 zero_(0);
			return zero_;
		}
	};

	// writing vector to output stream
	template<typename T>
	std::ostream &operator<<(std::ostream &os, const Vec2<T> &v) { return os << v.x << "," << v.y; }

	// reading vector from input stream
	template<typename T>
	std::istream &operator>>(std::istream &in, Vec2<T> &v) {
		readValue(in, v.x);
		readValue(in, v.y);
		return in;
	}

	typedef Vec2<GLfloat> Vec2f;
	typedef Vec2<GLdouble> Vec2d;
	typedef Vec2<GLint> Vec2i;
	typedef Vec2<GLuint> Vec2ui;
	typedef Vec2<GLboolean> Vec2b;

	/**
	 * \brief A 3D vector.
	 */
	template<typename T>
	class Vec3 {
	public:
		T x; /**< the x component. **/
		T y; /**< the y component. **/
		T z; /**< the z component. **/

		Vec3() : x(0), y(0), z(0) {}

		/** Set-component constructor. */
		Vec3(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

		/** @param _x value that is applied to all components. */
		Vec3(T _x) : x(_x), y(_x), z(_x) {}

		/** copy constructor. */
		Vec3(const Vec3 &b) : x(b.x), y(b.y), z(b.z) {}

		/** copy constructor. */
		template<typename U>
		explicit Vec3(const Vec3<U> &b) :
			x(static_cast<T>(b.x)),
			y(static_cast<T>(b.y)),
			z(static_cast<T>(b.z)) {}

		/** Construct from Vec2 and scalar. */
		Vec3(const Vec2<T> &b, T _z) : x(b.x), y(b.y), z(_z) {}

		/** Construct from Vec2 and scalar. */
		Vec3(T _x, const Vec2<T> &b) : x(_x), y(b.x), z(b.y) {}

		/** copy operator. */
		inline void operator=(const Vec3 &b) {
			x = b.x;
			y = b.y;
			z = b.z;
		}

		/**
		 * @param b another vector
		 * @return true if all values are equal
		 */
		inline bool operator==(const Vec3 &b) const { return x == b.x && y == b.y && z == b.z; }

		/**
		 * @param b another vector
		 * @return false if all values are equal
		 */
		inline bool operator!=(const Vec3 &b) const { return !operator==(b); }

		/**
		 * Subscript operator.
		 */
		inline T &operator[](int i) {
			return ((T *) this)[i];
		}

		/**
		 * Subscript operator.
		 */
		inline const T &operator[](int i) const {
			return ((T *) this)[i];
		}

		/**
		 * @return vector with each component negated.
		 */
		inline Vec3 operator-() const { return Vec3(-x, -y, -z); }

		/**
		 * @param b vector to add.
		 * @return the vector sum.
		 */
		inline Vec3 operator+(const Vec3 &b) const { return Vec3(x + b.x, y + b.y, z + b.z); }

		/**
		 * @param b vector to add.
		 * @return the vector sum.
		 */
		template<class U>
		inline Vec3<T> operator+(const Vec3<U> &b) const {
			return Vec3<T>(
				x + static_cast<T>(b.x),
				y + static_cast<T>(b.y),
				z + static_cast<T>(b.z));
		}

		/**
		 * @param b vector to subtract.
		 * @return the vector difference.
		 */
		inline Vec3 operator-(const Vec3 &b) const { return Vec3(x - b.x, y - b.y, z - b.z); }

		/**
		 * @param b vector to multiply.
		 * @return the vector product.
		 */
		inline Vec3 operator*(const Vec3 &b) const { return Vec3(x * b.x, y * b.y, z * b.z); }

		/**
		 * @param b vector to divide.
		 * @return the vector product.
		 */
		inline Vec3 operator/(const Vec3 &b) const { return Vec3(x / b.x, y / b.y, z / b.z); }

		/**
		 * @param b scalar to multiply.
		 * @return the vector product.
		 */
		inline Vec3 operator*(const T &b) const { return Vec3(x * b, y * b, z * b); }

		/**
		 * @param b scalar to divide.
		 * @return the vector product.
		 */
		inline Vec3 operator/(const T &b) const { return Vec3(x / b, y / b, z / b); }

		/**
		 * @param b vector to add.
		 */
		inline void operator+=(const Vec3 &b) {
			x += b.x;
			y += b.y;
			z += b.z;
		}

		/**
		 * @param b vector to subtract.
		 */
		inline void operator-=(const Vec3 &b) {
			x -= b.x;
			y -= b.y;
			z -= b.z;
		}

		/**
		 * @param b vector to multiply.
		 */
		inline void operator*=(const Vec3 &b) {
			x *= b.x;
			y *= b.y;
			z *= b.z;
		}

		/**
		 * @param b vector to divide.
		 */
		inline void operator/=(const Vec3 &b) {
			x /= b.x;
			y /= b.y;
			z /= b.z;
		}

		/**
		 * @param b scalar to multiply.
		 */
		inline void operator*=(const T &b) {
			x *= b;
			y *= b;
			z *= b;
		}

		/**
		 * @param b scalar to divide.
		 */
		inline void operator/=(const T &b) {
			x /= b;
			y /= b;
			z /= b;
		}

		/**
		 * @return vector length.
		 */
		inline GLfloat length() const { return sqrt(x * x + y * y + z * z); }

		/**
		 * @return squared vector length.
		 */
		inline GLfloat lengthSquared() const { return x * x + y * y + z * z; }

		/**
		 * Normalize this vector.
		 */
		inline Vec3& normalize() { *this /= length(); return *this; }

		/**
		 * Computes the cross product between two vectors.
		 * The result vector is perpendicular to both of the vectors being multiplied.
		 * @param b another vector.
		 * @return the cross product.
		 * @see http://en.wikipedia.org/wiki/Cross_product
		 */
		inline Vec3 cross(const Vec3 &b) const {
			return Vec3(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x);
		}

		/**
		 * Computes the dot product between two vectors.
		 * The dot product is equal to the acos of the angle
		 * between those vectors.
		 * @param b another vector.
		 * @return the dot product.
		 */
		inline T dot(const Vec3 &b) const {
			return x * b.x + y * b.y + z * b.z;
		}

		/**
		 * Rotates this vector around x/y/z axis.
		 * @param angle
		 * @param x_
		 * @param y_
		 * @param z_
		 */
		inline void rotate(GLfloat angle, GLfloat x_, GLfloat y_, GLfloat z_) {
			GLfloat c = cos(angle);
			GLfloat s = sin(angle);
			Vec3<GLfloat> rotated(
					(x_ * x_ * (1 - c) + c) * x
					+ (x_ * y_ * (1 - c) - z_ * s) * y
					+ (x_ * z_ * (1 - c) + y_ * s) * z,
					(y_ * x_ * (1 - c) + z_ * s) * x
					+ (y_ * y_ * (1 - c) + c) * y
					+ (y_ * z_ * (1 - c) - x_ * s) * z,
					(x_ * z_ * (1 - c) - y_ * s) * x
					+ (y_ * z_ * (1 - c) + x_ * s) * y
					+ (z_ * z_ * (1 - c) + c) * z
			);
			x = rotated.x;
			y = rotated.y;
			z = rotated.z;
		}

		/** @return xy component reference. */
		inline Vec2<T> &xy_() { return *((Vec2<T> *) this); }

		/** @return yz component reference. */
		inline Vec2<T> &yz_() { return *((Vec2<T> *) (((T *) this) + 1)); }

		/** @return x component reference. */
		inline T &x_() { return *((T *) this); }

		/** @return y component reference. */
		inline T &y_() { return *(((T *) this) + 1); }

		/** @return z component reference. */
		inline T &z_() { return *(((T *) this) + 2); }

		/** @return minimum component reference. */
		inline const T &min() const {
			if (x < y && x < z) return x;
			else if (y < z) return y;
			else return z;
		}

		/** @return maximum component reference. */
		inline const T &max() const {
			if (x > y && x > z) return x;
			else if (y > z) return y;
			else return z;
		}

		static inline Vec3 min(const Vec3 &a, const Vec3 &b) {
			return Vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
		}

		static inline Vec3 max(const Vec3 &a, const Vec3 &b) {
			return Vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
		}

		/** Set maximum component. */
		inline void setMax(const Vec3 &b) {
			x = std::max(x, b.x);
			y = std::max(y, b.y);
			z = std::max(z, b.z);
		}

		/** Set minimum component. */
		inline void setMin(const Vec3 &b) {
			x = std::min(x, b.x);
			y = std::min(y, b.y);
			z = std::min(z, b.z);
		}

		/**
		 * Compares vectors components.
		 * @return true if all components are nearly equal.
		 */
		inline GLboolean isApprox(const Vec3 &b, T delta) const {
			return abs(x - b.x) < delta && abs(y - b.y) < delta && abs(z - b.z) < delta;
		}

		/**
		 * @return static zero vector.
		 */
		static const Vec3 &zero() {
			static Vec3 zero_(0);
			return zero_;
		}

		/**
		 * @return static one vector.
		 */
		static const Vec3 &one() {
			static Vec3 one_(1);
			return one_;
		}

		/**
		 * @return static positive max vector.
		 */
		static const Vec3& posMax() {
			static Vec3 posMax_(std::numeric_limits<T>::max());
			return posMax_;
		}

		/**
		 * @return static negative max vector.
		 */
		static const Vec3& negMax() {
			static Vec3 negMax_(std::numeric_limits<T>::lowest());
			return negMax_;
		}

		/**
		 * @return static up vector.
		 */
		static const Vec3 &up() {
			static Vec3 up_(0, 1, 0);
			return up_;
		}

		/**
		 * @return static down vector.
		 */
		static const Vec3 &down() {
			static Vec3 up_(0, -1, 0);
			return up_;
		}

		/**
		 * @return static front vector.
		 */
		static const Vec3 &front() {
			static Vec3 up_(0, 0, 1);
			return up_;
		}

		/**
		 * @return static back vector.
		 */
		static const Vec3 &back() {
			static Vec3 up_(0, 0, -1);
			return up_;
		}

		/**
		 * @return static right vector.
		 */
		static const Vec3 &right() {
			static Vec3 up_(1, 0, 0);
			return up_;
		}

		/**
		 * @return static left vector.
		 */
		static const Vec3 &left() {
			static Vec3 up_(-1, 0, 0);
			return up_;
		}

		/**
		 * @return a random vector.
		 */
		static Vec3 random() {
			return Vec3(math::random<T>(), math::random<T>(), math::random<T>());
		}

		/**
		 * @return this vector as a Vec3f.
		 */
		Vec3<GLfloat> asVec3f() const {
			return {
				static_cast<GLfloat>(x),
				static_cast<GLfloat>(y),
				static_cast<GLfloat>(z) };
		}

		/**
		 * @return this vector as a Vec3f.
		 */
		Vec3<GLint> asVec3i() const {
			return {
					static_cast<GLint>(x),
					static_cast<GLint>(y),
					static_cast<GLint>(z) };
		}
	};

	// writing vector to output stream
	template<typename T>
	std::ostream &operator<<(std::ostream &os, const Vec3<T> &v) { return os << v.x << "," << v.y << "," << v.z; }

	// reading vector from input stream
	template<typename T>
	std::istream &operator>>(std::istream &in, Vec3<T> &v) {
		readValue(in, v.x);
		readValue(in, v.y);
		readValue(in, v.z);
		return in;
	}

	typedef Vec3<GLfloat> Vec3f;
	typedef Vec3<GLdouble> Vec3d;
	typedef Vec3<GLint> Vec3i;
	typedef Vec3<GLuint> Vec3ui;
	typedef Vec3<GLboolean> Vec3b;

	/**
	 * \brief A 4D vector.
	 */
	template<typename T>
	class Vec4 {
	public:
		T x; /**< the x component. **/
		T y; /**< the y component. **/
		T z; /**< the z component. **/
		T w; /**< the w component. **/

		Vec4() : x(0), y(0), z(0), w(0) {}

		/** Set-component constructor. */
		Vec4(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}

		/** @param _x value that is applied to all components. */
		Vec4(T _x) : x(_x), y(_x), z(_x), w(_x) {}

		/** copy constructor. */
		Vec4(const Vec4 &b) : x(b.x), y(b.y), z(b.z), w(b.w) {}

		/** Construct from two Vec2's. */
		Vec4(const Vec2<T> &a, const Vec2<T> &b) : x(a.x), y(a.y), z(b.x), w(b.y) {}

		/** Construct from Vec2 and two scalars. */
		Vec4(T _x, T _y, const Vec2<T> &b) : x(_x), y(_y), z(b.x), w(b.y) {}

		/** Construct from Vec2 and two scalars. */
		Vec4(T _x, const Vec2<T> &b, T _w) : x(_x), y(b.x), z(b.y), w(_w) {}

		/** Construct from Vec2 and two scalars. */
		Vec4(const Vec2<T> &b, T _z, T _w) : x(b.x), y(b.y), z(_z), w(_w) {}

		/** Construct from Vec3 and scalar. */
		Vec4(const Vec3<T> &b, T _w) : x(b.x), y(b.y), z(b.z), w(_w) {}

		/** Construct from Vec3 and scalar. */
		Vec4(T _x, const Vec3<T> &b) : x(_x), y(b.x), z(b.y), w(b.z) {}

		/** copy operator. */
		inline void operator=(const Vec4 &b) {
			x = b.x;
			y = b.y;
			z = b.z;
			w = b.w;
		}

		/**
		 * @param b another vector
		 * @return true if all values are equal
		 */
		inline bool operator==(const Vec4 &b) const { return x == b.x && y == b.y && z == b.z && w == b.w; }

		/**
		 * @param b another vector
		 * @return false if all values are equal
		 */
		inline bool operator!=(const Vec4 &b) const { return !operator==(b); }

		/**
		 * Subscript operator.
		 */
		inline T &operator[](int i) {
			return ((T *) this)[i];
		}

		/**
		 * Subscript operator.
		 */
		inline const T &operator[](int i) const {
			return ((T *) this)[i];
		}

		/**
		 * @return vector with each component negated.
		 */
		inline Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }

		/**
		 * @param b vector to add.
		 * @return the vector sum.
		 */
		inline Vec4 operator+(const Vec4 &b) const { return Vec4(x + b.x, y + b.y, z + b.z, w + b.w); }

		/**
		 * @param b vector to subtract.
		 * @return the vector sum.
		 */
		inline Vec4 operator-(const Vec4 &b) const { return Vec4(x - b.x, y - b.y, z - b.z, w - b.w); }

		/**
		 * @param b vector to multiply.
		 * @return the vector product.
		 */
		inline Vec4 operator*(const Vec4 &b) const { return Vec4(x * b.x, y * b.y, z * b.z, w * b.w); }

		/**
		 * @param b vector to divide.
		 * @return the vector product.
		 */
		inline Vec4 operator/(const Vec4 &b) const { return Vec4(x / b.x, y / b.y, z / b.z, w / b.w); }

		/**
		 * @param b scalar to multiply.
		 * @return the vector-scalar product.
		 */
		inline Vec4 operator*(const T &b) const { return Vec4(x * b, y * b, z * b, w * b); }

		/**
		 * @param b scalar to divide.
		 * @return the vector-scalar product.
		 */
		inline Vec4 operator/(const T &b) const { return Vec4(x / b, y / b, z / b, w / b); }

		/**
		 * @param b vector to add.
		 */
		inline void operator+=(const Vec4 &b) {
			x += b.x;
			y += b.y;
			z += b.z;
			w += b.w;
		}

		/**
		 * @param b vector to subtract.
		 */
		inline void operator-=(const Vec4 &b) {
			x -= b.x;
			y -= b.y;
			z -= b.z;
			w -= b.w;
		}

		/**
		 * @param b vector to multiply.
		 */
		inline void operator*=(const Vec4 &b) {
			x *= b.x;
			y *= b.y;
			z *= b.z;
			w *= b.w;
		}

		/**
		 * @param b vector to divide.
		 */
		inline void operator/=(const Vec4 &b) {
			x /= b.x;
			y /= b.y;
			z /= b.z;
			w /= b.w;
		}

		/**
		 * @param b scalar to multiply.
		 */
		inline void operator*=(const T &b) {
			x *= b;
			y *= b;
			z *= b;
			w *= b;
		}

		/**
		 * @param b scalar to divide.
		 */
		inline void operator/=(const T &b) {
			x /= b;
			y /= b;
			z /= b;
			w /= b;
		}

		/** @return xyz component reference. */
		inline Vec3<T> &xyz_() { return *((Vec3<T> *) this); }

		/** @return xyz component reference. */
		inline const Vec3<T> &xyz_() const { return *((Vec3<T> *) this); }

		/** @return yzw component reference. */
		inline Vec3<T> &yzw_() { return *((Vec3<T> *) (((T *) this) + 1)); }

		/** @return yzw component reference. */
		inline const Vec3<T> &yzw_() const { return *((Vec3<T> *) (((T *) this) + 1)); }

		/** @return xy component reference. */
		inline Vec2<T> &xy_() { return *((Vec2<T> *) this); }

		/** @return xy component reference. */
		inline const Vec2<T> &xy_() const { return *((Vec2<T> *) this); }

		/** @return yz component reference. */
		inline Vec2<T> &yz_() { return *((Vec2<T> *) (((T *) this) + 1)); }

		/** @return yz component reference. */
		inline const Vec2<T> &yz_() const { return *((Vec2<T> *) (((T *) this) + 1)); }

		/** @return zw component reference. */
		inline Vec2<T> &zw_() { return *((Vec2<T> *) (((T *) this) + 2)); }

		/** @return zw component reference. */
		inline const Vec2<T> &zw_() const { return *((Vec2<T> *) (((T *) this) + 2)); }

		/** @return x component reference. */
		inline T &x_() { return *((T *) this); }

		/** @return x component reference. */
		inline const T &x_() const { return *((T *) this); }

		/** @return y component reference. */
		inline T &y_() { return *(((T *) this) + 1); }

		/** @return y component reference. */
		inline const T &y_() const { return *(((T *) this) + 1); }

		/** @return z component reference. */
		inline T &z_() { return *(((T *) this) + 2); }

		/** @return z component reference. */
		inline const T &z_() const { return *(((T *) this) + 2); }

		/** @return w component reference. */
		inline T &w_() { return *(((T *) this) + 3); }

		/** @return w component reference. */
		inline const T &w_() const { return *(((T *) this) + 3); }

		/**
		 * Compares vectors components.
		 * @return true if all components are nearly equal.
		 */
		inline GLboolean isApprox(const Vec4 &b, T delta) const {
			return abs(x - b.x) < delta && abs(y - b.y) < delta && abs(z - b.z) < delta && abs(w - b.w) < delta;
		}

		/**
		 * @return static zero vector.
		 */
		static const Vec4 &zero() {
			static Vec4 zero_(0);
			return zero_;
		}

		/**
		 * @return static one vector.
		 */
		static const Vec4 &one() {
			static Vec4 one_(1);
			return one_;
		}
	};

	// writing vector to output stream
	template<typename T>
	std::ostream &operator<<(std::ostream &os, const Vec4<T> &v) {
		return os << v.x << "," << v.y << "," << v.z << "," << v.w;
	}

	// reading vector from input stream
	template<typename T>
	std::istream &operator>>(std::istream &in, Vec4<T> &v) {
		readValue(in, v.x);
		readValue(in, v.y);
		readValue(in, v.z);
		readValue(in, v.w);
		return in;
	}

	typedef Vec4<GLfloat> Vec4f;
	typedef Vec4<GLdouble> Vec4d;
	typedef Vec4<GLint> Vec4i;
	typedef Vec4<GLuint> Vec4ui;
	typedef Vec4<GLboolean> Vec4b;

	/**
	 * \brief A 1D vector.
	 */
	template<typename T>
	class Vec1 {
	public:
		T x; /**< the x component. **/

		Vec1() : x(0) {}

		/** @param _x value that is applied to all components. */
		explicit Vec1(T _x) : x(_x) {}

		/** copy constructor. */
		Vec1(const Vec1 &b) : x(b.x) {}

		/** copy operator. */
		inline void operator=(const Vec1 &b) {
			x = b.x;
		}

		/**
		 * @param b another vector
		 * @return true if all values are equal
		 */
		inline bool operator==(const Vec1 &b) const { return x == b.x; }

		/**
		 * @param b another vector
		 * @return false if all values are equal
		 */
		inline bool operator!=(const Vec1 &b) const { return !operator==(b); }

		/**
		 * Subscript operator.
		 */
		inline T &operator[](int i) {
			return ((T *) this)[i];
		}

		/**
		 * Subscript operator.
		 */
		inline const T &operator[](int i) const {
			return ((T *) this)[i];
		}
	};

	typedef Vec1<GLfloat> Vec1f;
	typedef Vec1<GLdouble> Vec1d;
	typedef Vec1<GLint> Vec1i;
	typedef Vec1<GLuint> Vec1ui;
	typedef Vec1<GLboolean> Vec1b;

	/**
	 * \brief A 6D vector.
	 */
	template<typename T>
	class Vec6 {
	public:
		T x0; /**< the 1. component. **/
		T x1; /**< the 2. component. **/
		T x2; /**< the 3. component. **/
		T x3; /**< the 4. component. **/
		T x4; /**< the 5. component. **/
		T x5; /**< the 6. component. **/
		Vec6() : x0(0), x1(0), x2(0), x3(0), x4(0), x5(0) {}

		/** Construct from two Vec3's. */
		Vec6(const Vec3f &v1, const Vec3f &v2)
				: x0(v1.x), x1(v1.y), x2(v1.z), x3(v2.x), x4(v2.y), x5(v2.z) {}
	};

	typedef Vec6<GLfloat> Vec6f;
	typedef Vec6<GLdouble> Vec6d;
	typedef Vec6<GLint> Vec6i;
	typedef Vec6<GLuint> Vec6ui;
	typedef Vec6<GLboolean> Vec6b;

	Vec4f calculateTangent(Vec3f *vertices, Vec2f *texco, const Vec3f &normal);
} // namespace

#endif /* REGEN_VECTOR_H_ */
