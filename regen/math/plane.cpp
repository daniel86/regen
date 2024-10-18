/*
 * plane.cpp
 *
 *  Created on: Oct 18, 2014
 *      Author: daniel
 */

#include "plane.h"

using namespace regen;

Plane::Plane() = default;

Plane::Plane(const Vec3f &p, const Vec3f &n) {
	point = p;
	normal = n;
}

void Plane::set(const Vec3f &p0, const Vec3f &p1, const Vec3f &p2) {
	point = p0;
	normal = (p2 - p0).cross(p1 - p0);
	normal.normalize();
}

GLfloat Plane::distance(const Vec3f &p) {
	return normal.dot(point - p);
}
