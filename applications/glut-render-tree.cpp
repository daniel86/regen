/*
 * glut-render-tree.cpp
 *
 *  Created on: 10.08.2012
 *      Author: daniel
 */

#ifndef GLUT_RENDER_TREE_CPP_
#define GLUT_RENDER_TREE_CPP_

#include "glut-render-tree.h"

#include <ogle/states/blit-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/font/font-manager.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/gl-error.h>
#include <ogle/textures/cube-image-texture.h>
#include <ogle/models/sky-box.h>

static void debugState(State *s, const string suffix)
{
  if(s==NULL) { return; }
  DEBUG_LOG(suffix << s->name());
  for(list< ref_ptr<State> >::iterator
      it=s->joined().begin(); it!=s->joined().end(); ++it)
  {
    debugState(it->get(), suffix + "_");
  }
}
static void debugTree(StateNode *n, const string suffix)
{
  ref_ptr<State> &s = n->state();
  debugState(s.get(), suffix);
  for(list< ref_ptr<StateNode> >::iterator
      it=n->childs().begin(); it!=n->childs().end(); ++it)
  {
    debugTree(it->get(), suffix+"  ");
  }
}

class UpdateFPS : public Animation
{
public:
  UpdateFPS(ref_ptr<Text> fpsText)
  : Animation(),
    fpsText_(fpsText),
    numFrames_(0),
    sumDtMiliseconds_(0.0f)
  {

  }
  virtual void animate(GLdouble milliSeconds)
  {
  }
  virtual void updateGraphics(GLdouble dt)
  {
    numFrames_ += 1;
    sumDtMiliseconds_ += dt;

    if (sumDtMiliseconds_ > 1000.0) {
      fps_ = (int) (numFrames_*1000.0/sumDtMiliseconds_);
      sumDtMiliseconds_ = 0;
      numFrames_ = 0;

      wstringstream ss;
      ss << fps_ << " FPS";
      fpsText_->set_value(ss.str());
    }
  }
private:
  ref_ptr<Text> fpsText_;
  unsigned int numFrames_;
  int fps_;
  double sumDtMiliseconds_;
};
class CameraMotionEvent : public EventCallable
{
public:
  CameraMotionEvent(
      ref_ptr<LookAtCameraManipulator> camManipulator,
      GLboolean &buttonPressed)
  : EventCallable(),
    camManipulator_(camManipulator),
    buttonPressed_(buttonPressed)
  {}
  virtual void call(EventObject *evObject, void *data)
  {
    GlutApplication::MouseMotionEvent *ev =
        (GlutApplication::MouseMotionEvent*)data;
    if(buttonPressed_) {
      camManipulator_->set_height(
          camManipulator_->height() + ((float)ev->dy)*0.02f, ev->dt );
      camManipulator_->setStepLength( ((float)ev->dx)*0.001f, ev->dt );
    }
  }
  ref_ptr<LookAtCameraManipulator> camManipulator_;
  const GLboolean &buttonPressed_;
};
class CameraButtonEvent : public EventCallable
{
public:
  CameraButtonEvent(ref_ptr<LookAtCameraManipulator> camManipulator)
  : EventCallable(),
    camManipulator_(camManipulator)
  {}
  virtual void call(EventObject *evObject, void *data)
  {
    GlutApplication::ButtonEvent *ev =
        (GlutApplication::ButtonEvent*)data;

    if(ev->button == 0) {
      buttonPressed_ = ev->pressed;
      if(ev->pressed) {
        camManipulator_->setStepLength( 0.0f );
      }
      } else if (ev->button == 4 && !ev->pressed) {
        camManipulator_->set_radius( camManipulator_->radius()+0.1f );
      } else if (ev->button == 3 && !ev->pressed) {
        camManipulator_->set_radius( camManipulator_->radius()-0.1f );
    }
  }
  ref_ptr<LookAtCameraManipulator> camManipulator_;
  GLboolean buttonPressed_;
};

GlutRenderTree::GlutRenderTree(
    int argc, char** argv,
    const string &windowTitle,
    GLuint windowWidth,
    GLuint windowHeight,
    GLuint displayMode,
    ref_ptr<RenderTree> renderTree,
    ref_ptr<RenderState> renderState,
    GLboolean useDefaultCameraManipulator)
: GlutApplication(argc, argv, windowTitle, windowWidth, windowHeight, displayMode),
  renderTree_(renderTree),
  renderState_(renderState)
{
  if(renderState_.get()==NULL) {
    renderState_ = ref_ptr<RenderState>::manage(new RenderState);
  }
  if(renderTree_.get()==NULL) {
    renderTree_ = ref_ptr<RenderTree>::manage(new RenderTree);
  }
  globalStates_ = renderTree_->rootNode();
  timeDelta_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("deltaT"));
  timeDelta_->setUniformData(0.0f);
  globalStates_->state()->joinShaderInput(ref_ptr<ShaderInput>::cast(timeDelta_));

  perspectiveCamera_ = ref_ptr<PerspectiveCamera>::manage(new PerspectiveCamera);
  perspectivePass_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(perspectiveCamera_)));

  // TODO: disable depth test for GUI/ortho
  guiCamera_ = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  guiCamera_->updateProjection(800.0f,600.0f);
  guiPass_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(guiCamera_)));
  ref_ptr<State> alphaBlending = ref_ptr<State>::manage(new BlendState);
  guiPass_->state()->joinStates(alphaBlending);

  if(useDefaultCameraManipulator) {
    camManipulator_ = ref_ptr<LookAtCameraManipulator>::manage(
        new LookAtCameraManipulator(perspectiveCamera_, 10) );
    camManipulator_->set_height( 1.5f );
    camManipulator_->set_lookAt( Vec3f(0.0f, 0.0f, 0.0f) );
    camManipulator_->set_radius( 5.0f );
    camManipulator_->set_degree( 0.0f );
    camManipulator_->setStepLength( M_PI*0.01 );
    AnimationManager::get().addAnimation( ref_ptr<Animation>::cast(camManipulator_) );

    ref_ptr<CameraButtonEvent> buttonCallable = ref_ptr<CameraButtonEvent>::manage(
        new CameraButtonEvent(camManipulator_));
    ref_ptr<CameraMotionEvent> mouseMotionCallable = ref_ptr<CameraMotionEvent>::manage(
        new CameraMotionEvent(camManipulator_, buttonCallable->buttonPressed_));
    connect(BUTTON_EVENT, ref_ptr<EventCallable>::cast(buttonCallable));
    connect(MOUSE_MOTION_EVENT, ref_ptr<EventCallable>::cast(mouseMotionCallable));
  }

  defaultLight_ = ref_ptr<Light>::manage(new Light);
}

ref_ptr<Light>& GlutRenderTree::defaultLight()
{
  return defaultLight_;
}
ref_ptr<LookAtCameraManipulator>& GlutRenderTree::camManipulator()
{
  return camManipulator_;
}
ref_ptr<StateNode>& GlutRenderTree::globalStates()
{
  return globalStates_;
}
ref_ptr<StateNode>& GlutRenderTree::perspectivePass()
{
  return perspectivePass_;
}
ref_ptr<StateNode>& GlutRenderTree::orthogonalPass()
{
  return guiPass_;
}
ref_ptr<PerspectiveCamera>& GlutRenderTree::perspectiveCamera()
{
  return perspectiveCamera_;
}
ref_ptr<OrthoCamera>& GlutRenderTree::orthogonalCamera()
{
  return guiCamera_;
}
ref_ptr<RenderTree>& GlutRenderTree::renderTree()
{
  return renderTree_;
}
ref_ptr<RenderState>& GlutRenderTree::renderState()
{
  return renderState_;
}

void GlutRenderTree::addRootNodeVBO(GLuint sizeMB)
{
  renderTree_->addVBONode(globalStates_, sizeMB);
}
void GlutRenderTree::addPerspectiveVBO(GLuint sizeMB)
{
  if(perspectivePass_->parent().get() == NULL) {
    usePerspectivePass();
  }
  renderTree_->addVBONode(perspectivePass_, sizeMB);
}
void GlutRenderTree::addGUIVBO(GLuint sizeMB)
{
  if(guiPass_->parent().get() == NULL) {
    useGUIPass();
  }
  renderTree_->addVBONode(guiPass_, sizeMB);
}

void GlutRenderTree::usePerspectivePass()
{
  renderTree_->addChild(globalStates_, perspectivePass_, false);
}
void GlutRenderTree::useGUIPass()
{
  renderTree_->addChild(globalStates_, guiPass_, false);
}
void GlutRenderTree::setBlitToScreen(
    ref_ptr<FrameBufferObject> fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, attachment));
  ref_ptr<StateNode> blitNode = ref_ptr<StateNode>::manage(
      new StateNode(blitState));
  globalStates_->addChild(blitNode);
}
void GlutRenderTree::setClearScreenColor(const Vec4f &clearColor)
{
  ref_ptr<ClearColorState> clearScreenColor =
      ref_ptr<ClearColorState>::manage(new ClearColorState);
  ClearColorData clearData;
  clearData.clearColor = clearColor;
  clearData.colorAttachment = GL_FRONT;
  clearScreenColor->data.push_back(clearData);
  globalStates_->state()->addEnabler(ref_ptr<Callable>::cast(clearScreenColor));
}
void GlutRenderTree::setClearScreenDepth()
{
  ref_ptr<Callable> clearScreenDepth =
      ref_ptr<Callable>::manage(new ClearDepthState);
  globalStates_->state()->addEnabler(clearScreenDepth);
}

ref_ptr<Light>& GlutRenderTree::setLight()
{
  setLight(defaultLight_);
  return defaultLight_;
}
void GlutRenderTree::setLight(ref_ptr<Light> light)
{
  perspectivePass_->state()->joinStates(ref_ptr<State>::cast(light));
}

ref_ptr<FBOState> GlutRenderTree::setRenderToTexture(
    GLuint width,
    GLuint height,
    GLenum colorAttachmentFormat,
    GLenum depthAttachmentFormat,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(
          width, height,
          colorAttachmentFormat,
          depthAttachmentFormat));
  ref_ptr<Texture> colorBuffer = fbo->addTexture(1);
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
  if(clearDepthBuffer) {
    fboState->setClearDepth();
  }
  if(clearColorBuffer) {
    ClearColorData colorData;
    colorData.clearColor = clearColor;
    colorData.colorAttachment = colorAttachment;
    fboState->setClearColor(colorData);
  }

  // call glDrawBuffer for GL_COLOR_ATTACHMENT0
  ref_ptr<ShaderFragmentOutput> output =
      ref_ptr<ShaderFragmentOutput>::manage(new DefaultFragmentOutput);
  output->set_colorAttachment(colorAttachment);
  fboState->addDrawBuffer(output);

  globalStates_->state()->joinStates(ref_ptr<State>::cast(fboState));
  return fboState;
}
ref_ptr<FBOState> GlutRenderTree::setRenderToTexture(
    ref_ptr<FrameBufferObject> fbo,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  GLenum colorAttachment = GL_COLOR_ATTACHMENT0;
  if(clearDepthBuffer) {
    fboState->setClearDepth();
  }
  if(clearColorBuffer) {
    ClearColorData colorData;
    colorData.clearColor = clearColor;
    colorData.colorAttachment = colorAttachment;
    fboState->setClearColor(colorData);
  }

  // call glDrawBuffer for GL_COLOR_ATTACHMENT0
  ref_ptr<ShaderFragmentOutput> output =
      ref_ptr<ShaderFragmentOutput>::manage(new DefaultFragmentOutput);
  output->set_colorAttachment(colorAttachment);
  fboState->addDrawBuffer(output);

  globalStates_->state()->joinStates(ref_ptr<State>::cast(fboState));

  return fboState;
}

ref_ptr<StateNode> GlutRenderTree::addMesh(
    ref_ptr<MeshState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    GLboolean generateShader,
    GLboolean generateVBO)
{
  if(perspectivePass_->parent().get() == NULL) {
    usePerspectivePass();
  }
  return addMesh(
      perspectivePass_,
      mesh,
      modelTransformation,
      material,
      generateShader,
      generateVBO);
}

ref_ptr<StateNode> GlutRenderTree::addGUIElement(
    ref_ptr<MeshState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    GLboolean generateShader,
    GLboolean generateVBO)
{
  if(guiPass_->parent().get() == NULL) {
    useGUIPass();
  }
  return addMesh(
      guiPass_,
      mesh,
      modelTransformation,
      material,
      generateShader,
      generateVBO);
}

ref_ptr<StateNode> GlutRenderTree::addMesh(
    ref_ptr<StateNode> parent,
    ref_ptr<MeshState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    GLboolean generateShader,
    GLboolean generateVBO)
{
  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  ref_ptr<ShaderState> shaderState =
      ref_ptr<ShaderState>::manage(new ShaderState);
  ref_ptr<StateNode> materialNode, shaderNode, modeltransformNode;

  ref_ptr<StateNode> *root = &meshNode;
  if(generateShader) {
    shaderNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(shaderState)));
    shaderNode->addChild(*root);
    (*root)->set_parent(shaderNode);
    root = &shaderNode;
  }
  if(modelTransformation.get()!=NULL) {
    modeltransformNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(modelTransformation)));
    modeltransformNode->addChild(*root);
    (*root)->set_parent(modeltransformNode);
    root = &modeltransformNode;
  }
  if(material.get()!=NULL) {
    materialNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(material)));
    materialNode->addChild(*root);
    (*root)->set_parent(materialNode);
    root = &materialNode;
  }

  renderTree_->addChild(parent, *root, generateVBO);

  if(generateShader) {
    ref_ptr<Shader> shader = renderTree_->generateShader(*meshNode.get());
    shaderState->set_shader(shader);
  }

  return meshNode;
}

ref_ptr<StateNode> GlutRenderTree::addSkyBox(
    const string &imagePath,
    const string &fileExtension)
{
  // TODO: update far... ehhm and other stuff tetue size and so on
  ref_ptr<Texture> skyTex = ref_ptr<Texture>::manage(
      new CubeImageTexture(imagePath, fileExtension));
  ref_ptr<MeshState> skyBox = ref_ptr<MeshState>::manage(
      new SkyBox(ref_ptr<Camera>::cast(perspectiveCamera_), skyTex, 200.0f));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->set_shading(Material::NO_SHADING);
  material->setConstantUniforms(GL_TRUE);

  return addMesh(skyBox, ref_ptr<ModelTransformationState>(), material);
}

void GlutRenderTree::setShowFPS()
{
  const Vec4f fpsColor = Vec4f(1.0f);
  const Vec4f fpsBackgroundColor = Vec4f(0.0f, 0.0f, 0.0f, 0.5f);
  FreeTypeFont& font = FontManager::get().getFont(
                  "res/fonts/arial.ttf",
                  12, // font size in pixel
                  fpsColor, //
                  fpsBackgroundColor,
                  0, // glyph rotation in degree
                  GL_LINEAR, // filter mode
                  false, // use mipmaps?
                  96); // dpi

  fpsText_ = ref_ptr<Text>::manage(new Text(font, 12.0, true, true));
  fpsText_->set_value(L"0 FPS");

  // TODO: modelMat needed for ortho objects ??
  ref_ptr<ModelTransformationState> modelTransformation =
      ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
  modelTransformation->translate( Vec3f( 2.0, 18.0, 0.0 ), 0.0f );
  addGUIElement(ref_ptr<MeshState>::cast(fpsText_), modelTransformation);

  updateFPS_ = ref_ptr<Animation>::manage(new UpdateFPS(fpsText_));
  AnimationManager::get().addAnimation(updateFPS_);
}

void GlutRenderTree::mainLoop()
{
  DEBUG_LOG("initial render tree:");
  debugTree(globalStates_.get(), "  ");
  GlutApplication::mainLoop();
}
void GlutRenderTree::render(GLdouble dt)
{
  timeDelta_->setUniformData(dt);
  renderTree_->traverse(renderState_.get());
}
void GlutRenderTree::postRender(GLdouble dt)
{
  renderTree_->updateStates(dt);
}

#endif /* GLUT_RENDER_TREE_CPP_ */
