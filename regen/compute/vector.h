#ifndef REGEN_VECTOR_H_
#define REGEN_VECTOR_H_

#include <cmath>
#include <cassert>
#include <list>

#include <regen/utility/strings.h>
#include <regen/compute/math.h>

namespace regen {
	/** \brief Traits to get base type of vector types. */
    template<typename T> struct VecTraits;

	/** Traits specialization for scalar types. */
	template<> struct VecTraits<float> { using BaseType = float; };
	template<> struct VecTraits<double> { using BaseType = double; };
	template<> struct VecTraits<int> { using BaseType = int; };
	template<> struct VecTraits<uint32_t> { using BaseType = uint32_t; };
	template<> struct VecTraits<bool> { using BaseType = bool; };

	/**
	 * \brief A 2D vector.
	 */
	template<typename T> struct Vec2 {
		using BaseType = T;

		T x; /**< the x component. **/
		T y; /**< the y component. **/

		static constexpr Vec2 create(T v) noexcept {
			return Vec2(v, v);
		}

		/**
		 * @param b another vector
		 * @return true if all values are equal
		 */
		constexpr bool operator==(const Vec2 &b) const { return x == b.x && y == b.y; }

		/**
		 * @param b another vector
		 * @return false if all values are equal
		 */
		constexpr bool operator!=(const Vec2 &b) const { return !operator==(b); }

		/**
		 * Subscript operator.
		 */
		constexpr T &operator[](int i) { return (&x)[i]; }

		/**
		 * Subscript operator.
		 */
		constexpr const T &operator[](int i) const { return (&x)[i]; }

		/**
		 * @return vector with each component negated.
		 */
		constexpr Vec2 operator-() const { return Vec2{-x, -y}; }

		/**
		 * @param b vector to add.
		 * @return the vector sum.
		 */
		constexpr Vec2 operator+(const Vec2 &b) const { return Vec2{x + b.x, y + b.y}; }

		/**
		 * @param b vector to subtract.
		 * @return the vector difference.
		 */
		constexpr Vec2 operator-(const Vec2 &b) const { return Vec2{x - b.x, y - b.y}; }

		/**
		 * @param b vector to multiply.
		 * @return the vector product.
		 */
		constexpr Vec2 operator*(const Vec2 &b) const { return Vec2{x * b.x, y * b.y}; }

		/**
		 * @param b vector to divide.
		 * @return the vector product.
		 */
		constexpr Vec2 operator/(const Vec2 &b) const { return Vec2{x / b.x, y / b.y}; }

		/**
		 * @param b scalar to multiply.
		 * @return the vector product.
		 */
		constexpr Vec2 operator*(const T &b) const { return Vec2{x * b, y * b}; }

		/**
		 * @param b scalar to divide.
		 * @return the vector product.
		 */
		constexpr Vec2 operator/(const T &b) const { return Vec2{x / b, y / b}; }

		/**
		 * @param b vector to add.
		 */
		constexpr void operator+=(const Vec2 &b) {
			x += b.x;
			y += b.y;
		}

		/**
		 * @param b vector to subtract.
		 */
		constexpr void operator-=(const Vec2 &b) {
			x -= b.x;
			y -= b.y;
		}

		/**
		 * @param b vector to multiply.
		 */
		constexpr void operator*=(const Vec2 &b) {
			x *= b.x;
			y *= b.y;
		}

		/**
		 * @param b vector to divide.
		 */
		constexpr void operator/=(const Vec2 &b) {
			x /= b.x;
			y /= b.y;
		}

		/**
		 * @param b scalar to multiply.
		 */
		constexpr void operator*=(const T &b) {
			x *= b;
			y *= b;
		}

		/**
		 * @param b scalar to divide.
		 */
		constexpr void operator/=(const T &b) {
			x /= b;
			y /= b;
		}

		/** @return minimum component reference. */
		constexpr const T &min() const { return (x < y) ? x : y; }

		/** @return maximum component reference. */
		constexpr const T &max() const { return (x > y) ? x : y; }

		/** Set maximum component. */
		constexpr void setMax(const Vec2 &b) {
			if (b.x > x) x = b.x;
			if (b.y > y) y = b.y;
		}

		/** Set minimum component. */
		constexpr void setMin(const Vec2 &b) {
			if (b.x < x) x = b.x;
			if (b.y < y) y = b.y;
		}

		/**
		 * @return vector length.
		 */
		constexpr float length() const { return sqrt(x * x + y * y); }

		/**
		 * @return squared vector length.
		 */
		constexpr float lengthSquared() const { return x * x + y * y; }

		/**
		 * Normalize this vector.
		 */
		constexpr const Vec2& normalize() {
			*this /= length();
			return *this;
		}

		/**
		 * @return normalized vector.
		 */
		constexpr Vec2 normalized() const {
			return Vec2{x, y} / length();
		}

		/**
		 * Computes the dot product between two vectors.
		 * The dot product is equal to the acos of the angle
		 * between those vectors.
		 * @param b another vector.
		 * @return the dot product.
		 */
		constexpr T dot(const Vec2 &b) const {
			return x * b.x + y * b.y;
		}

		/**
		 * @return static zero vector.
		 */
		static constexpr const Vec2 &zero() {
			static constexpr Vec2 Zero(0,0);
			return Zero;
		}

		/**
		 * @return static one vector.
		 */
		static constexpr const Vec2 &one() {
			static constexpr Vec2 One(1,1);
			return One;
		}
	};

	// writing vector to output stream
	template<typename T>
	std::ostream &operator<<(std::ostream &os, const Vec2<T> &v) { return os << v.x << "," << v.y; }

	// reading vector from input stream
	template<typename T>
	std::istream &operator>>(std::istream &in, Vec2<T> &v) {
		readValue(in, v.x, T(0));
		readValue(in, v.y, T(0));
		return in;
	}

	/** \brief Vector type for 2D float vectors. */
	typedef Vec2<float> Vec2f;
	/** \brief Vector type for 2D double vectors. */
	typedef Vec2<double> Vec2d;
	/** \brief Vector type for 2D int vectors. */
	typedef Vec2<int> Vec2i;
	/** \brief Vector type for 2D uint32_t vectors. */
	typedef Vec2<uint32_t> Vec2ui;
	/** \brief Vector type for 2D bool vectors. */
	typedef Vec2<bool> Vec2b;

	// Specializations of VecTraits for 2D vector types
	template<> struct VecTraits<Vec2f> { using BaseType = float; };
	template<> struct VecTraits<Vec2d> { using BaseType = double; };
	template<> struct VecTraits<Vec2i> { using BaseType = int; };
	template<> struct VecTraits<Vec2ui> { using BaseType = uint32_t; };
	template<> struct VecTraits<Vec2b> { using BaseType = bool; };

	/**
	 * \brief A 3D vector.
	 */
	template<typename T> struct Vec3 {
		using BaseType = T;

		T x; /**< the x component. **/
		T y; /**< the y component. **/
		T z; /**< the z component. **/

		/** Construct from Vec2 and scalar. */
		static constexpr Vec3 create(T v) {
			return Vec3{v, v, v};
		}

		/** Construct from Vec2 and scalar. */
		static constexpr Vec3 create(Vec2<T> &x1, T x2) {
			return Vec3{x1.x, x1.y, x2};
		}

		/** Construct from Vec2 and scalar. */
		static constexpr Vec3 create(T x1, Vec2<T> &x2) {
			return Vec3{x1, x2.x, x2.y};
		}

		/** Construct from another vector type. */
		template<typename U> static constexpr Vec3 create(const Vec3<U> &b) {
			return Vec3{
				static_cast<T>(b.x),
				static_cast<T>(b.y),
				static_cast<T>(b.z)};
		}

		/**
		 * @param b another vector
		 * @return true if all values are equal
		 */
		constexpr bool operator==(const Vec3 &b) const {
			return x == b.x && y == b.y && z == b.z;
		}

		/**
		 * @param b another vector
		 * @return false if all values are equal
		 */
		constexpr bool operator!=(const Vec3 &b) const {
			return !operator==(b);
		}

		/**
		 * Subscript operator.
		 */
		constexpr T &operator[](int i) { return (&x)[i]; }

		/**
		 * Subscript operator.
		 */
		constexpr const T &operator[](int i) const { return (&x)[i]; }

		/**
		 * @return vector with each component negated.
		 */
		constexpr Vec3 operator-() const { return Vec3{-x, -y, -z}; }

		/**
		 * @param b vector to add.
		 * @return the vector sum.
		 */
		constexpr Vec3 operator+(const Vec3 &b) const {
			return Vec3{x + b.x, y + b.y, z + b.z};
		}

		/**
		 * @param b vector to add.
		 * @return the vector sum.
		 */
		template<class U> Vec3<T> constexpr operator+(const Vec3<U> &b) const {
			return Vec3<T>{
				x + static_cast<T>(b.x),
				y + static_cast<T>(b.y),
				z + static_cast<T>(b.z)};
		}

		/**
		 * @param b vector to subtract.
		 * @return the vector difference.
		 */
		constexpr Vec3 operator-(const Vec3 &b) const {
			return Vec3{x - b.x, y - b.y, z - b.z};
		}

		/**
		 * @param b vector to multiply.
		 * @return the vector product.
		 */
		constexpr Vec3 operator*(const Vec3 &b) const {
			return Vec3{x * b.x, y * b.y, z * b.z};
		}

		/**
		 * @param b vector to divide.
		 * @return the vector product.
		 */
		constexpr Vec3 operator/(const Vec3 &b) const {
			return Vec3{x / b.x, y / b.y, z / b.z};
		}

		/**
		 * @param b scalar to multiply.
		 * @return the vector product.
		 */
		constexpr Vec3 operator*(const T &b) const {
			return Vec3{x * b, y * b, z * b};
		}

		/**
		 * @param b scalar to divide.
		 * @return the vector product.
		 */
		constexpr Vec3 operator/(const T &b) const {
			return Vec3{x / b, y / b, z / b};
		}

		/**
		 * @param b vector to add.
		 */
		constexpr void operator+=(const Vec3 &b) {
			x += b.x;
			y += b.y;
			z += b.z;
		}

		/**
		 * @param b vector to subtract.
		 */
		constexpr void operator-=(const Vec3 &b) {
			x -= b.x;
			y -= b.y;
			z -= b.z;
		}

		/**
		 * @param b vector to multiply.
		 */
		constexpr void operator*=(const Vec3 &b) {
			x *= b.x;
			y *= b.y;
			z *= b.z;
		}

		/**
		 * @param b vector to divide.
		 */
		constexpr void operator/=(const Vec3 &b) {
			x /= b.x;
			y /= b.y;
			z /= b.z;
		}

		/**
		 * @param b scalar to multiply.
		 */
		constexpr void operator*=(const T &b) {
			x *= b;
			y *= b;
			z *= b;
		}

		/**
		 * @param b scalar to divide.
		 */
		constexpr void operator/=(const T &b) {
			x /= b;
			y /= b;
			z /= b;
		}

		/**
		 * @return vector with each component as absolute value.
		 */
		constexpr Vec3 abs() const {
			return Vec3{std::abs(x), std::abs(y), std::abs(z)};
		}

		/**
		 * @return vector length.
		 */
		constexpr float length() const {
			return sqrtf(x * x + y * y + z * z);
		}

		/**
		 * @return squared vector length.
		 */
		constexpr float lengthSquared() const {
			return x * x + y * y + z * z;
		}

		/**
		 * Normalize this vector.
		 */
		constexpr Vec3& normalize() {
			*this /= length();
			return *this;
		}

		/**
		 * Computes the cross product between two vectors.
		 * The result vector is perpendicular to both of the vectors being multiplied.
		 * @param b another vector.
		 * @return the cross product.
		 * @see http://en.wikipedia.org/wiki/Cross_product
		 */
		constexpr Vec3 cross(const Vec3 &b) const {
			return Vec3{
				y * b.z - z * b.y,
				z * b.x - x * b.z,
				x * b.y - y * b.x};
		}

		/**
		 * Computes the dot product between two vectors.
		 * The dot product is equal to the acos of the angle
		 * between those vectors.
		 * @param b another vector.
		 * @return the dot product.
		 */
		constexpr T dot(const Vec3 &b) const {
			return x * b.x + y * b.y + z * b.z;
		}

		/**
		 * Computes the dot product between this vector and the vector
		 * defined by the three pointer components.
		 * @param ox x component pointer.
		 * @param oy y component pointer.
		 * @param oz z component pointer.
		 * @return the dot product.
		 */
		constexpr T dot(const T *ox, const T *oy, const T *oz) const {
			return x * (*ox) + y * (*oy) + z * (*oz);
		}

		/**
		 * Rotates this vector around x/y/z axis.
		 * @param angle rotation angle in radians.
		 * @param rx x component of rotation axis.
		 * @param ry y component of rotation axis.
		 * @param rz z component of rotation axis.
		 */
		constexpr void rotate(float angle, float rx, float ry, float rz) {
			float c = cosf(angle);
			float s = sinf(angle);
			x = (rx * rx * (1 - c) + c) * x
				+ (rx * ry * (1 - c) - rz * s) * y
				+ (rx * rz * (1 - c) + ry * s) * z;
			y = (ry * rx * (1 - c) + rz * s) * x
				+ (ry * ry * (1 - c) + c) * y
				+ (ry * rz * (1 - c) - rx * s) * z;
			z = (rx * rz * (1 - c) - ry * s) * x
				+ (ry * rz * (1 - c) + rx * s) * y
				+ (rz * rz * (1 - c) + c) * z;
		}

		/** @return xy component reference. */
		constexpr Vec2<T>& xy() {
			return *((Vec2<T> *) this);
		}

		/** @return yz component reference. */
		constexpr Vec2<T>& yz() {
			return *((Vec2<T> *) (((T *) this) + 1));
		}

		/** @return minimum component reference. */
		constexpr const T &min() const {
			if (x < y && x < z) return x;
			else if (y < z) return y;
			else return z;
		}

		/** @return maximum component reference. */
		constexpr const T &max() const {
			if (x > y && x > z) return x;
			else if (y > z) return y;
			else return z;
		}

		/** Return vector with minimum components of a and b. */
		static constexpr Vec3 min(const Vec3 &a, const Vec3 &b) {
			return Vec3{
				std::min(a.x, b.x),
				std::min(a.y, b.y),
				std::min(a.z, b.z)};
		}

		/** Return vector with maximum components of a and b. */
		static constexpr Vec3 max(const Vec3 &a, const Vec3 &b) {
			return Vec3{
				std::max(a.x, b.x),
				std::max(a.y, b.y),
				std::max(a.z, b.z)};
		}

		/** Set maximum component. */
		constexpr void setMax(const Vec3 &b) {
			x = std::max(x, b.x);
			y = std::max(y, b.y);
			z = std::max(z, b.z);
		}

		/** Set minimum component. */
		constexpr void setMin(const Vec3 &b) {
			x = std::min(x, b.x);
			y = std::min(y, b.y);
			z = std::min(z, b.z);
		}

		/**
		 * Sets each component to its absolute value.
		 */
		constexpr void setAbs() {
			x = std::abs(x);
			y = std::abs(y);
			z = std::abs(z);
		}

		/**
		 * Compares vectors components.
		 * @return true if all components are nearly equal.
		 */
		constexpr bool isApprox(const Vec3 &b, T delta) const {
			return
				std::abs(x - b.x) < delta &&
				std::abs(y - b.y) < delta &&
				std::abs(z - b.z) < delta;
		}

		/**
		 * @return static zero vector.
		 */
		static constexpr const Vec3 &zero() {
			static constexpr Vec3 Zero(0,0,0);
			return Zero;
		}

		/**
		 * @return static one vector.
		 */
		static constexpr const Vec3 &one() {
			static constexpr Vec3 One(1,1,1);
			return One;
		}

		/**
		 * @return static positive max vector.
		 */
		static constexpr const Vec3& posMax() {
			static constexpr Vec3 PosMax = Vec3::create(std::numeric_limits<T>::max());
			return PosMax;
		}

		/**
		 * @return static negative max vector.
		 */
		static constexpr const Vec3& negMax() {
			static constexpr Vec3 NegMax = Vec3::create(std::numeric_limits<T>::lowest());
			return NegMax;
		}

		/**
		 * @return static up vector.
		 */
		static constexpr const Vec3& up() {
			static constexpr Vec3 Up(0, 1, 0);
			return Up;
		}

		/**
		 * @return static down vector.
		 */
		static constexpr const Vec3& down() {
			static constexpr Vec3 Down(0, -1, 0);
			return Down;
		}

		/**
		 * @return static front vector.
		 */
		static constexpr const Vec3& front() {
			static constexpr Vec3 Front(0, 0, 1);
			return Front;
		}

		/**
		 * @return static back vector.
		 */
		static constexpr const Vec3& back() {
			static constexpr Vec3 Back(0, 0, -1);
			return Back;
		}

		/**
		 * @return static right vector.
		 */
		static constexpr const Vec3& right() {
			static constexpr Vec3 Right(1, 0, 0);
			return Right;
		}

		/**
		 * @return static left vector.
		 */
		static constexpr const Vec3& left() {
			static constexpr Vec3 Left(-1, 0, 0);
			return Left;
		}

		/**
		 * @return a random vector.
		 */
		static constexpr Vec3 random() {
			return Vec3{
				math::random<T>(),
				math::random<T>(),
				math::random<T>()};
		}

		/**
		 * @return this vector as a Vec3f.
		 */
		constexpr Vec3<float> asVec3f() const {
			return {
				static_cast<float>(x),
				static_cast<float>(y),
				static_cast<float>(z) };
		}

		/**
		 * @return this vector as a Vec3f.
		 */
		constexpr Vec3<int> asVec3i() const {
			return {
					static_cast<int>(x),
					static_cast<int>(y),
					static_cast<int>(z) };
		}
	};

	// writing vector to output stream
	template<typename T>
	std::ostream &operator<<(std::ostream &os, const Vec3<T> &v) { return os << v.x << "," << v.y << "," << v.z; }

	// reading vector from input stream
	template<typename T>
	std::istream &operator>>(std::istream &in, Vec3<T> &v) {
		readValue(in, v.x, T(0));
		readValue(in, v.y, T(0));
		readValue(in, v.z, T(0));
		return in;
	}

	/** \brief Vector type for 3D float vectors. */
	typedef Vec3<float> Vec3f;
	/** \brief Vector type for 3D double vectors. */
	typedef Vec3<double> Vec3d;
	/** \brief Vector type for 3D int vectors. */
	typedef Vec3<int> Vec3i;
	/** \brief Vector type for 3D uint32_t vectors. */
	typedef Vec3<uint32_t> Vec3ui;
	/** \brief Vector type for 3D bool vectors. */
	typedef Vec3<bool> Vec3b;

	// Specializations of VecTraits for 3D vector types
	template<> struct VecTraits<Vec3f> { using BaseType = float; };
	template<> struct VecTraits<Vec3d> { using BaseType = double; };
	template<> struct VecTraits<Vec3i> { using BaseType = int; };
	template<> struct VecTraits<Vec3ui> { using BaseType = uint32_t; };
	template<> struct VecTraits<Vec3b> { using BaseType = bool; };

	/**
	 * \brief A 4D vector.
	 */
	template<typename T>
	class Vec4 {
	public:
		using BaseType = T;

		T x; /**< the x component. **/
		T y; /**< the y component. **/
		T z; /**< the z component. **/
		T w; /**< the w component. **/

		/** Construct from two Vec2's. */
		static constexpr Vec4 create(const Vec2<T> &a, const Vec2<T> &b) {
			return Vec4{a.x, a.y, b.x, b.y};
		}

		/** Construct from Vec2 and two scalars. */
		static constexpr Vec4 create(T x, T y, const Vec2<T> &b) {
			return Vec4{x, y, b.x, b.y};
		}

		/** Construct from Vec2 and two scalars. */
		static constexpr Vec4 create(T x, const Vec2<T> &b, T w) {
			return Vec4{x, b.x, b.y, w};
		}

		/** Construct from Vec2 and two scalars. */
		static constexpr Vec4 create(const Vec2<T> &b, T z, T w) {
			return Vec4{b.x, b.y, z, w};
		}

		/** Construct from Vec3 and scalar. */
		static constexpr Vec4 create(const Vec3<T> &b, T w) {
			return Vec4{b.x, b.y, b.z, w};
		}

		/** Construct from Vec3 and scalar. */
		static constexpr Vec4 create(T x, const Vec3<T> &b) {
			return Vec4{x, b.x, b.y, b.z};
		}

		/** Construct from a scalar. */
		static constexpr Vec4 create(T v) {
			return Vec4{v, v, v, v};
		}

		/**
		 * @param b another vector
		 * @return true if all values are equal
		 */
		constexpr bool operator==(const Vec4 &b) const {
			return x == b.x && y == b.y && z == b.z && w == b.w;
		}

		/**
		 * @param b another vector
		 * @return false if all values are equal
		 */
		constexpr bool operator!=(const Vec4 &b) const {
			return !operator==(b);
		}

		/**
		 * Subscript operator.
		 */
		constexpr T &operator[](int i) { return (&x)[i]; }

		/**
		 * Subscript operator.
		 */
		constexpr const T &operator[](int i) const { return (&x)[i]; }

		/**
		 * @return vector with each component negated.
		 */
		constexpr Vec4 operator-() const { return Vec4{-x, -y, -z, -w}; }

		/**
		 * @param b vector to add.
		 * @return the vector sum.
		 */
		constexpr Vec4 operator+(const Vec4 &b) const {
			return Vec4{x + b.x, y + b.y, z + b.z, w + b.w};
		}

		/**
		 * @param b vector to subtract.
		 * @return the vector sum.
		 */
		constexpr Vec4 operator-(const Vec4 &b) const {
			return Vec4{x - b.x, y - b.y, z - b.z, w - b.w};
		}

		/**
		 * @param b vector to multiply.
		 * @return the vector product.
		 */
		constexpr Vec4 operator*(const Vec4 &b) const {
			return Vec4{x * b.x, y * b.y, z * b.z, w * b.w};
		}

		/**
		 * @param b vector to divide.
		 * @return the vector product.
		 */
		constexpr Vec4 operator/(const Vec4 &b) const {
			return Vec4{x / b.x, y / b.y, z / b.z, w / b.w};
		}

		/**
		 * @param b scalar to multiply.
		 * @return the vector-scalar product.
		 */
		constexpr Vec4 operator*(const T &b) const {
			return Vec4{x * b, y * b, z * b, w * b};
		}

		/**
		 * @param b scalar to divide.
		 * @return the vector-scalar product.
		 */
		constexpr Vec4 operator/(const T &b) const {
			return Vec4{x / b, y / b, z / b, w / b};
		}

		/**
		 * @param b vector to add.
		 */
		constexpr void operator+=(const Vec4 &b) {
			x += b.x;
			y += b.y;
			z += b.z;
			w += b.w;
		}

		/**
		 * @param b vector to subtract.
		 */
		constexpr void operator-=(const Vec4 &b) {
			x -= b.x;
			y -= b.y;
			z -= b.z;
			w -= b.w;
		}

		/**
		 * @param b vector to multiply.
		 */
		constexpr void operator*=(const Vec4 &b) {
			x *= b.x;
			y *= b.y;
			z *= b.z;
			w *= b.w;
		}

		/**
		 * @param b vector to divide.
		 */
		constexpr void operator/=(const Vec4 &b) {
			x /= b.x;
			y /= b.y;
			z /= b.z;
			w /= b.w;
		}

		/**
		 * @param b scalar to multiply.
		 */
		constexpr void operator*=(const T &b) {
			x *= b;
			y *= b;
			z *= b;
			w *= b;
		}

		/**
		 * @param b scalar to divide.
		 */
		constexpr void operator/=(const T &b) {
			x /= b;
			y /= b;
			z /= b;
			w /= b;
		}

		/** @return xyz component reference. */
		constexpr Vec3<T> &xyz() {
			return *((Vec3<T> *) this);
		}

		/** @return xyz component reference. */
		constexpr const Vec3<T> &xyz() const {
			return *((Vec3<T> *) this);
		}

		/** @return yzw component reference. */
		constexpr Vec3<T> &yzw() {
			return *((Vec3<T> *) (((T *) this) + 1));
		}

		/** @return yzw component reference. */
		constexpr const Vec3<T> &yzw() const {
			return *((Vec3<T> *) (((T *) this) + 1));
		}

		/** @return xy component reference. */
		constexpr Vec2<T> &xy() {
			return *((Vec2<T> *) this);
		}

		/** @return xy component reference. */
		constexpr const Vec2<T> &xy() const {
			return *((Vec2<T> *) this);
		}

		/** @return yz component reference. */
		constexpr Vec2<T> &yz() {
			return *((Vec2<T> *) (((T *) this) + 1));
		}

		/** @return yz component reference. */
		constexpr const Vec2<T> &yz() const {
			return *((Vec2<T> *) (((T *) this) + 1));
		}

		/** @return zw component reference. */
		constexpr Vec2<T> &zw() {
			return *((Vec2<T> *) (((T *) this) + 2));
		}

		/** @return zw component reference. */
		constexpr const Vec2<T> &zw() const {
			return *((Vec2<T> *) (((T *) this) + 2));
		}

		/**
		 * Compares vectors components.
		 * @return true if all components are nearly equal.
		 */
		constexpr bool isApprox(const Vec4 &b, T delta) const {
			return abs(x - b.x) < delta &&
				abs(y - b.y) < delta &&
				abs(z - b.z) < delta &&
				abs(w - b.w) < delta;
		}

		/**
		 * @return static zero vector.
		 */
		static constexpr const Vec4 &zero() {
			static constexpr Vec4 Zero(0,0,0,0);
			return Zero;
		}

		/**
		 * @return static one vector.
		 */
		static constexpr const Vec4 &one() {
			static constexpr Vec4 One(1,1,1,1);
			return One;
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
		readValue(in, v.x, T(0));
		readValue(in, v.y, T(0));
		readValue(in, v.z, T(0));
		readValue(in, v.w, T(0));
		return in;
	}

	/** \brief Vector type for 4D float vectors. */
	typedef Vec4<float> Vec4f;
	/** \brief Vector type for 4D double vectors. */
	typedef Vec4<double> Vec4d;
	/** \brief Vector type for 4D int vectors. */
	typedef Vec4<int> Vec4i;
	/** \brief Vector type for 4D uint32_t vectors. */
	typedef Vec4<uint32_t> Vec4ui;
	/** \brief Vector type for 4D bool vectors. */
	typedef Vec4<bool> Vec4b;

	// Specializations of VecTraits for 4D vector types
	template<> struct VecTraits<Vec4f> { using BaseType = float; };
	template<> struct VecTraits<Vec4d> { using BaseType = double; };
	template<> struct VecTraits<Vec4i> { using BaseType = int; };
	template<> struct VecTraits<Vec4ui> { using BaseType = uint32_t; };
	template<> struct VecTraits<Vec4b> { using BaseType = bool; };

	/**
	 * \brief A 6D vector.
	 */
	template<typename T> struct Vec6 {
		using BaseType = T;

		T x0; /**< the 1. component. **/
		T x1; /**< the 2. component. **/
		T x2; /**< the 3. component. **/
		T x3; /**< the 4. component. **/
		T x4; /**< the 5. component. **/
		T x5; /**< the 6. component. **/

		/** Construct from two Vec3's. */
		static constexpr Vec6<float> create(const Vec3f &v1, const Vec3f &v2) {
			return Vec6{v1.x, v1.y, v1.z, v2.x, v2.y, v2.z};
		}
	};

	/** \brief Vector type for 6D float vectors. */
	typedef Vec6<float> Vec6f;
	/** \brief Vector type for 6D double vectors. */
	typedef Vec6<double> Vec6d;
	/** \brief Vector type for 6D int vectors. */
	typedef Vec6<int> Vec6i;
	/** \brief Vector type for 6D uint32_t vectors. */
	typedef Vec6<uint32_t> Vec6ui;
	/** \brief Vector type for 6D bool vectors. */
	typedef Vec6<bool> Vec6b;

	// Specializations of VecTraits for 6D vector types
	template<> struct VecTraits<Vec6f> { using BaseType = float; };
	template<> struct VecTraits<Vec6d> { using BaseType = double; };
	template<> struct VecTraits<Vec6i> { using BaseType = int; };
	template<> struct VecTraits<Vec6ui> { using BaseType = uint32_t; };
	template<> struct VecTraits<Vec6b> { using BaseType = bool; };

	/** \brief Helper to create vectors. Needed for scalar special case. */
	struct Vec {
		template <typename VecType> static VecType create(VecTraits<VecType>::BaseType v) {
			static constexpr int NumComponents = sizeof(VecType) / sizeof(typename VecTraits<VecType>::BaseType);
			if constexpr (NumComponents == 1) {
				return v;
			} else {
				return VecType::create(v);
			}
		}
	};

	/**
	 * Calculate tangent vector for a triangle.
	 * @param vertices array of three triangle vertices.
	 * @param texco array of three triangle texture coordinates.
	 * @param normal triangle normal vector.
	 * @return tangent vector as Vec4f where w is the handedness.
	 */
	Vec4f calculateTangent(Vec3f *vertices, Vec2f *texco, const Vec3f &normal);
} // namespace

#endif /* REGEN_VECTOR_H_ */
