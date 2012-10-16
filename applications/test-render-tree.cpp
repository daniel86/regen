/*
 * glut-render-tree.cpp
 *
 *  Created on: 10.08.2012
 *      Author: daniel
 */

#ifndef GLUT_RENDER_TREE_CPP_
#define GLUT_RENDER_TREE_CPP_

#include "test-render-tree.h"

#include <ogle/states/blit-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/render-tree/picker.h>
#include <ogle/states/shader-state.h>
#include <ogle/font/font-manager.h>
#include <ogle/models/quad.h>
#include <ogle/shader/shader-manager.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/textures/cube-image-texture.h>

#include "ogle-application.h"

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
    TestRenderTree *tree = (TestRenderTree*)evObject;
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

GLuint TestRenderTree::RESIZE_EVENT =
    EventObject::registerEvent("testRenderTreeResize");

TestRenderTree::TestRenderTree(
    GLuint width, GLuint height)
: OGLERenderTree(),
  fov_(45.0f),
  near_(0.1f),
  far_(200.0f),
  numOrthoPasses_(0u),
  windowSize_(width,height)
{
  timeDelta_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("deltaT"));
  timeDelta_->setUniformData(0.0f);
  mousePosition_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("mousePosition"));
  mousePosition_->setUniformData(Vec2f(0.0f));

  perspectiveCamera_ = ref_ptr<PerspectiveCamera>::manage(new PerspectiveCamera);
  orthoCamera_ = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  orthoCamera_->updateProjection(1.0f, 1.0f);
  guiCamera_ = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  updateProjection();

  UnitQuad::Config quadCfg;
  quadCfg.isNormalRequired = GL_FALSE;
  quadCfg.isTangentRequired = GL_FALSE;
  quadCfg.isTexcoRequired = GL_FALSE;
  quadCfg.levelOfDetail = 0;
  quadCfg.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
  quadCfg.posScale = Vec3f(2.0f);
  quadCfg.translation = Vec3f(-1.0f,-1.0f,0.0f);
  orthoQuad_ = ref_ptr<MeshState>::manage(new UnitQuad(quadCfg));

  globalStates_ = rootNode();

  perspectivePass_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(perspectiveCamera_)));

  defaultLight_ = ref_ptr<Light>::manage(new Light);
}

void TestRenderTree::setRenderState(ref_ptr<RenderState> &renderState)
{
  renderState_ = renderState;
}

void TestRenderTree::setMousePosition(GLuint x, GLuint y)
{
  mousePosition_->setUniformData(Vec2f(x,y));
}

void TestRenderTree::initTree()
{
  if(renderState_.get()==NULL) {
    renderState_ = ref_ptr<RenderState>::manage(new RenderState);
  }

  globalStates_->state()->joinShaderInput(ref_ptr<ShaderInput>::cast(timeDelta_));
  globalStates_->state()->joinShaderInput(ref_ptr<ShaderInput>::cast(mousePosition_));

  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);

  // ortho camera uses orthogonal unit quad projection.
  // this is nice because then xy coordinates can be used
  // as texco coordinates without dividing by viewport.
  orthoPasses_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(orthoCamera_)));
  orthoPassesCustomTarget_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(orthoCamera_)));

  // gui camera uses orthogonal projection for a quad with
  // width=windowWidth and height=windowHeight.
  // GUI elements can just use window coordinates like this.
  guiPass_ = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(guiCamera_)));
  guiPass_->state()->joinStates(ref_ptr<State>::manage(new BlendState));
  // disable depth test for GUI
  guiPass_->state()->joinStates(ref_ptr<State>::cast(depthState));
}

ref_ptr<Light>& TestRenderTree::defaultLight()
{
  return defaultLight_;
}
ref_ptr<StateNode>& TestRenderTree::globalStates()
{
  return globalStates_;
}
ref_ptr<StateNode>& TestRenderTree::perspectivePass()
{
  return perspectivePass_;
}
ref_ptr<StateNode>& TestRenderTree::guiPass()
{
  return guiPass_;
}
ref_ptr<StateNode>& TestRenderTree::orthoPass()
{
  return orthoPasses_;
}
ref_ptr<PerspectiveCamera>& TestRenderTree::perspectiveCamera()
{
  return perspectiveCamera_;
}
ref_ptr<OrthoCamera>& TestRenderTree::guiCamera()
{
  return guiCamera_;
}
ref_ptr<OrthoCamera>& TestRenderTree::orthoCamera()
{
  return orthoCamera_;
}
ref_ptr<RenderState>& TestRenderTree::renderState()
{
  return renderState_;
}
ref_ptr<MeshState>& TestRenderTree::orthoQuad()
{
  return orthoQuad_;
}

void TestRenderTree::addRootNodeVBO(GLuint sizeMB)
{
  addVBONode(globalStates_, sizeMB);
}
void TestRenderTree::addPerspectiveVBO(GLuint sizeMB)
{
  if(perspectivePass_->parent().get() == NULL) {
    usePerspectivePass();
  }
  addVBONode(perspectivePass_, sizeMB);
}
void TestRenderTree::addGUIVBO(GLuint sizeMB)
{
  if(guiPass_->parent().get() == NULL) {
    useGUIPass();
  }
  addVBONode(guiPass_, sizeMB);
}

void TestRenderTree::usePerspectivePass()
{
  addChild(globalStates_, perspectivePass_, false);
}
ref_ptr<Picker> TestRenderTree::usePicking()
{
  if(perspectivePass_->parent().get() == NULL) {
    usePerspectivePass();
  }
  ref_ptr<Picker> picker = ref_ptr<Picker>::manage(new Picker(perspectivePass_));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(picker));
  return picker;
}
void TestRenderTree::useGUIPass()
{
  addChild(globalStates_, guiPass_, false);
}
void TestRenderTree::useOrthoPassesCustomTarget()
{
  addChild(globalStates_, orthoPassesCustomTarget_, false);
}
void TestRenderTree::useOrthoPasses()
{
  // do the first ping pong of scene buffer
  // first ortho pass renders to GL_COLOR_ATTACHMENT1
  // and uses GL_COLOR_ATTACHMENT0 as input scene texture
  GLboolean firstAttachmentIsNextTarget = GL_FALSE;
  ref_ptr<State> initPingPong = ref_ptr<State>::manage(
      new PingPongPass(sceneTexture_, firstAttachmentIsNextTarget));
  orthoPasses_->addChild(
      ref_ptr<StateNode>::manage(new StateNode(initPingPong)));
  addChild(globalStates_, orthoPasses_, false);
}
void TestRenderTree::setBlitToScreen(
    ref_ptr<FrameBufferObject> fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, windowSize_, attachment));
  ref_ptr<StateNode> blitNode = ref_ptr<StateNode>::manage(
      new StateNode(blitState));
  globalStates_->addChild(blitNode);
}
void TestRenderTree::setClearScreenColor(const Vec4f &clearColor)
{
  ref_ptr<ClearColorState> clearScreenColor =
      ref_ptr<ClearColorState>::manage(new ClearColorState);
  ClearColorData clearData;
  clearData.clearColor = clearColor;
  clearData.colorAttachment = GL_FRONT;
  clearScreenColor->data.push_back(clearData);
  globalStates_->state()->addEnabler(ref_ptr<Callable>::cast(clearScreenColor));
}
void TestRenderTree::setClearScreenDepth()
{
  ref_ptr<Callable> clearScreenDepth =
      ref_ptr<Callable>::manage(new ClearDepthState);
  globalStates_->state()->addEnabler(clearScreenDepth);
}

ref_ptr<Light>& TestRenderTree::setLight()
{
  setLight(defaultLight_);
  return defaultLight_;
}
void TestRenderTree::setLight(ref_ptr<Light> light)
{
  perspectivePass_->state()->joinStates(ref_ptr<State>::cast(light));
}

ref_ptr<FBOState> TestRenderTree::createRenderTarget(
    GLfloat windowWidthScale,
    GLfloat windowHeightScale,
    GLenum colorAttachmentFormat,
    GLenum depthAttachmentFormat,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(
          windowWidthScale*windowSize_.x,
          windowHeightScale*windowSize_.y,
          colorAttachmentFormat,
          depthAttachmentFormat));

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

  ref_ptr<EventCallable> resizeFramebuffer = ref_ptr<EventCallable>::manage(
      new ResizeFramebufferEvent(fboState, windowWidthScale, windowHeightScale)
      );
  connect(RESIZE_EVENT, resizeFramebuffer);

  return fboState;
}

ref_ptr<FBOState> TestRenderTree::setRenderToTexture(
    GLfloat windowWidthScale,
    GLfloat windowHeightScale,
    GLenum colorAttachmentFormat,
    GLenum depthAttachmentFormat,
    GLboolean clearDepthBuffer,
    GLboolean clearColorBuffer,
    const Vec4f &clearColor)
{
  ref_ptr<FBOState> fboState = createRenderTarget(
      windowWidthScale,
      windowHeightScale,
      colorAttachmentFormat,
      depthAttachmentFormat,
      clearDepthBuffer,
      clearColorBuffer,
      clearColor);
  sceneFBO_ = fboState->fbo();
  // add 2 textures for ping pong rendering
  sceneTexture_ = fboState->fbo()->addTexture(2);
  // shaders can access the texture with name 'sceneTexture'
  sceneTexture_->set_name("sceneTexture");

  globalStates_->state()->joinStates(ref_ptr<State>::cast(fboState));

  return fboState;
}
ref_ptr<FBOState> TestRenderTree::setRenderToTexture(
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
  sceneFBO_ = fboState->fbo();

  return fboState;
}

ref_ptr<StateNode> TestRenderTree::addDummyOrthoPass()
{
  ref_ptr<State> hiddenState = ref_ptr<State>::manage(new State);
  hiddenState->set_isHidden(GL_TRUE);
  // useOrthoPasses
  ref_ptr<State> dummyPingPong = ref_ptr<State>::manage(
      new PingPongPass(sceneTexture_, GL_TRUE));
  dummyPingPong->joinStates(hiddenState);

  return addOrthoPass(dummyPingPong, GL_FALSE);
}

ref_ptr<StateNode> TestRenderTree::addOrthoPass(ref_ptr<State> orthoPass, GLboolean pingPong)
{
  if(orthoPasses_->parent().get() == NULL) {
    useOrthoPasses();
  }
  // ping pong with scene texture (as input and target)
  if(pingPong) {
    if(lastOrthoPass_.get()) {
      GLboolean firstAttachmentIsNextTarget = (numOrthoPasses_%2 == 1);
      lastOrthoPass_->joinStates(ref_ptr<State>::manage(
          new PingPongPass(sceneTexture_, firstAttachmentIsNextTarget)));
    }
    lastOrthoPass_ = orthoPass;
    numOrthoPasses_ += 1;
  }
  // give ortho passes access to scene texture
  orthoPass->joinStates(
      ref_ptr<State>::manage(new TextureState(sceneTexture_)));
  // draw a quad
  orthoPass->joinStates(ref_ptr<State>::cast(orthoQuad_));
  ref_ptr<StateNode> orthoPassNode =
      ref_ptr<StateNode>::manage(new StateNode(orthoPass));
  // add a node to the render tree
  addChild(orthoPasses_, orthoPassNode, GL_TRUE);
  return orthoPassNode;
}

ref_ptr<Shader> TestRenderTree::createShader(
    ref_ptr<StateNode> &node,
    ShaderFunctions &vs,
    ShaderFunctions &fs)
{
  map< string, ref_ptr<ShaderInput> > inputs =
      collectParentInputs(*node.get());
  map<GLenum, string> stagesStr;
  map<GLenum, ShaderFunctions*> stages;

  stages[GL_FRAGMENT_SHADER] = &fs;
  stages[GL_VERTEX_SHADER] = &vs;
  ShaderManager::setupInputs(inputs, stages);

  stagesStr[GL_FRAGMENT_SHADER] =
      ShaderManager::generateSource(fs, GL_FRAGMENT_SHADER, GL_NONE);
  stagesStr[GL_VERTEX_SHADER] =
      ShaderManager::generateSource(vs, GL_VERTEX_SHADER, GL_FRAGMENT_SHADER);

  ref_ptr<Shader> shader_ = ref_ptr<Shader>::manage(new Shader(stagesStr));
  if(shader_->compile() && shader_->link())
  {
    ShaderManager::setupLocations(shader_, stages);
    shader_->setupInputs(inputs);
  }
  return shader_;
}
ref_ptr<State> TestRenderTree::createBlurState(
    const string &name,
    const ConvolutionKernel &kernel,
    ref_ptr<Texture> &blurredTexture,
    ref_ptr<StateNode> &blurNode)
{
  ShaderFunctions fs, vs;
  vs.addInput(GLSLTransfer("vec3", "in_pos"));
  vs.addExport(GLSLExport("gl_Position", "vec4(in_pos.xy,0.0,1.0)") );
  vs.addOutput(GLSLTransfer("vec2", "out_texco"));
  vs.addExport(GLSLExport("out_texco", "(in_pos.xy+vec2(1.0))*0.5") );

  fs.addInput(GLSLTransfer("vec2", "in_texco"));
  fs.addUniform( GLSLUniform( "sampler2D", "in_blurTexture" ));
  fs.addFragmentOutput(GLSLFragmentOutput(
    "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT1 ));
  ConvolutionShader blurShader(name, kernel);
  fs.addDependencyCode("blurHorizontal", blurShader.code());
  fs.addExport(GLSLExport(
    "defaultColorOutput", FORMAT_STRING(name<<"(in_blurTexture, in_texco)") ));

  ref_ptr<Shader> shader_ = createShader(blurNode, vs, fs);

  ref_ptr<State> blurState = ref_ptr<State>::manage(new ShaderState(shader_));
  blurState->joinStates(
      ref_ptr<State>::manage(new TextureState(blurredTexture)));
  blurState->joinStates(ref_ptr<State>::cast(orthoQuad_));

  return blurState;
}

ref_ptr<FBOState> TestRenderTree::addBlurPass(
    const BlurConfig &blurCfg,
    GLdouble winScaleX,
    GLdouble winScaleY,
    ref_ptr<State> blurState)
{
  if(orthoPassesCustomTarget_->parent().get() == NULL) {
    useOrthoPassesCustomTarget();
  }

  ref_ptr<FBOState> blurredBuffer = createRenderTarget(
      winScaleX, winScaleY, // window size scale
      sceneFBO_->colorAttachmentFormat(),
      GL_NONE,  // no depth attachment
      GL_FALSE, // no clear depth
      GL_FALSE, // no clear color
      Vec4f(0.0)
  );
  ref_ptr<Texture> blurredTexture = blurredBuffer->fbo()->addTexture(2);
  blurredTexture->set_name("blurTexture");
  // parent node for blur passes
  if(blurState.get()==NULL) {
    blurState = ref_ptr<State>::manage(new TextureState(sceneTexture_));
  } else {
    blurState->joinStates(
        ref_ptr<State>::manage(new TextureState(sceneTexture_)));
  }
  ref_ptr<StateNode> blurNode = ref_ptr<StateNode>::manage(new StateNode(blurState));
  // setup render target, first pass draws to attachment 0
  blurNode->state()->joinStates(ref_ptr<State>::cast(blurredBuffer));

  addChild(orthoPassesCustomTarget_, blurNode, GL_TRUE);

  // first pass downsamples the scene texture with attachment
  // 0 of blurredBuffer as render target.
  {
    ShaderFunctions fs, vs;
    vs.addInput(GLSLTransfer("vec3", "in_pos"));
    vs.addExport(GLSLExport("gl_Position", "vec4(in_pos.xy,0.0,1.0)") );
    vs.addOutput(GLSLTransfer("vec2", "out_texco"));
    vs.addExport(GLSLExport("out_texco", "(in_pos.xy+vec2(1.0))*0.5") );

    fs.addInput(GLSLTransfer("vec2", "in_texco"));
    fs.addUniform( GLSLUniform( "sampler2D", "in_sceneTexture" ));
    fs.addFragmentOutput(GLSLFragmentOutput(
      "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 ));
    fs.addExport(GLSLExport(
      "defaultColorOutput", "texture(in_sceneTexture, in_texco)" ));

    ref_ptr<Shader> shader_ = createShader(blurNode, vs, fs);

    ref_ptr<State> downsampleState = ref_ptr<State>::manage(new ShaderState(shader_));
    downsampleState->joinStates(ref_ptr<State>::cast(orthoQuad_));

    // next target attachment is attachment1
    GLboolean firstAttachmentIsNextTarget = GL_FALSE;
    downsampleState->joinStates(ref_ptr<State>::manage(
        new PingPongPass(blurredTexture, firstAttachmentIsNextTarget)));

    addChild(blurNode, ref_ptr<StateNode>::manage(
        new StateNode(downsampleState)), GL_TRUE);
  }

  // second pass does horizontal blur of downsampled buffer
  {
    ref_ptr<State> blurHorizontalState = createBlurState(
        "blurHorizontal",
        blurHorizontalKernel(*blurredTexture.get(), blurCfg),
        blurredTexture,
        blurNode);
    // next target attachment is attachment1
    GLboolean firstAttachmentIsNextTarget = GL_TRUE;
    blurHorizontalState->joinStates(ref_ptr<State>::manage(
        new PingPongPass(blurredTexture, firstAttachmentIsNextTarget)));
    addChild(blurNode,
        ref_ptr<StateNode>::manage(new StateNode(blurHorizontalState)),
        GL_TRUE);
  }

  // third pass does vertical blur of downsampled buffer
  {
    ref_ptr<State> blurVerticalState = createBlurState(
        "blurVertical",
        blurVerticalKernel(*blurredTexture.get(), blurCfg),
        blurredTexture,
        blurNode);
    // next target attachment is attachment0
    GLboolean firstAttachmentIsNextTarget = GL_FALSE;
    blurVerticalState->joinStates(ref_ptr<State>::manage(
        new PingPongPass(blurredTexture, firstAttachmentIsNextTarget)));
    addChild(blurNode,
        ref_ptr<StateNode>::manage(new StateNode(blurVerticalState)),
        GL_TRUE);
  }

  return blurredBuffer;
}

ref_ptr<StateNode> TestRenderTree::addTonemapPass(
    const TonemapConfig &tonemapCfg,
    ref_ptr<Texture> blurTexture,
    GLdouble winScaleX,
    GLdouble winScaleY,
    ref_ptr<State> tonemapState)
{
  if(orthoPasses_->parent().get() == NULL) {
    useOrthoPasses();
  }

  ShaderFunctions fs, vs;

  vs.addInput(GLSLTransfer("vec3", "in_pos"));
  vs.addExport(GLSLExport("gl_Position", "vec4(in_pos.xy,0.0,1.0)") );
  vs.addOutput(GLSLTransfer("vec2", "out_texco"));
  vs.addExport(GLSLExport("out_texco", "(in_pos.xy+vec2(1.0))*0.5") );

  vector<string> args(3);
  args[0] = FORMAT_STRING("in_" << sceneTexture_->name());
  args[1] = FORMAT_STRING("in_" << blurTexture->name());
  args[2] = "color";
  TonemapShader tonemap(tonemapCfg, args);

  fs.addInput(GLSLTransfer("vec2", "in_texco"));
  fs.addFragmentOutput(GLSLFragmentOutput(
    "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 ));
  fs.addMainVar(GLSLVariable("vec4", "color" ));
  fs.operator +=(tonemap);
  fs.addExport(GLSLExport("defaultColorOutput", "color" ));

  if(tonemapState.get()==NULL) {
    tonemapState = ref_ptr<State>::manage(new TextureState(blurTexture));
  } else {
    tonemapState->joinStates(ref_ptr<State>::manage(
        new TextureState(blurTexture)));
  }
  ref_ptr<ShaderState> shaderState =
      ref_ptr<ShaderState>::manage(new ShaderState);
  tonemapState->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> node = addOrthoPass(tonemapState);

  // create the shader after node was added to the tree
  ref_ptr<Shader> shader_ = createShader(node, vs, fs);
  shaderState->set_shader(shader_);

  return node;
}

ref_ptr<StateNode> TestRenderTree::addAntiAliasingPass(
    FXAA::Config &cfg,
    ref_ptr<State> aaState)
{
  if(orthoPasses_->parent().get() == NULL) {
    useOrthoPasses();
  }

  ShaderFunctions fs, vs;

  vs.addInput(GLSLTransfer("vec3", "in_pos"));
  vs.addExport(GLSLExport("gl_Position", "vec4(in_pos.xy,0.0,1.0)") );
  vs.addOutput(GLSLTransfer("vec2", "out_texco"));
  vs.addExport(GLSLExport("out_texco", "(in_pos.xy+vec2(1.0))*0.5") );

  fs.addInput(GLSLTransfer("vec2", "in_texco"));
  fs.operator +=(FXAA(cfg));
  fs.addFragmentOutput(GLSLFragmentOutput(
    "vec4", "defaultColorOutput", GL_COLOR_ATTACHMENT0 ));
  fs.addExport(GLSLExport(
    "defaultColorOutput", "fxaa()" ));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  if(aaState.get()==NULL) {
    aaState = ref_ptr<State>::cast(shaderState);
  } else {
    aaState->joinStates(ref_ptr<State>::cast(shaderState));
  }

  ref_ptr<StateNode> node = addOrthoPass(aaState);

  // create the shader after node was added to the tree
  ref_ptr<Shader> shader_ = createShader(node, vs, fs);
  shaderState->set_shader(shader_);

  return node;
}

ref_ptr<StateNode> TestRenderTree::addMesh(
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

ref_ptr<StateNode> TestRenderTree::addGUIElement(
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

ref_ptr<StateNode> TestRenderTree::addMesh(
    ref_ptr<StateNode> parent,
    ref_ptr<MeshState> mesh,
    ref_ptr<ModelTransformationState> modelTransformation,
    ref_ptr<Material> material,
    GLboolean isShaderRequired,
    GLboolean isVBORequired)
{
  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  ref_ptr<ShaderState> shaderState =
      ref_ptr<ShaderState>::manage(new ShaderState);
  ref_ptr<StateNode> materialNode, shaderNode, modeltransformNode;

  ref_ptr<StateNode> *root = &meshNode;
  if(isShaderRequired) {
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

  addChild(parent, *root, isVBORequired);

  if(isShaderRequired) {
    ref_ptr<Shader> shader = generateShader(*meshNode.get());
    shaderState->set_shader(shader);
  }

  return meshNode;
}

ref_ptr<StateNode> TestRenderTree::addSkyBox(
    const string &imagePath,
    GLenum internalFormat,
    GLboolean flipBackFace)
{
  ref_ptr<Texture> skyTex = ref_ptr<Texture>::manage(
      new CubeImageTexture(imagePath, internalFormat, flipBackFace));
  skyBox_ = ref_ptr<SkyBox>::manage(
      new SkyBox(ref_ptr<Camera>::cast(perspectiveCamera_), skyTex, far_));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->set_shading(Material::NO_SHADING);
  material->setConstantUniforms(GL_TRUE);

  return addMesh(ref_ptr<MeshState>::cast(skyBox_),
      ref_ptr<ModelTransformationState>(), material);
}
ref_ptr<StateNode> TestRenderTree::addSkyBox(
    ref_ptr<Texture> &customSkyTex)
{
  ref_ptr<Texture> skyTex = ref_ptr<Texture>::manage(new CubeImageTexture);
  skyTex->setGLResources(*customSkyTex.get());
  skyTex->set_format(customSkyTex->format());
  skyTex->set_internalFormat(customSkyTex->internalFormat());
  skyTex->set_pixelType(customSkyTex->pixelType());
  skyTex->set_size(customSkyTex->width(), customSkyTex->height());
  skyBox_ = ref_ptr<SkyBox>::manage(
      new SkyBox(ref_ptr<Camera>::cast(perspectiveCamera_), skyTex, far_));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->set_shading(Material::NO_SHADING);
  material->setConstantUniforms(GL_TRUE);

  return addMesh(ref_ptr<MeshState>::cast(skyBox_),
      ref_ptr<ModelTransformationState>(), material);
}

void TestRenderTree::setShowFPS()
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

void TestRenderTree::set_nearDistance(GLfloat near)
{
  near_ = near;
  updateProjection();
}
void TestRenderTree::set_farDistance(GLfloat far)
{
  far_ = far;
  updateProjection();
  if(skyBox_.get()!=NULL) {
    skyBox_->resize(far);
  }
}
void TestRenderTree::set_fieldOfView(GLfloat fov)
{
  fov_ = fov;
  updateProjection();
}

void TestRenderTree::updateProjection()
{
  guiCamera_->updateProjection(windowSize_.x, windowSize_.y);
  GLfloat aspect = ((GLfloat)windowSize_.x)/((GLfloat)windowSize_.y);
  perspectiveCamera_->updateProjection(fov_, near_, far_, aspect);
}

void TestRenderTree::setWindowSize(GLuint w, GLuint h)
{
  windowSize_.x = w;
  windowSize_.y = h;
  updateProjection();
  emitEvent(RESIZE_EVENT);
}
void TestRenderTree::render(GLdouble dt)
{
  timeDelta_->setUniformData(dt);
  RenderTree::traverse(renderState_.get(), dt);
}
void TestRenderTree::postRender(GLdouble dt)
{
  updateStates(dt);
}

#endif /* GLUT_RENDER_TREE_CPP_ */
