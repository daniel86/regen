/*
 * camera.h
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <regen/states/state.h>
#include <regen/utility/ref-ptr.h>
#include <regen/math/matrix.h>
#include <regen/math/frustum.h>
#include <regen/meshes/mesh-state.h>
#include <regen/states/model-transformation.h>
#include <regen/gl-types/shader-input-container.h>

namespace regen {
  /**
   * \brief Camera with perspective projection.
   */
  class Camera : public HasInputState
  {
  public:
    Camera(GLboolean initializeMatrices=GL_TRUE);

    /**
     * @return the camera position.
     */
    const ref_ptr<ShaderInput3f>& position() const;
    /**
     * @return the camera direction.
     */
    const ref_ptr<ShaderInput3f>& direction() const;

    /**
     * @return the camera velocity.
     */
    const ref_ptr<ShaderInput3f>& velocity() const;

    /**
     * Transforms world-space to view-space.
     * @return the view matrix.
     */
    const ref_ptr<ShaderInputMat4>& view() const;
    /**
     * Transforms view-space to world-space.
     * @return the inverse view matrix.
     */
    const ref_ptr<ShaderInputMat4>& viewInverse() const;

    /**
     * Transforms view-space to screen-space.
     * @return the projection matrix.
     */
    const ref_ptr<ShaderInputMat4>& projection() const;
    /**
     * Transforms screen-space to view-space.
     * @return the inverse projection matrix.
     */
    const ref_ptr<ShaderInputMat4>& projectionInverse() const;

    /**
     * Transforms world-space to screen-space.
     * @return the view-projection matrix.
     */
    const ref_ptr<ShaderInputMat4>& viewProjection() const;
    /**
     * Transforms screen-space to world-space.
     * @return the inverse view-projection matrix.
     */
    const ref_ptr<ShaderInputMat4>& viewProjectionInverse() const;

    /**
     * @return the camera view Frustum.
     */
    const ref_ptr<Frustum>& frustum() const;
    /**
     * Update frustum and projection matrix.
     */
    void updateFrustum(
        const Vec2i viewport,
        GLfloat fov, GLfloat near, GLfloat far);

    /**
     * @param useAudio true if this camera is the OpenAL audio listener.
     */
    void set_isAudioListener(GLboolean useAudio);
    /**
     * @return true if this camera is the OpenAL audio listener.
     */
    GLboolean isAudioListener() const;

  protected:
    ref_ptr<ShaderInputContainer> inputs_;
    GLboolean isAudioListener_;

    ref_ptr<ShaderInput3f> position_;
    ref_ptr<ShaderInput3f> direction_;
    ref_ptr<ShaderInput3f> vel_;
    ref_ptr<Frustum> frustum_;

    ref_ptr<ShaderInputMat4> view_;
    ref_ptr<ShaderInputMat4> viewInv_;
    ref_ptr<ShaderInputMat4> proj_;
    ref_ptr<ShaderInputMat4> projInv_;
    ref_ptr<ShaderInputMat4> viewproj_;
    ref_ptr<ShaderInputMat4> viewprojInv_;
  };

  /**
   * Virtual camera that reflects another camera along
   * an arbitrary plane.
   */
  class ReflectionCamera : public Camera
  {
  public:
    /**
     * @param cam The user camera to reflect.
     * @param mesh The reflector plane (first vertex and normal taken).
     * @param vertexIndex Index of mesh vertex and normal used for compute the plane equation
     */
    ReflectionCamera(const ref_ptr<Camera> &cam, const ref_ptr<Mesh> &mesh, GLuint vertexIndex=0);
    ReflectionCamera(
        const ref_ptr<Camera> &userCamera,
        const Vec3f &reflectorNormal,
        const Vec3f &reflectorPoint);

    // Override
    virtual void enable(RenderState *rs);

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

  class Light;
  /**
   * A Camera from Light's point of view.
   * To cover the light multiple views may be used.
   */
  class LightCamera : public Camera {
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
                Vec2f extends=Vec2f(0.1f,200.0f),
                GLuint numLayer=1, GLdouble splitWeight=0.9);

    /**
     * @return A matrix used to transform world space points to
     *          texture coordinates for shadow mapping.
     */
    const ref_ptr<ShaderInputMat4>& lightMatrix() const;
    /**
     * @return Light far planes.
     */
    const ref_ptr<ShaderInput1f>& lightFar() const;
    /**
     * @return Light near planes.
     */
    const ref_ptr<ShaderInput1f>& lightNear() const;

    /**
     * Discard specified cube faces.
     */
    void set_isCubeFaceVisible(GLenum face, GLboolean visible);

    // Override
    void enable(RenderState *rs);

  protected:
    ref_ptr<Light> light_;
    ref_ptr<Camera> userCamera_;
    GLuint numLayer_;
    GLdouble splitWeight_;

    ref_ptr<ShaderInput1f> lightFar_;
    ref_ptr<ShaderInput1f> lightNear_;
    ref_ptr<ShaderInputMat4> lightMatrix_;
    vector<Frustum*> shadowFrusta_;

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

#endif /* _CAMERA_H_ */
