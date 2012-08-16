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
#include <ogle/states/light-state.h>
#include <ogle/states/camera.h>
#include <ogle/states/model-transformation.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/mesh-state.h>

#include <applications/glut-application.h>

class GlutRenderTree : public GlutApplication
{
public:
  GlutRenderTree(
      int argc=0,
      char** argv=NULL,
      const string &windowTitle="OGLE - OpenGL Engine",
      GLuint windowWidth=800,
      GLuint windowHeight=600,
      GLuint displayMode=GLUT_RGB,
      ref_ptr<RenderTree> renderTree=ref_ptr<RenderTree>(),
      ref_ptr<RenderState> renderState=ref_ptr<RenderState>(),
      GLboolean useDefaultCameraManipulator=GL_TRUE);

  virtual void render(GLdouble dt);
  virtual void postRender(GLdouble dt);
  virtual void mainLoop();

  ref_ptr<RenderTree>& renderTree();
  ref_ptr<RenderState>& renderState();

  ref_ptr<StateNode>& globalStates();
  ref_ptr<StateNode>& perspectivePass();
  ref_ptr<StateNode>& orthogonalPass();
  ref_ptr<PerspectiveCamera>& perspectiveCamera();
  ref_ptr<OrthoCamera>& orthogonalCamera();
  ref_ptr<LookAtCameraManipulator>& camManipulator();
  ref_ptr<Light>& defaultLight();

  void addRootNodeVBO(GLuint sizeMB=5);
  void addPerspectiveVBO(GLuint sizeMB=5);
  void addGUIVBO(GLuint sizeMB=5);

  void usePerspectivePass();
  void useGUIPass();

  void setBlitToScreen(
      ref_ptr<FrameBufferObject> fbo,
      GLenum attachment);

  void setClearScreenColor(const Vec4f &clearColor);
  void setClearScreenDepth();

  void setLight();
  void setLight(ref_ptr<Light> light);

  ref_ptr<FBOState> setRenderToTexture(
      GLuint width,
      GLuint height,
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

  ref_ptr<StateNode> addSkyBox(
      const string &imagePath,
      const string &fileExtension="png");

  void setShowFPS();

protected:
  ref_ptr<RenderTree> renderTree_;
  ref_ptr<RenderState> renderState_;

  ref_ptr<Light> defaultLight_;

  ///////////

  ref_ptr<StateNode> globalStates_;
  ref_ptr<ShaderInput1f> timeDelta_;

  ///////////

  ref_ptr<StateNode> perspectivePass_;
  ref_ptr<PerspectiveCamera> perspectiveCamera_;
  ref_ptr<LookAtCameraManipulator> camManipulator_;

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
