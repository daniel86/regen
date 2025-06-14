#include "plane.h"

using namespace regen;

Plane::Plane() = default;

Plane::Plane(const Vec3f &p, const Vec3f &n) {
	point = p;
	normal = n;
}

Vec4f Plane::equation() const {
	return { -normal.x, -normal.y, -normal.z, normal.dot(point) };
}

void Plane::set(const Vec3f &p0, const Vec3f &p1, const Vec3f &p2) {
	point = p0;
	// TODO: I think the normal is reversed.
	//         - plane equation should be {n, -n.dot(p)}
	//         - requires some changes in camera/frustum code
	normal = (p2 - p0).cross(p1 - p0);
	//normal = (p1 - p0).cross(p2 - p0);
	normal.normalize();
}
