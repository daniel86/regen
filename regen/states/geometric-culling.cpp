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
		  transform_(transform),
		  tfStamp_(0),
		  camStamp_(0),
		  isCulled_(false) {
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

void GeometricCulling::traverse(RenderState *rs) {
	auto tfStamp = transform_->get()->stamp();
	auto camStamp = camera_->stamp();
	if (tfStamp==tfStamp_ && camStamp==camStamp_) {
		// camera and TF did not change, no need to recompute
		if (!isCulled_) {
			StateNode::traverse(rs);
		}
	} else {
		if (isCulled()) {
			isCulled_ = true;
		} else {
			StateNode::traverse(rs);
			isCulled_ = false;
		}
		tfStamp_ = tfStamp;
		camStamp_ = camStamp;
	}
}

/**********************/
/* SphereCulling      */
/**********************/

SphereCulling::SphereCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform)
		: GeometricCulling(camera, mesh, transform) {
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

bool SphereCulling::isCulled() const {
	auto &tf = transform_->get()->getVertex(0).position();
	return !camera_->hasIntersectionWithSphere(tf + center_, radius_);
}

/**********************/
/* BoxCulling        */
/**********************/

BoxCulling::BoxCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform)
		: GeometricCulling(camera, mesh, transform) {
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

bool BoxCulling::isCulled() const {
	auto &tf = transform_->get()->getVertex(0).position();
	return !camera_->hasIntersectionWithBox(tf, points_);
}

/**********************/
/* AABBCulling        */
/**********************/

AABBCulling::AABBCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform)
		: BoxCulling(camera, mesh, transform) {
	// FIXME: need to update AABB if the mesh changes, or the rotation of transform changes
	updateAABB();
}

void AABBCulling::updateAABB() {
	const auto &tf = transform_->get()->getVertex(0);
	const Vec3f &a = tf.rotateVector(center_ + min_);
	const Vec3f &b = tf.rotateVector(center_ + max_);
	points_[0] = Vec3f(a.x, a.y, a.z);
	points_[1] = Vec3f(a.x, a.y, b.z);
	points_[2] = Vec3f(a.x, b.y, a.z);
	points_[3] = Vec3f(a.x, b.y, b.z);
	points_[4] = Vec3f(b.x, a.y, a.z);
	points_[5] = Vec3f(b.x, a.y, b.z);
	points_[6] = Vec3f(b.x, b.y, a.z);
	points_[7] = Vec3f(b.x, b.y, b.z);
}

/**********************/
/* OBBCulling        */
/**********************/

OBBCulling::OBBCulling(
		const ref_ptr<Camera> &camera,
		const ref_ptr<MeshVector> &mesh,
		const ref_ptr<ModelTransformation> &transform)
		: BoxCulling(camera, mesh, transform) {
	// FIXME: need to update AABB if the mesh changes, or the rotation of transform changes
	updateOBB();
}

void OBBCulling::updateOBB() {
	const auto &tf = transform_->get()->getVertex(0);
	const auto &a = center_ + min_;
	const auto &b = center_ + max_;
	points_[0] = tf.rotateVector(Vec3f(a.x, a.y, a.z));
	points_[1] = tf.rotateVector(Vec3f(a.x, a.y, b.z));
	points_[2] = tf.rotateVector(Vec3f(a.x, b.y, a.z));
	points_[3] = tf.rotateVector(Vec3f(a.x, b.y, b.z));
	points_[4] = tf.rotateVector(Vec3f(b.x, a.y, a.z));
	points_[5] = tf.rotateVector(Vec3f(b.x, a.y, b.z));
	points_[6] = tf.rotateVector(Vec3f(b.x, b.y, a.z));
	points_[7] = tf.rotateVector(Vec3f(b.x, b.y, b.z));
}
