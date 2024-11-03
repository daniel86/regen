/*
 * light-camera.h
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#ifndef LIGHT_CAMERA_H_
#define LIGHT_CAMERA_H_

#include <regen/states/light-state.h>
#include <regen/camera/camera.h>

namespace regen {
	/**
	 * A Camera from Light's point of view.
	 * To cover the light multiple views may be used.
	 */
	class LightCamera : public OmniDirectionalCamera {
	public:
		/**
		 * Default constructor.
		 * @param light the Light instance.
		 * @param userCamera the user camera, maybe used to optimize precision.
		 * @param extends near and far plane.
		 * @param numLayer number of shadow map layer.
		 * @param splitWeight split weight for splitting the view frustum of the user camera.
		 */
		LightCamera(const ref_ptr<Light> &light,
					const ref_ptr<Camera> &userCamera,
					Vec2f extends = Vec2f(0.1f, 200.0f),
					GLuint numLayer = 1, GLdouble splitWeight = 0.9);

		/**
		 * @return A matrix used to transform world space points to
		 *          texture coordinates for shadow mapping.
		 */
		auto &lightMatrix() const { return lightMatrix_; }

		/**
		 * Discard specified cube faces.
		 */
		void set_isCubeFaceVisible(GLenum face, GLboolean visible);

		// Override
		void enable(RenderState *rs) override;

		GLboolean hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) override;

		GLboolean hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) override;

	protected:
		ref_ptr<Light> light_;
		ref_ptr<Camera> userCamera_;
		GLuint numLayer_;
		GLdouble splitWeight_;

		ref_ptr<ShaderInputMat4> lightMatrix_;
		std::vector<Frustum *> shadowFrusta_;

		GLuint lightPosStamp_;
		GLuint lightDirStamp_;
		GLuint lightRadiusStamp_;
		GLuint projectionStamp_;

		GLboolean isCubeFaceVisible_[6];

		void (LightCamera::*update_)();

		void updateDirectional();

		void updatePoint();

		void updateSpot();
	};
} // namespace

#endif /* LIGHT_CAMERA_H_ */
