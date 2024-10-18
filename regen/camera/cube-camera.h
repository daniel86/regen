/*
 * cube-camera.h
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#ifndef CUBE_CAMERA_H_
#define CUBE_CAMERA_H_

#include <regen/camera/camera.h>
#include <regen/meshes/mesh-state.h>
#include <regen/gl-types/shader-input-container.h>

namespace regen {
	/**
	 * A layered camera looking at 6 cube faces.
	 */
	class CubeCamera : public OmniDirectionalCamera {
	public:
		/**
		 * @param mesh Defines cube center position.
		 * @param userCamera The user camera.
		 */
		CubeCamera(
				const ref_ptr<Mesh> &mesh,
				const ref_ptr<Camera> &userCamera);

		/**
		 * Toggle visibility for a cube face.
		 * @param face the face enumeration.
		 * @param visible if false face is ignored.
		 */
		void set_isCubeFaceVisible(GLenum face, GLboolean visible);

		// Override
		void enable(RenderState *rs) override;

	protected:
		ref_ptr<Camera> userCamera_;
		ref_ptr<ShaderInputMat4> modelMatrix_;
		ref_ptr<ShaderInput3f> pos_;
		GLboolean isCubeFaceVisible_[6];

		GLuint positionStamp_;
		GLuint matrixStamp_;

		void update();
	};
} // namespace

#endif /* CUBE_CAMERA_H_ */
