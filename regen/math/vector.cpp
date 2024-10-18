/*
 * vector.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "vector.h"

namespace regen {

	Vec4f calculateTangent(Vec3f *vertices, Vec2f *texco, const Vec3f &normal) {
		Vec3f tangent, binormal;
		// calculate vertex and uv edges
		Vec3f edge1 = vertices[1] - vertices[0];
		edge1.normalize();
		Vec3f edge2 = vertices[2] - vertices[0];
		edge2.normalize();
		Vec2f texEdge1 = texco[1] - texco[0];
		texEdge1.normalize();
		Vec2f texEdge2 = texco[2] - texco[0];
		texEdge2.normalize();
		GLfloat det = texEdge1.x * texEdge2.y - texEdge2.x * texEdge1.y;

		if (math::isApprox(det, 0.0)) {
			tangent = Vec3f(1.0, 0.0, 0.0);
			binormal = Vec3f(0.0, 1.0, 0.0);
		} else {
			det = 1.0f / det;
			tangent = Vec3f(
					(texEdge2.y * edge1.x - texEdge1.y * edge2.x),
					(texEdge2.y * edge1.y - texEdge1.y * edge2.y),
					(texEdge2.y * edge1.z - texEdge1.y * edge2.z)
			) * det;
			binormal = Vec3f(
					(-texEdge2.x * edge1.x + texEdge1.x * edge2.x),
					(-texEdge2.x * edge1.y + texEdge1.x * edge2.y),
					(-texEdge2.x * edge1.z + texEdge1.x * edge2.z)
			) * det;
		}

		// Gram-Schmidt orthogonalize tangent with normal.
		tangent -= normal * normal.dot(tangent);
		tangent.normalize();

		Vec3f bitangent = normal.cross(tangent);
		// Calculate the handedness of the local tangent space.
		GLfloat handedness = (bitangent.dot(binormal) < 0.0f) ? 1.0f : -1.0f;

		return {tangent.x, tangent.y, tangent.z, handedness};
	}

}
