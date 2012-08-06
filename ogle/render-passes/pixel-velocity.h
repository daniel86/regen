/*
 * pixel-velocity.h
 *
 *  Created on: 18.07.2012
 *      Author: daniel
 */

#ifndef PIXEL_VELOCITY_H_
#define PIXEL_VELOCITY_H_

#include <render-pass.h>
#include <scene.h>
#include <mesh.h>

typedef enum {
  // object local coordinate space
  OBJECT_SPACE,
  // .. transformed to global coordinate system
  WORLD_SPACE,
  // .. and transformed by camera matrix
  EYE_SPACE,
  // .. and transformed by projection matrix
  SCREEN_SPACE
}CoordinateSpace;

/**
 * Generates velocity texture for camera perspective.
 * The calculation is done using transform feedback
 * of the position attribute in a configurable space
 * (world/eye/screen).
 */
class PixelVelocityPass : public RenderPass {
public:
  PixelVelocityPass(Scene* scene,
      CoordinateSpace velocitySpace=SCREEN_SPACE,
      bool useDepthBuffer=true,
      bool useSceneDepthBuffer=true,
      float depthBias=0.1f,
      float sizeScale=0.5f);
  ~PixelVelocityPass();

  // override
  virtual void render();

  /**
   * Resize buffer.
   */
  void resize();

  /**
   * Using custom buffer.
   */
  virtual bool usesSceneBuffer() { return false; }

  /**
   * The generated velocity texture.
   */
  ref_ptr<Texture2D>& velocityTex() { return velocityTex_; }

  /**
   * Add a mesh to this pass.
   * Only added meshes are rendered to the velocity buffer.
   */
  void addMesh(Mesh*);

protected:
  struct PixelVelocityMesh {
    Mesh *mesh;
    ref_ptr<VertexAttribute> posAtt0;
    ref_ptr<VertexAttribute> posAtt1;
  };

  Scene *scene_;
  ref_ptr<FrameBufferObject> velocityBuffer_;
  ref_ptr<Texture2D> velocityTex_;
  ref_ptr<Texture2D> depthTexture_;
  list<PixelVelocityMesh> meshes_;

  GLfloat updateInterval_;
  ref_ptr<EventCallable> projectionChangedCB_;

  Vec2ui bufferSize_;
  GLfloat sizeScale_;
  GLfloat depthBias_;
  bool useDepthBuffer_;
  bool useSceneDepthBuffer_;
  CoordinateSpace velocitySpace_;

  ref_ptr<Shader> velocityPassShader_;
  GLint posAttLoc0_, posAttLoc1_;
  GLuint depthTextureLoc_, matLoc_, viewportLoc_;
};

#endif /* PIXEL_VELOCITY_H_ */
