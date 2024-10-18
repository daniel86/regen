/*
 * reflection-camera.h
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#ifndef REFLECTION_CAMERA_H_
#define REFLECTION_CAMERA_H_

#include <regen/camera/camera.h>
#include <regen/meshes/mesh-state.h>

namespace regen {
	/**
	 * Virtual camera that reflects another camera along
	 * an arbitrary plane.
	 */
	class ReflectionCamera : public OmniDirectionalCamera {
	public:
		/**
		 * @param cam The user camera to reflect.
		 * @param mesh The reflector plane (first vertex and normal taken).
		 * @param vertexIndex Index of mesh vertex and normal used for compute the plane equation
		 * @param hasBackFace if true handle reflector back faces.
		 */
		ReflectionCamera(
				const ref_ptr<Camera> &cam,
				const ref_ptr<Mesh> &mesh,
				GLuint vertexIndex = 0,
				GLboolean hasBackFace = GL_FALSE);

		/**
		 * @param userCamera The user camera to reflect.
		 * @param reflectorNormal Fixed reflector normal.
		 * @param reflectorPoint Fixed reflector center position.
		 * @param hasBackFace if true handle reflector back faces.
		 */
		ReflectionCamera(
				const ref_ptr<Camera> &userCamera,
				const Vec3f &reflectorNormal,
				const Vec3f &reflectorPoint,
				GLboolean hasBackFace = GL_FALSE);

		// Override
		void enable(RenderState *rs) override;

	protected:
		ref_ptr<Camera> userCamera_;
		ref_ptr<ShaderInput> pos_;
		ref_ptr<ShaderInput> nor_;
		ref_ptr<ShaderInput> transform_;
		ref_ptr<ShaderInput4f> clipPlane_;
		ref_ptr<State> cullState_;
		Vec3f posWorld_;
		Vec3f norWorld_;
		GLuint vertexIndex_;
		GLuint projStamp_;
		GLuint posStamp_;
		GLuint norStamp_;
		GLuint camPosStamp_;
		GLuint camDirStamp_;
		GLuint transformStamp_;
		GLboolean cameraChanged_;
		GLboolean isReflectorValid_;
		GLboolean isFront_;
		GLboolean hasMesh_;
		Mat4f reflectionMatrix_;

		void updateReflection();
	};
} // namespace

#endif /* REFLECTION_CAMERA_H_ */
