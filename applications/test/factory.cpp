/*
 * factory.cpp
 *
 *  Created on: 18.02.2013
 *      Author: daniel
 */

#include <regen/config.h>
#include "factory.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

namespace regen {

// Resizes Framebuffer texture when the window size changed
class FramebufferResizer : public EventHandler
{
public:
  FramebufferResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
  : EventHandler(), fboState_(fbo), wScale_(wScale), hScale_(hScale) { }

  void call(EventObject *evObject, EventData*) {
    Application *app = (Application*)evObject;
    const Vec2i& winSize = app->windowViewport()->getVertex2i(0);
    fboState_->resize(winSize.x*wScale_, winSize.y*hScale_);
  }

protected:
  ref_ptr<FBOState> fboState_;
  GLfloat wScale_, hScale_;
};
// Resizes FilterSequence textures when the window size changed
class ResizableResizer : public EventHandler
{
public:
  ResizableResizer(const ref_ptr<Resizable> &f)
  : EventHandler(), f_(f) { }

  void call(EventObject *evObject, EventData*) { f_->resize(); }

protected:
  ref_ptr<Resizable> f_;
};

// Updates Camera Projection when window size changes
class ProjectionUpdater : public EventHandler
{
public:
  ProjectionUpdater(const ref_ptr<Camera> &cam,
      GLfloat fov, GLfloat near, GLfloat far)
  : EventHandler(), cam_(cam), fov_(fov), near_(near), far_(far) { }

  void call(EventObject *evObject, EventData*) {
    Application *app = (Application*)evObject;
    const Vec2i& winSize = app->windowViewport()->getVertex2i(0);
    GLfloat aspect = winSize.x/(GLfloat)winSize.y;

    Mat4f &view = *(Mat4f*)cam_->view()->dataPtr();
    Mat4f &viewInv = *(Mat4f*)cam_->viewInverse()->dataPtr();
    Mat4f &proj = *(Mat4f*)cam_->projection()->dataPtr();
    Mat4f &projInv = *(Mat4f*)cam_->projectionInverse()->dataPtr();
    Mat4f &viewproj = *(Mat4f*)cam_->viewProjection()->dataPtr();
    Mat4f &viewprojInv = *(Mat4f*)cam_->viewProjectionInverse()->dataPtr();

    proj = Mat4f::projectionMatrix(fov_, aspect, near_, far_);
    projInv = proj.projectionInverse();
    viewproj = view * proj;
    viewprojInv = projInv * viewInv;
  }

protected:
  ref_ptr<Camera> cam_;
  GLfloat fov_, near_, far_;
};

class FramebufferClear : public State
{
public:
  FramebufferClear() : State() {}
  virtual void enable(RenderState *rs) {
    rs->clearColor().push(ClearColor(0.0));
    glClear(GL_COLOR_BUFFER_BIT);
    rs->clearColor().pop();
  }
};


// create application window and set up OpenGL
ref_ptr<QtApplication> initApplication(int argc, char** argv, const string &windowTitle)
{
#ifdef Q_WS_X11
  // needed because gui and render thread use xlib calls
  XInitThreads();
#endif
  QGLFormat glFormat(
    QGL::SingleBuffer
   |QGL::NoAlphaChannel
   |QGL::NoAccumBuffer
   |QGL::NoDepthBuffer
   |QGL::NoStencilBuffer
   |QGL::NoStereoBuffers
   |QGL::NoSampleBuffers);
  glFormat.setSwapInterval(0);
  glFormat.setDirectRendering(true);
  glFormat.setRgba(true);
  glFormat.setOverlay(false);
  // XXX: text not rendering with core profile...
  //glFormat.setVersion(3,3);
  //glFormat.setProfile(QGLFormat::CoreProfile);

  // create and show application window
  ref_ptr<QtApplication> app = ref_ptr<QtApplication>::manage(
      new QtApplication(argc,argv,glFormat));
  app->setupLogging();
  app->toplevelWidget()->setWindowTitle(windowTitle.c_str());
  app->show();
  return app;
}

// Blits fbo attachment to screen
void setBlitToScreen(
    QtApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &texture,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitTexToScreen(fbo, texture, app->windowViewport(), attachment));
  app->renderTree()->addChild(
      ref_ptr<StateNode>::manage(new StateNode(blitState)));
}
void setBlitToScreen(
    QtApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, app->windowViewport(), attachment));
  app->renderTree()->addChild(
      ref_ptr<StateNode>::manage(new StateNode(blitState)));
}

ref_ptr<TextureCube> createStaticReflectionMap(
    QtApplication *app,
    const string &file,
    const GLboolean flipBackFace,
    const GLenum textureFormat,
    const GLfloat aniso)
{
  ref_ptr<TextureCube> reflectionMap = TextureLoader::loadCube(
      file,flipBackFace,GL_FALSE,textureFormat);

  RenderState::get()->textureChannel().push(GL_TEXTURE7);
  RenderState::get()->textureBind().push(7,
      TextureBind(reflectionMap->targetType(), reflectionMap->id()));
  reflectionMap->set_aniso(aniso);
  reflectionMap->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  reflectionMap->setupMipmaps(GL_DONT_CARE);
  reflectionMap->set_wrapping(GL_CLAMP_TO_EDGE);
  RenderState::get()->textureBind().pop(7);
  RenderState::get()->textureChannel().pop();

  return reflectionMap;
}

PickingGeom* createPicker(GLdouble interval,GLuint maxPickedObjects)
{
  PickingGeom *picker = new PickingGeom(maxPickedObjects);
  picker->set_pickInterval(interval);
  return picker;
}

/////////////////////////////////////
//// Camera
/////////////////////////////////////

class LookAtMotion : public EventHandler
{
public:
  LookAtMotion(const ref_ptr<LookAtCameraManipulator> &m, GLboolean &buttonPressed)
  : EventHandler(), m_(m), buttonPressed_(buttonPressed) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::MouseMotionEvent *ev = (Application::MouseMotionEvent*)data;
    if(buttonPressed_) {
      m_->set_height(m_->height() + ((float)ev->dy)*stepX_, ev->dt );
      m_->setStepLength( ((float)ev->dx)*stepY_, ev->dt );
    }
  }
  ref_ptr<LookAtCameraManipulator> m_;
  const GLboolean &buttonPressed_;
  GLfloat stepX_;
  GLfloat stepY_;
};
class LookAtButton : public EventHandler
{
public:
  LookAtButton(const ref_ptr<LookAtCameraManipulator> &m)
  : EventHandler(), m_(m), buttonPressed_(GL_FALSE) {}

  void call(EventObject *evObject, EventData *data)
  {
    Application::ButtonEvent *ev = (Application::ButtonEvent*)data;

    if(ev->button == 0) {
      buttonPressed_ = ev->pressed;
      if(ev->pressed) {
        m_->setStepLength( 0.0f );
      }
      } else if (ev->button == 4 && !ev->pressed) {
        m_->set_radius( m_->radius()+scrollStep_ );
      } else if (ev->button == 3 && !ev->pressed) {
        m_->set_radius( m_->radius()-scrollStep_ );
    }
  }
  ref_ptr<LookAtCameraManipulator> m_;
  GLboolean buttonPressed_;
  GLfloat scrollStep_;
};

ref_ptr<LookAtCameraManipulator> createLookAtCameraManipulator(
    QtApplication *app,
    const ref_ptr<Camera> &cam,
    const GLfloat &scrollStep,
    const GLfloat &stepX,
    const GLfloat &stepY,
    const GLuint &interval)
{
  ref_ptr<LookAtCameraManipulator> manipulator =
      ref_ptr<LookAtCameraManipulator>::manage(new LookAtCameraManipulator(cam,interval));
  manipulator->set_height( 0.0f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 5.0f );
  manipulator->set_degree( 0.0f );
  manipulator->setStepLength( M_PI*0.01 );

  ref_ptr<LookAtButton> buttonCallable =
      ref_ptr<LookAtButton>::manage(new LookAtButton(manipulator));
  buttonCallable->scrollStep_ = scrollStep;
  app->connect(Application::BUTTON_EVENT, ref_ptr<EventHandler>::cast(buttonCallable));

  ref_ptr<LookAtMotion> motionCallable = ref_ptr<LookAtMotion>::manage(
      new LookAtMotion(manipulator, buttonCallable->buttonPressed_));
  motionCallable->stepX_ = stepX;
  motionCallable->stepY_ = stepY;
  app->connect(Application::MOUSE_MOTION_EVENT, ref_ptr<EventHandler>::cast(motionCallable));

  return manipulator;
}

ref_ptr<Camera> createPerspectiveCamera(
    QtApplication *app,
    GLfloat fov,
    GLfloat near,
    GLfloat far)
{
  ref_ptr<Camera> cam =
      ref_ptr<Camera>::manage(new Camera);

  ref_ptr<ProjectionUpdater> projUpdater =
      ref_ptr<ProjectionUpdater>::manage(new ProjectionUpdater(cam, fov, near, far));
  app->connect(Application::RESIZE_EVENT, ref_ptr<EventHandler>::cast(projUpdater));
  EventData evData;
  evData.eventID = Application::RESIZE_EVENT;
  projUpdater->call(app, &evData);

  return cam;
}

/////////////////////////////////////
//// Instancing
/////////////////////////////////////

ref_ptr<ModelTransformation> createInstancedModelMat(
    GLuint numInstancesX, GLuint numInstancesY, GLfloat instanceDistance)
{
#define RANDOM (rand()%100)/100.0f

  const GLuint numInstances = numInstancesX*numInstancesY;
  const GLfloat startX = -0.5f*numInstancesX*instanceDistance -0.5f;
  const GLfloat startZ = -0.5f*numInstancesY*instanceDistance -0.5f;
  const Vec3f instanceScale(1.0f);
  const Vec3f instanceRotation(0.0f,M_PI,0.0f);
  Vec3f baseTranslation(startX, 0.0f, startZ);

  ref_ptr<ModelTransformation> modelMat =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelMat->modelMat()->setInstanceData(numInstances, 1, NULL);

  Mat4f* instancedModelMats = (Mat4f*)modelMat->modelMat()->dataPtr();
  for(GLuint x=0; x<numInstancesX; ++x)
  {
    baseTranslation.x += instanceDistance;

    for(GLuint y=0; y<numInstancesY; ++y)
    {
      baseTranslation.z += instanceDistance;

      Vec3f instanceTranslation = baseTranslation +
          Vec3f(1.5f*(0.5f-RANDOM),0.0f,1.25f*(0.5f-RANDOM));
      *instancedModelMats = Mat4f::transformationMatrix(
          instanceRotation, instanceTranslation, instanceScale).transpose();
      ++instancedModelMats;
    }

    baseTranslation.z = startZ;
  }

  modelMat->setInput(ref_ptr<ShaderInput>::cast(modelMat->modelMat()));

  return modelMat;

#undef RANDOM
}

/////////////////////////////////////
//// GBuffer / TBuffer
/////////////////////////////////////

// Creates render target for deferred shading.
ref_ptr<FBOState> createGBuffer(
    QtApplication *app,
    GLfloat gBufferScaleW,
    GLfloat gBufferScaleH,
    GLenum colorBufferFormat,
    GLenum depthFormat)
{
  // diffuse, specular, norWorld
  static const GLenum count[] = { 2, 1, 1 };
  static const GLenum formats[] = { GL_RGBA, GL_RGBA, GL_RGBA };
  static const GLenum internalFormats[] = { colorBufferFormat, GL_RGBA, GL_RGBA };
  static const GLenum clearBuffers[] = {
      GL_COLOR_ATTACHMENT2, // spec
      GL_COLOR_ATTACHMENT3  // norWorld
  };

  const Vec2i& winSize = app->windowViewport()->getVertex2i(0);
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject(
      winSize.x*gBufferScaleW, winSize.y*gBufferScaleH, 1,
          GL_TEXTURE_2D, depthFormat, GL_UNSIGNED_BYTE));
  ref_ptr<FBOState> gBufferState = ref_ptr<FBOState>::manage(new FBOState(fbo));

  for(GLuint i=0; i<sizeof(count)/sizeof(GLenum); ++i) {
    fbo->addTexture(count[i], GL_TEXTURE_2D,
        formats[i], internalFormats[i], GL_UNSIGNED_BYTE);
    // call glDrawBuffer
    gBufferState->addDrawBuffer(GL_COLOR_ATTACHMENT0+i+1);
  }
  // make sure buffer index of diffuse texture is set to 0
  gBufferState->joinStates(ref_ptr<State>::manage(
      new TextureSetBufferIndex(fbo->colorBuffer()[0], 0)));

  ClearColorState::Data clearData;
  clearData.clearColor = Vec4f(0.0f);
  clearData.colorBuffers = std::vector<GLenum>(
      clearBuffers, clearBuffers + sizeof(clearBuffers)/sizeof(GLenum));
  gBufferState->setClearColor(clearData);
  if(depthFormat!=GL_NONE) {
    gBufferState->setClearDepth();
  }

  app->connect(Application::RESIZE_EVENT, ref_ptr<EventHandler>::manage(
      new FramebufferResizer(gBufferState,gBufferScaleW,gBufferScaleH)));

  return gBufferState;
}

ref_ptr<TBuffer> createTBuffer(
    QtApplication *app,
    const ref_ptr<Camera> &cam,
    const ref_ptr<Texture> &depthTexture,
    TBuffer::Mode mode,
    GLfloat tBufferScaleW,
    GLfloat tBufferScaleH)
{
  const Vec2i& winSize = app->windowViewport()->getVertex2i(0);
  Vec2ui bufferSize(winSize.x*tBufferScaleW, winSize.y*tBufferScaleH);
  TBuffer *tBufferState = new TBuffer(mode, bufferSize, depthTexture);

  app->addShaderInput("T-buffer",
      ref_ptr<ShaderInput>::cast(tBufferState->ambientLight()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(3),
      "the ambient light.");

  app->connect(Application::RESIZE_EVENT, ref_ptr<EventHandler>::manage(
      new FramebufferResizer(tBufferState->fboState(),tBufferScaleW,tBufferScaleH)));

  return ref_ptr<TBuffer>::manage(tBufferState);
}

/////////////////////////////////////
//// Post Passes
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createPostPassNode(
    QtApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->setDrawBufferOntop(tex, baseAttachment);

  ref_ptr<StateNode> root = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));

  // no depth writing
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthWrite(GL_FALSE);
  depthState->set_useDepthTest(GL_FALSE);
  fboState->joinStates(ref_ptr<State>::cast(depthState));

  return root;
}

ref_ptr<FilterSequence> createBlurState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root,
    GLint size, GLfloat sigma,
    GLboolean downsampleTwice,
    const string &treePath)
{
  ref_ptr<FilterSequence> filter = ref_ptr<FilterSequence>::manage(new FilterSequence(input));

  ref_ptr<ShaderInput1i> blurSize = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("numBlurPixels"));
  blurSize->setUniformData(size);
  filter->joinShaderInput(ref_ptr<ShaderInput>::cast(blurSize));

  ref_ptr<ShaderInput1f> blurSigma = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  blurSigma->setUniformData(sigma);
  filter->joinShaderInput(ref_ptr<ShaderInput>::cast(blurSigma));

  // first downsample the moments texture
  filter->addFilter(ref_ptr<Filter>::manage(new Filter("sampling.downsample", 0.5)));
  if(downsampleTwice) {
    filter->addFilter(ref_ptr<Filter>::manage(new Filter("sampling.downsample", 0.5)));
  }
  filter->addFilter(ref_ptr<Filter>::manage(new Filter("blur.horizontal")));
  filter->addFilter(ref_ptr<Filter>::manage(new Filter("blur.vertical")));

  ref_ptr<StateNode> blurNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(filter)));
  root->addChild(blurNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(blurNode.get());
  filter->createShader(shaderConfigurer.cfg());

  string treePath_ = (treePath.empty() ? "blur" : treePath + ".blur");
  app->addShaderInput(treePath_,
      ref_ptr<ShaderInput>::cast(blurSize),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
      "Width and height of blur kernel.");
  app->addShaderInput(treePath_,
      ref_ptr<ShaderInput>::cast(blurSigma),
      Vec4f(0.0f), Vec4f(99.0f), Vec4i(2),
      "Blur sigma.");

  app->connect(Application::RESIZE_EVENT, ref_ptr<EventHandler>::manage(
      new ResizableResizer(ref_ptr<Resizable>::cast(filter))));

  return filter;
}

ref_ptr<DepthOfField> createDoFState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<Texture> &depthInput,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<DepthOfField> dof =
      ref_ptr<DepthOfField>::manage(new DepthOfField(input,blurInput,depthInput));

  app->addShaderInput("Depth of Field",
      ref_ptr<ShaderInput>::cast(dof->focalDistance()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "distance to point with max sharpness in NDC space.");
  app->addShaderInput("Depth of Field",
      ref_ptr<ShaderInput>::cast(dof->focalWidth()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "Inner and outer focal width. Between the original and the blurred image are linear combined.");

  ref_ptr<StateNode> node = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(dof)));
  root->addChild(node);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  dof->createShader(shaderConfigurer.cfg());

  return dof;
}

ref_ptr<Tonemap> createTonemapState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<Tonemap> tonemap =
      ref_ptr<Tonemap>::manage(new Tonemap(input, blurInput));

  app->addShaderInput("Tonemap",
      ref_ptr<ShaderInput>::cast(tonemap->blurAmount()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "mix factor for input and blurred input.");
  app->addShaderInput("Tonemap",
      ref_ptr<ShaderInput>::cast(tonemap->exposure()),
      Vec4f(0.0f), Vec4f(50.0f), Vec4i(2),
      "overall exposure factor.");
  app->addShaderInput("Tonemap",
      ref_ptr<ShaderInput>::cast(tonemap->gamma()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "gamma correction factor.");
  app->addShaderInput("Tonemap.streamRays",
      ref_ptr<ShaderInput>::cast(tonemap->effectAmount()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "streaming rays factor.");
  app->addShaderInput("Tonemap.streamRays",
      ref_ptr<ShaderInput>::cast(tonemap->radialBlurSamples()),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
      "number of radial blur samples for streaming rays.");
  app->addShaderInput("Tonemap.streamRays",
      ref_ptr<ShaderInput>::cast(tonemap->radialBlurStartScale()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "initial scale of texture coordinates for streaming rays.");
  app->addShaderInput("Tonemap.streamRays",
      ref_ptr<ShaderInput>::cast(tonemap->radialBlurScaleMul()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "scale factor of texture coordinates for streaming rays.");
  app->addShaderInput("Tonemap.vignette",
      ref_ptr<ShaderInput>::cast(tonemap->vignetteInner()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "inner distance for vignette effect.");
  app->addShaderInput("Tonemap.vignette",
      ref_ptr<ShaderInput>::cast(tonemap->vignetteOuter()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "outer distance for vignette effect.");

  ref_ptr<StateNode> node = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(tonemap)));
  root->addChild(node);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  tonemap->createShader(shaderConfigurer.cfg());

  return tonemap;
}

ref_ptr<FullscreenPass> createAAState(
    QtApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<FullscreenPass> aa =
      ref_ptr<FullscreenPass>::manage(new FullscreenPass("fxaa"));

  ref_ptr<TextureState> texState;
  texState = ref_ptr<TextureState>::manage(new TextureState(input, "inputTexture"));
  aa->joinStatesFront(ref_ptr<State>::cast(texState));

  ref_ptr<ShaderInput1f> spanMax = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("spanMax"));
  spanMax->setUniformData(8.0f);
  aa->joinShaderInput(ref_ptr<ShaderInput>::cast(spanMax));

  ref_ptr<ShaderInput1f> reduceMul = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("reduceMul"));
  reduceMul->setUniformData(1.0f/8.0f);
  aa->joinShaderInput(ref_ptr<ShaderInput>::cast(reduceMul));

  ref_ptr<ShaderInput1f> reduceMin = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("reduceMin"));
  reduceMin->setUniformData(1.0f/128.0f);
  aa->joinShaderInput(ref_ptr<ShaderInput>::cast(reduceMin));

  ref_ptr<ShaderInput3f> luma = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("luma"));
  luma->setUniformData(Vec3f(0.299, 0.587, 0.114));
  aa->joinShaderInput(ref_ptr<ShaderInput>::cast(luma));

  app->addShaderInput("FXAA",
      ref_ptr<ShaderInput>::cast(spanMax),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2), "");
  app->addShaderInput("FXAA",
      ref_ptr<ShaderInput>::cast(reduceMul),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2), "");
  app->addShaderInput("FXAA",
      ref_ptr<ShaderInput>::cast(reduceMin),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2), "");
  app->addShaderInput("FXAA",
      ref_ptr<ShaderInput>::cast(luma),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2), "");

  ref_ptr<StateNode> node = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(aa)));
  root->addChild(node);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  aa->createShader(shaderConfigurer.cfg());

  return aa;
}

/////////////////////////////////////
//// Background/Foreground States
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createBackground(
    QtApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->setDrawBufferOntop(tex, baseAttachment);

  ref_ptr<StateNode> root = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));

  // no depth writing
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthWrite(GL_FALSE);
  fboState->joinStates(ref_ptr<State>::cast(depthState));

  return root;
}

// Creates sky box mesh
ref_ptr<SkyScattering> createSky(QtApplication *app, const ref_ptr<StateNode> &root)
{
  ref_ptr<SkyScattering> sky = ref_ptr<SkyScattering>::manage(new SkyScattering);
  sky->setSunElevation(0.8, 20.0, -20.0);
  //sky->set_updateInterval(1000.0);
  //sky->set_timeScale(0.0001);
  sky->set_dayTime(0.5); // middle of the day
  sky->setEarth();

  ref_ptr<TextureCube> milkyway = TextureLoader::loadCube(
      "res/textures/cube-maps/milkyway.png", GL_FALSE, GL_FALSE, GL_RGB);

  RenderState::get()->textureChannel().push(GL_TEXTURE7);
  RenderState::get()->textureBind().push(7,
      TextureBind(milkyway->targetType(), milkyway->id()));
  milkyway->set_wrapping(GL_CLAMP_TO_EDGE);
  sky->setStarMap(ref_ptr<Texture>::cast(milkyway));
  sky->setStarMapBrightness(1.0f);
  RenderState::get()->textureBind().pop(7);
  RenderState::get()->textureChannel().pop();

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(sky)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  sky->createShader(shaderConfigurer.cfg());

  return sky;
}

ref_ptr<SkyBox> createSkyCube(
    QtApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<SkyBox> mesh = ref_ptr<SkyBox>::manage(new SkyBox);
  mesh->setCubeMap(reflectionMap);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  mesh->createShader(shaderConfigurer.cfg());

  return mesh;
}

ref_ptr<ParticleRain> createRain(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numParticles)
{
  ref_ptr<ParticleRain> particles =
      ref_ptr<ParticleRain>::manage(new ParticleRain(numParticles));
  particles->set_depthTexture(depthTexture);
  //particles->loadIntensityTextureArray(
  //    "res/textures/rainTextures", "cv[0-9]+_vPositive_[0-9]+\\.dds");
  //particles->loadIntensityTexture("res/textures/rainTextures/cv0_vPositive_0000.dds");
  particles->loadIntensityTexture("res/textures/splats/flare.jpg");
  particles->createBuffer();

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(particles)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  particles->createShader(shaderConfigurer.cfg());

  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->gravity()),
      Vec4f(-100.0f), Vec4f(100.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->dampingFactor()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->noiseFactor()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->cloudPosition()),
      Vec4f(-10.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->cloudRadius()),
      Vec4f(0.1f), Vec4f(100.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->particleMass()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->particleSize()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->streakSize()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->brightness()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "");
  app->addShaderInput("RainParticles",
      ref_ptr<ShaderInput>::cast(particles->softScale()),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "");

  return particles;
}

ref_ptr<ParticleSnow> createSnow(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numSnowFlakes)
{
  ref_ptr<ParticleSnow> particles =
      ref_ptr<ParticleSnow>::manage(new ParticleSnow(numSnowFlakes));
  ref_ptr<Texture> tex = TextureLoader::load("res/textures/splats/flare.jpg");
  particles->set_particleTexture(tex);
  particles->set_depthTexture(depthTexture);
  particles->createBuffer();

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(particles)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  particles->createShader(shaderConfigurer.cfg());

  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->gravity()),
      Vec4f(-100.0f), Vec4f(100.0f), Vec4i(2),
      "");
  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->dampingFactor()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->noiseFactor()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->cloudPosition()),
      Vec4f(-10.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->cloudRadius()),
      Vec4f(0.1f), Vec4f(100.0f), Vec4i(2),
      "");
  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->particleMass()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->particleSize()),
      Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
      "");
  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->brightness()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "");
  app->addShaderInput("SnowParticles",
      ref_ptr<ShaderInput>::cast(particles->softScale()),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "");

  return particles;
}

ref_ptr<VolumetricFog> createVolumeFog(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<Texture> &tBufferColor,
    const ref_ptr<Texture> &tBufferDepth,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<VolumetricFog> fog =
      ref_ptr<VolumetricFog>::manage(new VolumetricFog);
  fog->set_gDepthTexture(depthTexture);
  fog->set_tBuffer(tBufferColor,tBufferDepth);

  ref_ptr<StateNode> node = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fog)));
  root->addChild(node);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  fog->createShader(shaderConfigurer.cfg());

  app->addShaderInput("Fog.volume",
      ref_ptr<ShaderInput>::cast(fog->fogDistance()),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "inner and outer fog distance to camera.");

  return fog;
}
ref_ptr<VolumetricFog> createVolumeFog(
    QtApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root)
{
  return regen::createVolumeFog(app,depthTexture,
      ref_ptr<Texture>(), ref_ptr<Texture>(),
      root);
}

ref_ptr<DistanceFog> createDistanceFog(
    QtApplication *app,
    const Vec3f &fogColor,
    const ref_ptr<TextureCube> &skyColor,
    const ref_ptr<Texture> &gDepth,
    const ref_ptr<Texture> &tBufferColor,
    const ref_ptr<Texture> &tBufferDepth,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<DistanceFog> fog =
      ref_ptr<DistanceFog>::manage(new DistanceFog);
  fog->set_gBuffer(gDepth);
  fog->set_tBuffer(tBufferColor,tBufferDepth);
  fog->set_skyColor(skyColor);
  fog->fogColor()->setVertex3f(0,fogColor);

  ref_ptr<StateNode> node = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fog)));
  root->addChild(node);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  fog->createShader(shaderConfigurer.cfg());

  app->addShaderInput("Fog.distance",
      ref_ptr<ShaderInput>::cast(fog->fogDistance()),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "inner and outer fog distance to camera.");
  app->addShaderInput("Fog.distance",
      ref_ptr<ShaderInput>::cast(fog->fogDensity()),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "constant fog density.");

  return fog;
}
ref_ptr<DistanceFog> createDistanceFog(
    QtApplication *app,
    const Vec3f &fogColor,
    const ref_ptr<TextureCube> &skyColor,
    const ref_ptr<Texture> &gDepth,
    const ref_ptr<StateNode> &root)
{
  return regen::createDistanceFog(app,fogColor,skyColor,gDepth,
      ref_ptr<Texture>(), ref_ptr<Texture>(),
      root);
}

/////////////////////////////////////
//// Shading States
/////////////////////////////////////

// Creates deferred shading state and add to render tree
ref_ptr<DeferredShading> createShadingPass(
    QtApplication *app,
    const ref_ptr<FrameBufferObject> &gBuffer,
    const ref_ptr<StateNode> &root,
    ShadowMap::FilterMode shadowFiltering,
    GLboolean useAmbientLight)
{
  ref_ptr<DeferredShading> shading =
      ref_ptr<DeferredShading>::manage(new DeferredShading);

  if(useAmbientLight) {
    shading->setUseAmbientLight();
    app->addShaderInput("Shading",
        ref_ptr<ShaderInput>::cast(shading->ambientLight()),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(3),
        "the ambient light.");
  }
  shading->dirShadowState()->setShadowFiltering(shadowFiltering);
  shading->pointShadowState()->setShadowFiltering(shadowFiltering);
  shading->spotShadowState()->setShadowFiltering(shadowFiltering);

  ref_ptr<Texture> gDiffuseTexture = gBuffer->colorBuffer()[0];
  ref_ptr<Texture> gSpecularTexture = gBuffer->colorBuffer()[2];
  ref_ptr<Texture> gNorWorldTexture = gBuffer->colorBuffer()[3];
  ref_ptr<Texture> gDepthTexture = gBuffer->depthTexture();
  shading->set_gBuffer(
      gDepthTexture, gNorWorldTexture,
      gDiffuseTexture, gSpecularTexture);

  ref_ptr<FBOState> fboState =
      ref_ptr<FBOState>::manage(new FBOState(gBuffer));
  fboState->setDrawBufferUpdate(gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  shading->joinStatesFront(ref_ptr<State>::manage(new FramebufferClear));
  shading->joinStatesFront(ref_ptr<State>::cast(fboState));

  // no depth testing/writing
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  shading->joinStatesFront(ref_ptr<State>::cast(depthState));

  ref_ptr<StateNode> shadingNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(shading)));
  root->addChild(shadingNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(shadingNode.get());
  shading->createShader(shaderConfigurer.cfg());

  return shading;
}

ref_ptr<Light> createPointLight(QtApplication *app,
    const Vec3f &pos,
    const Vec3f &diffuse,
    const Vec2f &radius)
{
  ref_ptr<Light> pointLight = ref_ptr<Light>::manage(new Light(Light::POINT));
  pointLight->position()->setVertex3f(0,pos);
  pointLight->diffuse()->setVertex3f(0,diffuse);
  pointLight->radius()->setVertex2f(0,radius);

  app->addShaderInput("Shading.point",
      ref_ptr<ShaderInput>::cast(pointLight->position()),
      Vec4f(-100.0f), Vec4f(100.0f), Vec4i(2),
      "the world space light position.");
  app->addShaderInput("Shading.point",
      ref_ptr<ShaderInput>::cast(pointLight->radius()),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "inner and outer light radius.");
  app->addShaderInput("Shading.point",
      ref_ptr<ShaderInput>::cast(pointLight->diffuse()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "diffuse light color.");
  app->addShaderInput("Shading.point",
      ref_ptr<ShaderInput>::cast(pointLight->specular()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "specular light color.");

  return pointLight;
}

ref_ptr<Light> createSpotLight(QtApplication *app,
    const Vec3f &pos,
    const Vec3f &dir,
    const Vec3f &diffuse,
    const Vec2f &radius,
    const Vec2f &coneAngles)
{
  ref_ptr<Light> l = ref_ptr<Light>::manage(new Light(Light::SPOT));
  l->position()->setVertex3f(0,pos);
  l->direction()->setVertex3f(0,dir);
  l->diffuse()->setVertex3f(0,diffuse);
  l->radius()->setVertex2f(0,radius);
  l->set_innerConeAngle(coneAngles.x);
  l->set_outerConeAngle(coneAngles.y);

  app->addShaderInput("Shading.spot",
      ref_ptr<ShaderInput>::cast(l->position()),
      Vec4f(-100.0f), Vec4f(100.0f), Vec4i(2),
      "the world space light position.");
  app->addShaderInput("Shading.spot",
      ref_ptr<ShaderInput>::cast(l->direction()),
      Vec4f(-1.0f), Vec4f(1.0f), Vec4i(2),
      "the light direction.");
  app->addShaderInput("Shading.spot",
      ref_ptr<ShaderInput>::cast(l->coneAngle()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "inner and outer cone angles.");
  app->addShaderInput("Shading.spot",
      ref_ptr<ShaderInput>::cast(l->radius()),
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
      "inner and outer light radius.");
  app->addShaderInput("Shading.spot",
      ref_ptr<ShaderInput>::cast(l->diffuse()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "diffuse light color.");
  app->addShaderInput("Shading.spot",
      ref_ptr<ShaderInput>::cast(l->specular()),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "specular light color.");

  return l;
}

ref_ptr<ShadowMap> createShadow(
    QtApplication *app,
    const ref_ptr<Light> &light,
    const ref_ptr<Camera> &cam,
    ShadowMap::Config cfg)
{
  ref_ptr<ShadowMap> sm = ref_ptr<ShadowMap>::manage(
      new ShadowMap(light, cam, cfg));
  return sm;
}

/////////////////////////////////////
//// Mesh Factory
/////////////////////////////////////

// Loads Meshes from File using Assimp. Optionally Bone animations are loaded.
list<MeshData> createAssimpMesh(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const string &modelFile,
    const string &texturePath,
    const Mat4f &meshRotation,
    const Vec3f &meshTranslation,
    const Mat4f &meshScaling,
    const BoneAnimRange *animRanges,
    GLuint numAnimationRanges,
    GLdouble ticksPerSecond,
    const string &shaderKey)
{
  AssimpImporter importer(modelFile, texturePath);
  list< ref_ptr<Mesh> > meshes = importer.loadMeshes(meshScaling);
  NodeAnimation *boneAnim=NULL;

  if(animRanges && numAnimationRanges>0) {
    boneAnim = importer.loadNodeAnimation(
        GL_TRUE,
        NodeAnimation::BEHAVIOR_LINEAR,
        NodeAnimation::BEHAVIOR_LINEAR,
        ticksPerSecond);
    boneAnim->stopAnimation();
  }
  ref_ptr<ModelTransformation> modelMat =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelMat->set_modelMat(meshRotation, 0.0f);
  modelMat->translate(meshTranslation, 0.0f);

  list<MeshData> ret;

  for(list< ref_ptr<Mesh> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<Mesh> &mesh = *it;

    if(boneAnim) {
      list< ref_ptr<AnimationNode> > meshBones =
          importer.loadMeshBones(mesh.get(), boneAnim);
      ref_ptr<Bones> bonesState = ref_ptr<Bones>::manage(new Bones(
          meshBones, importer.numBoneWeights(mesh.get())));
      mesh->joinStates(ref_ptr<State>::cast(bonesState));
    }

    ref_ptr<Material> material = importer.getMeshMaterial(mesh.get());
    mesh->joinStates(ref_ptr<State>::cast(material));
    mesh->joinStates(ref_ptr<State>::cast(modelMat));

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh->joinStates(ref_ptr<State>::cast(shaderState));

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(mesh)));
    root->addChild(meshNode);

    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), shaderKey);

    MeshData d;
    d.material_ = material;
    d.mesh_ = mesh;
    d.shader_ = shaderState;
    d.node_ = meshNode;
    ret.push_back(d);
  }

  if(boneAnim) {
    ref_ptr<NodeAnimation> anim = ref_ptr<NodeAnimation>::manage(boneAnim);
    ref_ptr<EventHandler> animStopped = ref_ptr<EventHandler>::manage(
        new AnimationRangeUpdater(anim,animRanges,numAnimationRanges));
    boneAnim->connect(Animation::ANIMATION_STOPPED, animStopped);
    boneAnim->startAnimation();

    EventData evData;
    evData.eventID = Animation::ANIMATION_STOPPED;
    animStopped->call(boneAnim, &evData);
  }

  return ret;
}

void createConeMesh(QtApplication *app, const ref_ptr<StateNode> &root)
{
  ConeClosed::Config cfg;
  cfg.levelOfDetail = 3;
  cfg.isBaseRequired = GL_TRUE;
  cfg.isNormalRequired = GL_TRUE;
  cfg.height = 3.0;
  cfg.radius = 1.0;
  ref_ptr<Mesh> mesh = ref_ptr<Mesh>::manage(new ConeClosed(cfg));

  ref_ptr<ModelTransformation> modelMat =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);
  mesh->joinStates(ref_ptr<State>::cast(modelMat));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->ambient()->setUniformData(Vec3f(0.3f));
  material->diffuse()->setUniformData(Vec3f(0.7f));
  mesh->joinStates(ref_ptr<State>::cast(material));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  mesh->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "mesh");
}

// Creates simple floor mesh
MeshData createFloorMesh(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const GLfloat &height,
    const Vec3f &posScale,
    const Vec2f &texcoScale,
    TextureState::TransferTexco transferMode,
    GLboolean useTess)
{
  Rectangle::Config meshCfg;
  meshCfg.levelOfDetail = 0;
  meshCfg.isTexcoRequired = GL_TRUE;
  meshCfg.isNormalRequired = GL_TRUE;
  meshCfg.isTangentRequired = GL_TRUE;
  meshCfg.centerAtOrigin = GL_TRUE;
  meshCfg.rotation = Vec3f(0.0*M_PI, 0.0*M_PI, 1.0*M_PI);
  meshCfg.posScale = posScale;
  meshCfg.texcoScale = texcoScale;
  ref_ptr<Mesh> floor = ref_ptr<Mesh>::manage(new Rectangle(meshCfg));

  ref_ptr<ModelTransformation> modelMat =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelMat->translate(Vec3f(0.0f, height, 0.0f), 0.0f);
  modelMat->setConstantUniforms(GL_TRUE);
  floor->joinStates(ref_ptr<State>::cast(modelMat));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->ambient()->setUniformData(Vec3f(0.3f));
  material->diffuse()->setUniformData(Vec3f(0.7f));
  material->setConstantUniforms(GL_TRUE);
  floor->joinStates(ref_ptr<State>::cast(material));

  // setup texco transfer uniforms
  if(useTess) {
    ref_ptr<TesselationState> tess =
        ref_ptr<TesselationState>::manage(new TesselationState(3));
    tess->set_lodMetric(TesselationState::CAMERA_DISTANCE_INVERSE);
    tess->lodFactor()->setVertex1f(0,1.0f);
    floor->set_primitive(GL_PATCHES);
    floor->joinStates(ref_ptr<State>::cast(tess));
    app->addShaderInput("Meshes.Floor",
        ref_ptr<ShaderInput>::cast(tess->lodFactor()),
        Vec4f(0.0f), Vec4f(100.0f), Vec4i(2),
        "Tesselation has a range for its levels, maxLevel is currently 64.0.");
  }
  else if(transferMode==TextureState::TRANSFER_TEXCO_PARALLAX) {
    ref_ptr<ShaderInput1f> bias =
        ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("parallaxBias"));
    bias->setUniformData(0.015);
    material->joinShaderInput(ref_ptr<ShaderInput>::cast(bias));
    app->addShaderInput("Meshes.Floor",
        ref_ptr<ShaderInput>::cast(bias),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Parallax-Mapping bias.");

    ref_ptr<ShaderInput1f> scale =
        ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("parallaxScale"));
    scale->setUniformData(0.03);
    material->joinShaderInput(ref_ptr<ShaderInput>::cast(scale));
    app->addShaderInput("Meshes.Floor",
        ref_ptr<ShaderInput>::cast(scale),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Parallax-Mapping scale.");
  }
  else if(transferMode==TextureState::TRANSFER_TEXCO_PARALLAX_OCC) {
    ref_ptr<ShaderInput1f> scale =
        ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("parallaxScale"));
    scale->setUniformData(0.03);
    material->joinShaderInput(ref_ptr<ShaderInput>::cast(scale));
    app->addShaderInput("Meshes.Floor",
        ref_ptr<ShaderInput>::cast(scale),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Parallax-Occlusion-Mapping scale.");

    ref_ptr<ShaderInput1i> steps =
        ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("parallaxSteps"));
    steps->setUniformData(10);
    material->joinShaderInput(ref_ptr<ShaderInput>::cast(steps));
    app->addShaderInput("Meshes.Floor",
        ref_ptr<ShaderInput>::cast(steps),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(0),
        "Parallax-Occlusion-Mapping steps.");
  }
  else if(transferMode==TextureState::TRANSFER_TEXCO_RELIEF) {
    ref_ptr<ShaderInput1f> scale =
        ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("reliefScale"));
    scale->setUniformData(0.03);
    material->joinShaderInput(ref_ptr<ShaderInput>::cast(scale));
    app->addShaderInput("Meshes.Floor",
        ref_ptr<ShaderInput>::cast(scale),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Relief-Mapping scale.");

    ref_ptr<ShaderInput1i> linearSteps =
        ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("reliefLinearSteps"));
    linearSteps->setUniformData(10);
    material->joinShaderInput(ref_ptr<ShaderInput>::cast(linearSteps));
    app->addShaderInput("Meshes.Floor",
        ref_ptr<ShaderInput>::cast(linearSteps),
        Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
        "Relief-Mapping linear steps.");

    ref_ptr<ShaderInput1i> binarySteps =
        ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("reliefBinarySteps"));
    binarySteps->setUniformData(2);
    material->joinShaderInput(ref_ptr<ShaderInput>::cast(binarySteps));
    app->addShaderInput("Meshes.Floor",
        ref_ptr<ShaderInput>::cast(binarySteps),
        Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
        "Relief-Mapping binary steps.");
  }
  //material->shaderDefine("DEPTH_CORRECT", "TRUE");

  //ref_ptr<Texture> colMap_ = TextureLoader::load("res/textures/relief/color2.jpg");
  //ref_ptr<Texture> norMap_ = TextureLoader::load("res/textures/relief/normal.png");
  //ref_ptr<Texture> heightMap_ = TextureLoader::load("res/textures/relief/height.png");
  ref_ptr<Texture> colMap_ = TextureLoader::load("res/textures/brick/color.jpg");
  ref_ptr<Texture> norMap_ = TextureLoader::load("res/textures/brick/normal.jpg");
  ref_ptr<Texture> heightMap_ = TextureLoader::load("res/textures/brick/height.jpg");

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
      new TextureState(colMap_, "colorTexture"));
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_texcoTransfer(transferMode);
  texState->set_blendMode(BLEND_MODE_SRC);
  material->joinStates(ref_ptr<State>::cast(texState));

  texState = ref_ptr<TextureState>::manage(
      new TextureState(norMap_, "normalTexture"));
  texState->set_mapTo(TextureState::MAP_TO_NORMAL);
  texState->set_texcoTransfer(transferMode);
  texState->set_texelTransferKey("textures.normalTBNTransfer");
  texState->set_blendMode(BLEND_MODE_SRC);
  material->joinStates(ref_ptr<State>::cast(texState));

  texState = ref_ptr<TextureState>::manage(
      new TextureState(heightMap_, "heightTexture"));
  if(useTess) {
    texState->set_blendMode(BLEND_MODE_ADD);
    texState->set_mapTo(TextureState::MAP_TO_HEIGHT);
    texState->set_texelTransferFunction(
        "void brickHeight(inout vec4 t) { t.x = t.x*0.05 - 0.05; }",
        "brickHeight");
  }
  material->joinStates(ref_ptr<State>::cast(texState));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  floor->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(floor)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "mesh");

  MeshData d;
  d.mesh_ = floor;
  d.shader_ = shaderState;
  d.node_ = meshNode;
  return d;
}

MeshData createBox(QtApplication *app, const ref_ptr<StateNode> &root)
{
    Box::Config cubeConfig;
    cubeConfig.texcoMode = Box::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 0.5f, 0.5f);
    ref_ptr<Mesh> mesh = ref_ptr<Mesh>::manage(new Box(cubeConfig));

    ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::manage(
        new ModelTransformation);
    modelMat->translate(Vec3f(-2.0f, 0.75f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);
    mesh->joinStates(ref_ptr<State>::cast(modelMat));

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh->joinStates(ref_ptr<State>::cast(shaderState));

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(mesh)));
    root->addChild(meshNode);

    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), "mesh");

    MeshData d;
    d.mesh_ = mesh;
    d.shader_ = shaderState;
    d.node_ = meshNode;
    return d;
}

ref_ptr<Mesh> createSphere(QtApplication *app, const ref_ptr<StateNode> &root)
{
    Sphere::Config sphereConfig;
    sphereConfig.texcoMode = Sphere::TEXCO_MODE_NONE;
    ref_ptr<Mesh> mesh = ref_ptr<Mesh>::manage(new Sphere(sphereConfig));

    ref_ptr<ModelTransformation> modelMat =
        ref_ptr<ModelTransformation>::manage(new ModelTransformation);
    modelMat->translate(Vec3f(0.0f, 0.5f, 0.0f), 0.0f);
    mesh->joinStates(ref_ptr<State>::cast(modelMat));

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_ruby();
    mesh->joinStates(ref_ptr<State>::cast(material));

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh->joinStates(ref_ptr<State>::cast(shaderState));

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(mesh)));
    root->addChild(meshNode);

    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), "mesh");

    return mesh;
}

ref_ptr<Mesh> createQuad(QtApplication *app, const ref_ptr<StateNode> &root)
{
  Rectangle::Config quadConfig;
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_TRUE;
  quadConfig.isTangentRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.0*M_PI, 0.0*M_PI, 1.0*M_PI);
  quadConfig.posScale = Vec3f(10.0f);
  quadConfig.texcoScale = Vec2f(2.0f);
  ref_ptr<Mesh> mesh =
      ref_ptr<Mesh>::manage(new Rectangle(quadConfig));

  ref_ptr<ModelTransformation> modelMat =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelMat->translate(Vec3f(0.0f, -0.5f, 0.0f), 0.0f);
  mesh->joinStates(ref_ptr<State>::cast(modelMat));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->set_chrome();
  material->specular()->setUniformData(Vec3f(0.0f));
  mesh->joinStates(ref_ptr<State>::cast(material));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  mesh->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "mesh");

  return mesh;
}

ref_ptr<Mesh> createReflectionSphere(
    QtApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root)
{
  SphereSprite::Config cfg;
  GLfloat radi[] = { 1.0 };
  Vec3f pos[] = { Vec3f(0.0f) };
  cfg.radius = radi;
  cfg.position = pos;
  cfg.sphereCount = 1;
  ref_ptr<SphereSprite> mesh = ref_ptr<SphereSprite>::manage(new SphereSprite(cfg));

  ref_ptr<ModelTransformation> modelMat =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelMat->translate(Vec3f(0.0f), 0.0f);
  mesh->joinStatesFront(ref_ptr<State>::cast(modelMat));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  mesh->joinStatesFront(ref_ptr<State>::cast(material));

  ref_ptr<TextureState> refractionTexture = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(reflectionMap)));
  refractionTexture->set_mapTo(TextureState::MAP_TO_COLOR);
  refractionTexture->set_blendMode(BLEND_MODE_SRC);
  refractionTexture->set_mapping(TextureState::MAPPING_REFRACTION);
  material->joinStates(ref_ptr<State>::cast(refractionTexture));

  ref_ptr<TextureState> reflectionTexture = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(reflectionMap)));
  reflectionTexture->set_mapTo(TextureState::MAP_TO_COLOR);
  reflectionTexture->set_blendMode(BLEND_MODE_MIX);
  reflectionTexture->set_blendFactor(0.35f);
  reflectionTexture->set_mapping(TextureState::MAPPING_REFLECTION);
  material->joinStates(ref_ptr<State>::cast(reflectionTexture));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  mesh->createShader(shaderConfigurer.cfg());

  return ref_ptr<Mesh>::cast(mesh);
}

/////////////////////////////////////
//// GUI Factory
/////////////////////////////////////

class UpdateFPS : public Animation
{
public:
  UpdateFPS(const ref_ptr<TextureMappedText> &widget)
  : Animation(GL_TRUE,GL_FALSE),
    widget_(widget), frameCounter_(0), sumDtMiliseconds_(0.0f)
  {}

  void glAnimate(RenderState *rs, GLdouble dt) {
    frameCounter_ += 1;
    sumDtMiliseconds_ += dt;

    if (sumDtMiliseconds_ > 1000.0) {
      fps_ = (GLint) (frameCounter_*1000.0/sumDtMiliseconds_);
      sumDtMiliseconds_ = 0;
      frameCounter_ = 0;

      wstringstream ss;
      ss << fps_ << " FPS";
      widget_->set_value(ss.str());
    }
  }

private:
  ref_ptr<TextureMappedText> widget_;
  GLuint frameCounter_;
  GLint fps_;
  GLdouble sumDtMiliseconds_;
};

// Creates GUI widgets displaying the current FPS
Animation* createFPSWidget(QtApplication *app, const ref_ptr<StateNode> &root)
{
  FreeTypeFont& font = FontManager::get().getFont("res/fonts/obelix.ttf", 16, 96);

  RenderState::get()->textureChannel().push(GL_TEXTURE7);
  RenderState::get()->textureBind().push(7,
      TextureBind(font.texture()->targetType(), font.texture()->id()));
  font.texture()->set_filter(GL_LINEAR,GL_LINEAR);
  RenderState::get()->textureBind().pop(7);
  RenderState::get()->textureChannel().pop();

  ref_ptr<TextureMappedText> widget =
      ref_ptr<TextureMappedText>::manage(new TextureMappedText(font, 16.0));
  widget->set_color(Vec4f(0.97,0.86,0.77,0.95));
  widget->set_value(L"0 FPS");

  ref_ptr<ModelTransformation> modelTransformation =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelTransformation->translate( Vec3f( 12.0, 8.0, 0.0 ), 0.0f );
  widget->joinStatesFront(ref_ptr<State>::cast(modelTransformation));

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(widget)));
  root->addChild(widgetNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(widgetNode.get());
  widget->createShader(shaderConfigurer.cfg());

  return new UpdateFPS(widget);
}

void createLogoWidget(QtApplication *app, const ref_ptr<StateNode> &root)
{
  ref_ptr<Texture> logoTex = TextureLoader::load("res/textures/logo/logo.png");
  Vec2f size(logoTex->width()*0.25, logoTex->height()*0.25);
  //Vec2f size(100.0, 100.0);

  Rectangle::Config cfg;
  cfg.levelOfDetail = 0;
  cfg.isTexcoRequired = GL_TRUE;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.centerAtOrigin = GL_FALSE;
  cfg.posScale = Vec3f(size.x, 1.0, size.y);
  cfg.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
  cfg.texcoScale = Vec2f(-1.0,1.0);
  cfg.translation = Vec3f(0.0f,0.0f,0.0f);
  ref_ptr<Mesh> widget =
      ref_ptr<Mesh>::manage(new Rectangle(cfg));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  widget->joinStates(ref_ptr<State>::cast(material));

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(logoTex));
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_blendMode(BLEND_MODE_SRC);
  material->joinStates(ref_ptr<State>::cast(texState));

  ref_ptr<ModelTransformation> modelTransformation =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelTransformation->translate( Vec3f(0.0,-10.0,0.0 ), 0.0f );
  widget->joinStates(ref_ptr<State>::cast(modelTransformation));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  widget->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(widget)));
  root->addChild(widgetNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.define("INVERT_Y", "TRUE");
  shaderConfigurer.addNode(widgetNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "gui");
}

void createTextureWidget(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const ref_ptr<Texture> &tex,
    const Vec2ui &pos,
    const GLfloat &size)
{
  Rectangle::Config cfg;
  cfg.levelOfDetail = 0;
  cfg.isTexcoRequired = GL_TRUE;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.centerAtOrigin = GL_FALSE;
  cfg.posScale = Vec3f(size);
  cfg.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
  cfg.texcoScale = Vec2f(1.0);
  cfg.translation = Vec3f(0.0f,-size,0.0f);
  ref_ptr<Mesh> widget =
      ref_ptr<Mesh>::manage(new Rectangle(cfg));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  widget->joinStates(ref_ptr<State>::cast(material));

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(tex));
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_blendMode(BLEND_MODE_SRC);
  texState->set_texelTransferFunction(
      "void transferIgnoreAlpha(inout vec4 v) { v.a=1.0; }", "transferIgnoreAlpha");
  material->joinStates(ref_ptr<State>::cast(texState));

  ref_ptr<ModelTransformation> modelTransformation =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelTransformation->translate( Vec3f( pos.x, pos.y, 0.0 ), 0.0f );
  widget->joinStates(ref_ptr<State>::cast(modelTransformation));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  widget->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(widget)));
  root->addChild(widgetNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(widgetNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "gui");
}

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(QtApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<StateNode> guiRoot = ref_ptr<StateNode>::manage(new StateNode);
  ref_ptr<State> guiState = guiRoot->state();

  // enable fbo and call DrawBuffer()
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->setDrawBufferOntop(tex,baseAttachment);
  guiState->joinStates(ref_ptr<State>::cast(fboState));
  // alpha blend GUI widgets with scene
  guiState->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  // no depth testing for gui
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  guiState->joinStates(ref_ptr<State>::cast(depthState));

  return guiRoot;
}

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(QtApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum baseAttachment)
{
  ref_ptr<StateNode> guiRoot = ref_ptr<StateNode>::manage(new StateNode);

  // enable fbo and call DrawBuffer()
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBuffer(baseAttachment);
  guiRoot->state()->joinStates(ref_ptr<State>::cast(fboState));
  // alpha blend GUI widgets with scene
  guiRoot->state()->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  // no depth testing for gui
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  guiRoot->state()->joinStates(ref_ptr<State>::cast(depthState));

  return guiRoot;
}

}
