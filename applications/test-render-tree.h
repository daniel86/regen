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
    return windowSize_.x;
  }
  GLuint windowHeight() const {
    return windowSize_.y;
  }

  virtual void setMousePosition(GLuint x, GLuint y);

  virtual void render(GLdouble dt);
  virtual void postRender(GLdouble dt);

  ref_ptr<RenderTree>& renderTree();
  ref_ptr<RenderState>& renderState();

  ref_ptr<StateNode>& globalStates();
  ref_ptr<StateNode>& perspectivePass();
  ref_ptr<StateNode>& guiPass();
  ref_ptr<StateNode>& orthoPass();

  ref_ptr<PerspectiveCamera>& perspectiveCamera();
  ref_ptr<OrthoCamera>& guiCamera();
  ref_ptr<OrthoCamera>& orthoCamera();

  ref_ptr<MeshState>& orthoQuad();

  ref_ptr<Light>& defaultLight();

  void addRootNodeVBO(GLuint sizeMB=5);
  void addPerspectiveVBO(GLuint sizeMB=5);
  void addGUIVBO(GLuint sizeMB=5);

  void usePerspectivePass();
  ref_ptr<Picker> usePicking();
  void useGUIPass();
  void useOrthoPasses();
  void useOrthoPassesCustomTarget();

  void setBlitToScreen(
      ref_ptr<FrameBufferObject> fbo,
      GLenum attachment);

  void setClearScreenColor(const Vec4f &clearColor);
  void setClearScreenDepth();

  ref_ptr<Light>& setLight();
  void setLight(ref_ptr<Light> light);

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
      GLboolean generateShader=true,
      GLboolean generateVBO=true);

  ref_ptr<StateNode> addGUIElement(
      ref_ptr<MeshState> mesh,
      ref_ptr<ModelTransformationState> modelTransformation=ref_ptr<ModelTransformationState>(),
      ref_ptr<Material> material=ref_ptr<Material>(),
      GLboolean generateShader=true,
      GLboolean generateVBO=true);

  ref_ptr<StateNode> addDummyOrthoPass();
  ref_ptr<StateNode> addOrthoPass(ref_ptr<State> orthoPass, GLboolean pingPong=GL_TRUE);

  /*
  ref_ptr<StateNode> addAntiAliasingPass(
      FXAA::Config &cfg,
      ref_ptr<State> state=ref_ptr<State>());
  ref_ptr<FBOState> addBlurPass(
      const BlurConfig &blurCfg,
      GLdouble winScaleX=0.25,
      GLdouble winScaleY=0.25,
      ref_ptr<State> state=ref_ptr<State>());
  ref_ptr<StateNode> addTonemapPass(
      const TonemapConfig &tonemapCfg,
      ref_ptr<Texture> blurTexture,
      GLdouble winScaleX=0.25,
      GLdouble winScaleY=0.25,
      ref_ptr<State> state=ref_ptr<State>());
      */

  ref_ptr<StateNode> addSkyBox(
      ref_ptr<Texture> &skyTex);
  ref_ptr<StateNode> addSkyBox(
      const string &imagePath,
      GLenum internalFormat=GL_NONE,
      GLboolean flipBackFace=GL_FALSE);

  void setShowFPS();

  void updateProjection();

  void set_nearDistance(GLfloat near);
  void set_farDistance(GLfloat far);
  void set_fieldOfView(GLfloat fov);

  ref_ptr<FBOState> createRenderTarget(
      GLfloat windowWidthScale,
      GLfloat windowHeightScale,
      GLenum colorAttachmentFormat,
      GLenum depthAttachmentFormat,
      GLboolean clearDepthBuffer,
      GLboolean clearColorBuffer,
      const Vec4f &clearColor);

protected:

  Vec2ui windowSize_;
  ref_ptr<RenderTree> renderTree_;
  ref_ptr<RenderState> renderState_;

  GLfloat fov_;
  GLfloat near_, far_;

  GLuint numOrthoPasses_;

  ref_ptr<Light> defaultLight_;

  ref_ptr<FrameBufferObject> sceneFBO_;
  ref_ptr<Texture> sceneTexture_;

  ///////////

  ref_ptr<StateNode> globalStates_;
  ref_ptr<ShaderInput1f> timeDelta_;
  ref_ptr<ShaderInput2f> mousePosition_;

  ///////////

  ref_ptr<StateNode> perspectivePass_;
  ref_ptr<PerspectiveCamera> perspectiveCamera_;
  ref_ptr<SkyBox> skyBox_;

  ///////////
  ref_ptr<StateNode> orthoPasses_;
  ref_ptr<StateNode> orthoPassesCustomTarget_;
  ref_ptr<OrthoCamera> orthoCamera_;
  ref_ptr<MeshState> orthoQuad_;
  ref_ptr<State> lastOrthoPass_;

  /*
  ref_ptr<Shader> createShader(
      ref_ptr<StateNode> &node,
      ShaderFunctions &fs,
      ShaderFunctions &vs);
  ref_ptr<State> createBlurState(
      const string &name,
      const ConvolutionKernel &kernel,
      ref_ptr<Texture> &blurredTexture,
      ref_ptr<StateNode> &blurNode);
      */

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
      GLboolean generateShader,
      GLboolean generateVBO);
};

#endif /* GLUT_RENDER_TREE_H_ */
