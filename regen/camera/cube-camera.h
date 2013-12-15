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
  class CubeCamera : public Camera
  {
  public:
    CubeCamera(
        const ref_ptr<Mesh> &mesh,
        const ref_ptr<Camera> &userCamera);

    void set_isCubeFaceVisible(GLenum face, GLboolean visible);

    // Override
    void enable(RenderState *rs);
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
