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
#include <ogle/states/depth-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/font/font-manager.h>
#include <ogle/models/quad.h>
#include <ogle/shader/shader-manager.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/textures/cube-image-texture.h>

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
class ResizeFramebufferEvent : public EventCallable
{
public:
  ResizeFramebufferEvent(
      ref_ptr<FBOState> &fboState,
      GLfloat windowWidthScale,
      GLfloat windowHeightScale)
  : EventCallable(),
    fboState_(fboState),
    widthScale_(windowWidthScale),
    heightScale_(windowHeightScale)
  {
  }
  virtual void call(EventObject *evObject, void*)
  {
    GlutRenderTree *tree = (GlutRenderTree*)evObject;
    fboState_->resize(
        tree->windowWidth()*widthScale_,
        tree->windowHeight()*heightScale_);
  }
  ref_ptr<FBOState> fboState_;
  GLfloat widthScale_;
  GLfloat heightScale_;
};
class PingPongPass : public State
{
public:
  PingPongPass(
      ref_ptr<Texture> sceneTexture,
      GLboolean firstAttachmentIsNextTarget)
  : State(),
    sceneTexture_(sceneTexture)
  {
    if(firstAttachmentIsNextTarget) {
      nextRenderTarget_ = GL_COLOR_ATTACHMENT0;
      nextRenderSource_ = 1;
    } else {
      nextRenderTarget_ = GL_COLOR_ATTACHMENT1;
      nextRenderSource_ = 0;
    }
  }
  virtual void disable(RenderState *state)
  {
    // render next pass to the attachment that this pass
    // used as render source
    glDrawBuffer(nextRenderTarget_);
    // next sceneTexture_->bind() will affect color
    // attachment nextRenderSource_
    sceneTexture_->set_bufferIndex(nextRenderSource_);
  }
  virtual string name() {
    return FORMAT_STRING(
        "PingPongPass[nextSource=" << nextRenderSource_ << "]");
  };
  ref_ptr<Texture> sceneTexture_;
  GLuint nextRenderSource_;
  GLenum nextRenderTarget_;
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
  renderState_(renderState),
  fov_(45.0f),
  near_(0.1f),
  far_(200.0f),
  numOrthoPasses_(0u)
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

  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);

  // ortho camera uses orthogonal unit quad projection.
  // this is nice because then xy coordinates can be used
  // as texco coordinates without dividing by viewport.
  orthoCamera_ = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  orthoCamera_->updateProjection(1.0f, 1.0f);
  orthoPasses_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(orthoCamera_)));
  // disable depth test for ortho
  orthoPasses_->state()->joinStates(ref_ptr<State>::cast(depthState));
  UnitQuad::Config quadCfg;
  quadCfg.isNormalRequired = GL_FALSE;
  quadCfg.isTangentRequired = GL_FALSE;
  quadCfg.isTexcoRequired = GL_FALSE;
  quadCfg.levelOfDetail = 0;
  quadCfg.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
  quadCfg.posScale = Vec3f(2.0f);
  quadCfg.translation = Vec3f(-1.0f,-1.0f,0.0f);
  orthoQuad_ = ref_ptr<MeshState>::manage(new UnitQuad(quadCfg));

  // gui camera uses orthogonal projection for a quad with
  // width=windowWidth and height=windowHeight.
  // GUI elements can just use window coordinates like this.
  guiCamera_ = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  guiPass_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(guiCamera_)));
  guiPass_->state()->joinStates(ref_ptr<State>::manage(new BlendState));
  // disable depth test for GUI
  guiPass_->state()->joinStates(ref_ptr<State>::cast(depthState));

  if(useDefaultCameraManipulator) {
    camManipulator_ = ref_ptr<LookAtCameraManipulator>::manage(
        new LookAtCameraManipulator(perspectiveCamera_, 10) );
    camManipulator_->set_height( 0.0f );
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

  updateProjection();
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
ref_ptr<StateNode>& GlutRenderTree::guiPass()
{
  return guiPass_;
}
ref_ptr<StateNode>& GlutRenderTree::orthoPass()
{
  return orthoPasses_;
}
ref_ptr<PerspectiveCamera>& GlutRenderTree::perspectiveCamera()
{
  return perspectiveCamera_;
}
ref_ptr<OrthoCamera>& GlutRenderTree::guiCamera()
{
  return guiCamera_;
}
ref_ptr<OrthoCamera>& GlutRenderTree::orthoCamera()
{
  return orthoCamera_;
}
ref_ptr<RenderTree>& GlutRenderTree::renderTree()
{
  return renderTree_;
}
ref_ptr<RenderState>& GlutRenderTree::renderState()
{
  return renderState_;
}
ref_ptr<MeshState>& GlutRenderTree::orthoQuad()
{
  return orthoQuad_;
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
void GlutRenderTree::useOrthoPasses()
{
  // do the first ping pong of scene buffer
  // first ortho pass renders to GL_COLOR_ATTACHMENT1
  // and uses GL_COLOR_ATTACHMENT0 as input scene texture
  GLboolean firstAttachmentIsNextTarget = GL_FALSE;
  ref_ptr<State> initPingPong = ref_ptr<State>::manage(
      new PingPongPass(sceneTexture_, firstAttachmentIsNextTarget));
  orthoPasses_->addChild(
      ref_ptr<StateNode>::manage(new StateNode(initPingPong)));
  renderTree_->addChild(globalStates_, orthoPasses_, false);
}
void GlutRenderTree::setBlitToScreen(
    ref_ptr<FrameBufferObject> fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, windowSize_, attachment));
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
    GLfloat windowWidthScale,
    GLfloat windowHeightScale,
    GLenum colorAttachmentFormat,
    GLenum depthAttachmentFormat,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  sceneFBO_ = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(
          windowWidthScale*windowSize_.x,
          windowHeightScale*windowSize_.y,
          colorAttachmentFormat,
          depthAttachmentFormat));
  // add 2 textures for ping pong rendering
  //sceneTexture_ = sceneFBO_->addTexture(2);
  sceneTexture_ = sceneFBO_->addRectangleTexture(2);
  // shaders can access the texture with name 'sceneTexture'
  sceneTexture_->set_name("sceneTexture");

  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(sceneFBO_));
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

  ref_ptr<EventCallable> resizeFramebuffer = ref_ptr<EventCallable>::manage(
      new ResizeFramebufferEvent(fboState, windowWidthScale, windowHeightScale)
      );
  connect(RESIZE_EVENT, resizeFramebuffer);

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

ref_ptr<StateNode> GlutRenderTree::addOrthoPass(ref_ptr<State> orthoPass)
{
  if(orthoPasses_->parent().get() == NULL) {
    useOrthoPasses();
  }
  // ping pong with scene texture (as input and target)
  if(lastOrthoPass_.get()) {
    GLboolean firstAttachmentIsNextTarget = (numOrthoPasses_%2 == 1);
    lastOrthoPass_->joinStates(ref_ptr<State>::manage(
        new PingPongPass(sceneTexture_, firstAttachmentIsNextTarget)));
  }
  lastOrthoPass_ = orthoPass;
  numOrthoPasses_ += 1;
  // give ortho passes access to scene texture
  orthoPass->joinStates(
      ref_ptr<State>::manage(new TextureState(sceneTexture_)));
  // draw a quad
  orthoPass->joinStates(ref_ptr<State>::cast(orthoQuad_));
  ref_ptr<StateNode> orthoPassNode =
      ref_ptr<StateNode>::manage(new StateNode(orthoPass));
  // add a node to the render tree
  renderTree_->addChild(orthoPasses_, orthoPassNode, GL_TRUE);
  return orthoPassNode;
}

ref_ptr<StateNode> GlutRenderTree::addAntiAliasingPass(FXAA::Config &cfg)
{
  if(orthoPasses_->parent().get() == NULL) {
    useOrthoPasses();
  }

  map< string, ref_ptr<ShaderInput> > inputs =
      renderTree_->collectParentInputs(*orthoPasses_.get());
  ShaderFunctions fs, vs;

  vs.addInput(GLSLTransfer("vec3", "in_pos"));
  vs.addExport(GLSLExport("gl_Position", "vec4(in_pos.xy,0.0,1.0)") );

  fs.operator +=(FXAA(cfg));
  fs.addFragmentOutput(GLSLFragmentOutput(
    "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 ));
  fs.addExport(GLSLExport(
    "defaultColorOutput", "fxaa()" ));

  ref_ptr<Shader> shader_ = ref_ptr<Shader>::manage(new Shader);
  map<GLenum, string> stages;
  stages[GL_FRAGMENT_SHADER] =
      ShaderManager::generateSource(fs, GL_FRAGMENT_SHADER, GL_NONE);
  stages[GL_VERTEX_SHADER] =
      ShaderManager::generateSource(vs, GL_VERTEX_SHADER, GL_FRAGMENT_SHADER);
  if(shader_->compile(stages) && shader_->link())
  {
    set<string> attributeNames, uniformNames;
    for(list<GLSLTransfer>::const_iterator
        it=vs.inputs().begin(); it!=vs.inputs().end(); ++it)
    {
      attributeNames.insert(it->name);
    }
    for(list<GLSLUniform>::const_iterator
        it=vs.uniforms().begin(); it!=vs.uniforms().end(); ++it)
    {
      uniformNames.insert(it->name);
    }
    for(list<GLSLUniform>::const_iterator
        it=fs.uniforms().begin(); it!=fs.uniforms().end(); ++it)
    {
      uniformNames.insert(it->name);
    }
    shader_->setupLocations(attributeNames, uniformNames);
    shader_->setupInputs(inputs);
  }

  return addOrthoPass(ref_ptr<State>::manage(new ShaderState(shader_)));
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
  ref_ptr<Texture> skyTex = ref_ptr<Texture>::manage(
      new CubeImageTexture(imagePath, fileExtension));
  skyBox_ = ref_ptr<SkyBox>::manage(
      new SkyBox(ref_ptr<Camera>::cast(perspectiveCamera_), skyTex, far_));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->set_shading(Material::NO_SHADING);
  material->setConstantUniforms(GL_TRUE);

  return addMesh(ref_ptr<MeshState>::cast(skyBox_),
      ref_ptr<ModelTransformationState>(), material);
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

  ref_ptr<ModelTransformationState> modelTransformation =
      ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
  modelTransformation->translate( Vec3f( 2.0, 18.0, 0.0 ), 0.0f );
  addGUIElement(ref_ptr<MeshState>::cast(fpsText_), modelTransformation);

  updateFPS_ = ref_ptr<Animation>::manage(new UpdateFPS(fpsText_));
  AnimationManager::get().addAnimation(updateFPS_);
}

void GlutRenderTree::set_nearDistance(GLfloat near)
{
  near_ = near;
  updateProjection();
}
void GlutRenderTree::set_farDistance(GLfloat far)
{
  far_ = far;
  updateProjection();
  if(skyBox_.get()!=NULL) {
    skyBox_->resize(far);
  }
}
void GlutRenderTree::set_fieldOfView(GLfloat fov)
{
  fov_ = fov;
  updateProjection();
}

void GlutRenderTree::updateProjection()
{
  guiCamera_->updateProjection(windowSize_.x, windowSize_.y);

  GLfloat aspect = ((GLfloat)windowSize_.x)/((GLfloat)windowSize_.y);
  perspectiveCamera_->updateProjection(fov_, near_, far_, aspect);
}

void GlutRenderTree::reshape()
{
  GlutApplication::reshape();
  updateProjection();
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
