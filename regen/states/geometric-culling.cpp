/*
 * geometric-culling.cpp
 *
 *  Created on: Oct 17, 2014
 *      Author: daniel
 */

#include <regen/states/state-node.h>
#include <regen/camera/camera.h>
#include <regen/meshes/mesh-state.h>

#include "geometric-culling.h"

using namespace regen;

GeometricCulling::GeometricCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform)
		: StateNode(),
		  camera_(camera),
		  mesh_(mesh),
		  transform_(transform) {
	// XXX: must be updated when mesh attributes updated!
	if (mesh->size() == 1) {
		ref_ptr<Mesh> &m = *mesh->begin();
		center_ = m->centerPosition();
		min_ = m->minPosition();
		max_ = m->maxPosition();
	} else {
		min_ = Vec3f(999999.0f);
		max_ = Vec3f(-999999.0f);

		Vec3f v;
		for (auto it = mesh->begin(); it != mesh->end(); ++it) {
			ref_ptr<Mesh> &m = *it;
			v = m->centerPosition() + m->minPosition();
			if (min_.x > v.x) min_.x = v.x;
			if (min_.y > v.y) min_.y = v.y;
			if (min_.z > v.z) min_.z = v.z;
			v = m->centerPosition() + m->maxPosition();
			if (max_.x < v.x) max_.x = v.x;
			if (max_.y < v.y) max_.y = v.y;
			if (max_.z < v.z) max_.z = v.z;
		}
		center_ = (max_ + min_) * 0.5;
		max_ -= center_;
		min_ -= center_;
	}
}

SphereCulling::SphereCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform)
		: GeometricCulling(camera, mesh, transform) {
	// XXX: must be updated when mesh attributes updated!
	radius_ = Vec2f(abs(min_.min()), max_.max()).max();
}

SphereCulling::SphereCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform,
		GLfloat radius)
		: GeometricCulling(camera, mesh, transform),
		  radius_(radius) {
}

void SphereCulling::traverse(RenderState *rs) {
	if (camera_->hasIntersectionWithSphere(transform_->get()->getVertex(0).position() + center_, radius_)) {
		StateNode::traverse(rs);
	}
}

BoxCulling::BoxCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform)
		: GeometricCulling(camera, mesh, transform) {
	// XXX: must be updated when mesh attributes updated!
	const Vec3f &a = center_ + min_;
	const Vec3f &b = center_ + max_;
	points_[0] = Vec3f(a.x, a.y, a.z);
	points_[1] = Vec3f(a.x, a.y, b.z);
	points_[2] = Vec3f(a.x, b.y, a.z);
	points_[3] = Vec3f(a.x, b.y, b.z);
	points_[4] = Vec3f(b.x, a.y, a.z);
	points_[5] = Vec3f(b.x, a.y, b.z);
	points_[6] = Vec3f(b.x, b.y, a.z);
	points_[7] = Vec3f(b.x, b.y, b.z);
}

BoxCulling::BoxCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform,
		Vec3f *points)
		: GeometricCulling(camera, mesh, transform) {
	for (int i = 0; i < 8; ++i)
		points_[i] = points[i];
}

void BoxCulling::traverse(RenderState *rs) {
	if (camera_->hasIntersectionWithBox(transform_->get()->getVertex(0).position(), points_)) {
		StateNode::traverse(rs);
	}
}
