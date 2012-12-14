/*
 * glut-render-tree.h
 *
 *  Created on: 10.08.2012
 *      Author: daniel
 */

#ifndef GLUT_RENDER_TREE_H_
#define GLUT_RENDER_TREE_H_

#include <ogle/algebra/vector.h>
#include <ogle/animations/animation.h>
#include <ogle/animations/camera-manipulator.h>
#include <ogle/models/text.h>
#include <ogle/render-tree/render-tree.h>
#include <ogle/render-tree/picker.h>
#include <ogle/states/light-state.h>
#include <ogle/states/camera.h>
#include <ogle/states/model-transformation.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/mesh-state.h>
#include <ogle/states/material-state.h>
#include <ogle/states/transparency-state.h>
#include <ogle/models/sky-box.h>

#include <applications/ogle-render-tree.h>

class TestRenderTree : public OGLERenderTree
{
public:
  /**
   * Resize event.
   */
  static GLuint RESIZE_EVENT;

  TestRenderTree(GLuint width=800, GLuint height=600);

  void setRenderState(ref_ptr<RenderState> &renderState);

  virtual void initTree();

  virtual void setWindowSize(GLuint w, GLuint h);
  GLuint windowWidth() const {
    return viewport_->getVertex2f(0).x;
  }
  GLuint windowHeight() const {
    return viewport_->getVertex2f(0).y;
  }

  ref_ptr<Texture>& sceneTexture() {
    return sceneTexture_;
  }
  ref_ptr<Texture>& sceneDepthTexture() {
    return sceneDepthTexture_;
  }

  virtual void setMousePosition(GLuint x, GLuint y);

  virtual void render(GLdouble dt);
  virtual void postRender(GLdouble dt);

  ref_ptr<RenderTree>& renderTree();
  ref_ptr<RenderState>& renderState();

  ref_ptr<StateNode>& globalStates();
  ref_ptr<StateNode>& perspectivePass();
  ref_ptr<StateNode>& transparencyPass();
  ref_ptr<StateNode>& guiPass();

  ref_ptr<PerspectiveCamera>& perspectiveCamera();
  ref_ptr<OrthoCamera>& guiCamera();

  ref_ptr<MeshState>& orthoQuad();

  ref_ptr<DirectionalLight>& defaultLight();

  void usePerspectivePass();
  ref_ptr<Picker> usePicking();
  void useGUIPass();

  void setBlitToScreen(
      ref_ptr<FrameBufferObject> fbo,
      GLenum attachment);

  void setClearScreenColor(const Vec4f &clearColor);
  void setClearScreenDepth();

  ref_ptr<DirectionalLight>& setLight();
  void setLight(ref_ptr<Light> light);

  void setTransparencyMode(TransparencyMode transparencyMode);

  ref_ptr<FBOState> setRenderToTexture(
      GLfloat windowWidthScale,
      GLfloat windowHeightScale,
      GLenum colorAttachmentFormat,
      GLenum depthAttachmentFormat,
      GLboolean clearDepthBuffer,
      GLboolean clearColorBuffer,
      const Vec4f &clearColor);
  ref_ptr<FBOState> setRenderToTexture(
      ref_ptr<FrameBufferObject> fbo,
      GLboolean clearDepthBuffer,
      GLboolean clearColorBuffer,
      const Vec4f &clearColor);

  ref_ptr<StateNode> addMesh(
      ref_ptr<MeshState> mesh,
      ref_ptr<ModelTransformationState> modelTransformation=ref_ptr<ModelTransformationState>(),
      ref_ptr<Material> material=ref_ptr<Material>(),
      const string &shaderKey="mesh",
      GLboolean useAlpha=GL_FALSE);

  ref_ptr<StateNode> addGUIElement(
      ref_ptr<MeshState> mesh,
      ref_ptr<ModelTransformationState> modelTransformation=ref_ptr<ModelTransformationState>(),
      ref_ptr<Material> material=ref_ptr<Material>(),
      const string &shaderKey="gui");

  ref_ptr<StateNode> addDynamicSky();
  ref_ptr<StateNode> addSkyBox(ref_ptr<TextureCube> &cubeMap);
  ref_ptr<StateNode> addSkyBox(
      const string &imagePath,
      GLenum internalFormat=GL_NONE,
      GLboolean flipBackFace=GL_FALSE);
  ref_ptr<SkyBox>& skyBox() { return skyBox_; }

  void setShowFPS();

  void updateProjection();

  void set_nearDistance(GLfloat near);
  void set_farDistance(GLfloat far);
  void set_fieldOfView(GLfloat fov);

  ref_ptr<FBOState> createRenderTarget(
      GLfloat windowWidthScale,
      GLfloat windowHeightScale,
      GLenum depthAttachmentFormat,
      GLboolean clearDepthBuffer,
      GLboolean clearColorBuffer,
      const Vec4f &clearColor);

protected:
  ref_ptr<ShaderInput2f> viewport_;
  ref_ptr<RenderTree> renderTree_;
  ref_ptr<RenderState> renderState_;

  GLfloat fov_;
  GLfloat near_;
  GLfloat far_;

  ref_ptr<DirectionalLight> defaultLight_;

  ref_ptr<FrameBufferObject> sceneFBO_;
  ref_ptr<Texture> sceneTexture_;
  ref_ptr<Texture> sceneDepthTexture_;

  ///////////

  ref_ptr<StateNode> globalStates_;
  ref_ptr<ShaderInput1f> timeDelta_;
  ref_ptr<ShaderInput2f> mousePosition_;

  ///////////

  ref_ptr<StateNode> lightNode_;

  ref_ptr<StateNode> perspectivePass_;
  ref_ptr<PerspectiveCamera> perspectiveCamera_;

  ref_ptr<StateNode> backgroundPass_;
  ref_ptr<SkyBox> skyBox_;

  ref_ptr<StateNode> transparencyPass_;
  ref_ptr<TransparencyState> transparencyState_;
  ref_ptr<StateNode> transparencyAccumulation_;

  ///////////
  ref_ptr<MeshState> orthoQuad_;

  ///////////

  ref_ptr<StateNode> guiPass_;
  ref_ptr<OrthoCamera> guiCamera_;
  ref_ptr<Text> fpsText_;
  ref_ptr<Animation> updateFPS_;

  ///////////

  ref_ptr<StateNode> addMesh(
      ref_ptr<StateNode> parent,
      ref_ptr<MeshState> mesh,
      ref_ptr<ModelTransformationState> modelTransformation,
      ref_ptr<Material> material,
      const string &shaderKey);
};

#endif /* GLUT_RENDER_TREE_H_ */
