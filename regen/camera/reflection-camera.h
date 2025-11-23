#ifndef REGEN_REFLECTION_CAMERA_H_
#define REGEN_REFLECTION_CAMERA_H_

#include <regen/camera/camera.h>
#include <regen/objects/mesh.h>

namespace regen {
	/**
	 * Virtual camera that reflects another camera along
	 * an arbitrary plane.
	 */
	class ReflectionCamera : public Camera {
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
				uint32_t vertexIndex = 0,
				bool hasBackFace = false);

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
				bool hasBackFace = false);

		/**
		 * Update reflection camera.
		 */
		bool updateReflection();

	protected:
		ref_ptr<Camera> userCamera_;
		ref_ptr<ShaderInput> pos_;
		ref_ptr<ShaderInput> nor_;
		ref_ptr<ShaderInput> transform_;

		ref_ptr<State> cullState_;
		ref_ptr<Animation> reflectionUpdater_;
		Vec3f posWorld_;
		Vec3f norWorld_;
		uint32_t vertexIndex_;
		uint32_t projStamp_;
		uint32_t posStamp_;
		uint32_t norStamp_;
		uint32_t camPosStamp_;
		uint32_t camDirStamp_;
		uint32_t transformStamp_;
		bool cameraChanged_;
		bool isReflectorValid_;
		bool isFront_;
		bool hasMesh_;
		bool hasBackFace_;
		Mat4f reflectionMatrix_;
	};
} // namespace

#endif /* REGEN_REFLECTION_CAMERA_H_ */
