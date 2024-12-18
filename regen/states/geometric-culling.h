/*
 * geometric-culling.h
 *
 *  Created on: Oct 17, 2014
 *      Author: daniel
 */

#ifndef GEOMETRIC_CULLING_H_
#define GEOMETRIC_CULLING_H_

#include <regen/states/state-node.h>

namespace regen {
	/**
	 * Baseclass for geometric culling approaches.
	 */
	class GeometricCulling : public StateNode {
	public:
		GeometricCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<MeshVector> &mesh,
				const ref_ptr<ModelTransformation> &transform);

		~GeometricCulling() override = default;

		void traverse(RenderState *rs) override;

		virtual bool isCulled() const = 0;
	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<MeshVector> mesh_;
		ref_ptr<ModelTransformation> transform_;
		Vec3f center_;
		Vec3f min_;
		Vec3f max_;
		GLuint tfStamp_;
		GLuint camStamp_;
		GLboolean isCulled_;
	};

	/**
	 * Frustum culling based on intersection of a sphere
	 * with the frustum.
	 */
	class SphereCulling : public GeometricCulling {
	public:
		SphereCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<MeshVector> &mesh,
				const ref_ptr<ModelTransformation> &transform);

		SphereCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<MeshVector> &mesh,
				const ref_ptr<ModelTransformation> &transform,
				GLfloat radius);

		// Override
		bool isCulled() const override;

	protected:
		GLfloat radius_;
	};

	/**
	 * Frustum culling based on intersection of a box with the frustum.
	 */
	class BoxCulling : public GeometricCulling {
	public:
		BoxCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<MeshVector> &mesh,
				const ref_ptr<ModelTransformation> &transform);

		BoxCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<MeshVector> &mesh,
				const ref_ptr<ModelTransformation> &transform,
				Vec3f *points);

		// Override
		bool isCulled() const override;

	protected:
		Vec3f points_[8];
	};

	/**
	 * Frustum culling based on intersection of an AABB
	 * (axis-aligned bounding box) with the frustum.
	 */
	class AABBCulling : public BoxCulling {
	public:
		AABBCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<MeshVector> &mesh,
				const ref_ptr<ModelTransformation> &transform);

	protected:
		void updateAABB();
	};

	/**
	 * Frustum culling based on intersection of an OBB
	 * (oriented bounding box) with the frustum.
	 */
	class OBBCulling : public BoxCulling {
	public:
		OBBCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<MeshVector> &mesh,
				const ref_ptr<ModelTransformation> &transform);

	protected:
		void updateOBB();
	};
}


#endif /* GEOMETRIC_CULLING_H_ */
