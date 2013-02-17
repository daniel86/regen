/*
 * factory.h
 *
 *  Created on: 17.02.2013
 *      Author: daniel
 */

#ifndef FACTORY_H_
#define FACTORY_H_

#include <applications/fltk-ogle-application.h>
#include <applications/application-config.h>
#include <applications/test-camera-manipulator.h>
#include <ogle/config.h>

#include <ogle/utility/font-manager.h>

#include <ogle/render-tree/shader-configurer.h>

#include <ogle/meshes/rectangle.h>
#include <ogle/meshes/sky.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/texture-mapped-text.h>
#include <ogle/meshes/precipitation-particles.h>

#include <ogle/states/fbo-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/blit-state.h>
#include <ogle/states/material-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/shading.h>
#include <ogle/states/light-shafts.h>
#include <ogle/states/blur-state.h>
#include <ogle/states/depth-of-field.h>
#include <ogle/states/tonemap.h>
#include <ogle/states/anti-aliasing.h>
#include <ogle/states/transparency-state.h>
#include <ogle/states/assimp-importer.h>
#include <ogle/states/tesselation-state.h>
#include <ogle/states/picking.h>

#include <ogle/textures/texture-loader.h>

#include <ogle/animations/animation-manager.h>
#include <ogle/animations/mesh-animation.h>

struct BoneAnimRange {
  string name;
  Vec2d range;
};

class SortByModelMatrix : public State
{
public:
  SortByModelMatrix(ref_ptr<StateNode> &n, ref_ptr<PerspectiveCamera> &cam, GLboolean frontToBack)
  : State(), n_(n), comparator_(cam,frontToBack) {}

  virtual void enable(RenderState *state) {
    n_->childs().sort(comparator_);
  }
protected:
  ref_ptr<StateNode> n_;
  NodeEyeDepthComparator comparator_;
};

// Resizes Framebuffer texture when the window size changed
class FramebufferResizer : public EventCallable
{
public:
  FramebufferResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
  : EventCallable(), fboState_(fbo), wScale_(wScale), hScale_(hScale) { }

  virtual void call(EventObject *evObject, void*) {
    OGLEApplication *app = (OGLEApplication*)evObject;
    fboState_->resize(app->glWidth()*wScale_, app->glHeight()*hScale_);
  }

protected:
  ref_ptr<FBOState> fboState_;
  GLfloat wScale_, hScale_;
};

// Updates Camera Projection when window size changes
class ProjectionUpdater : public EventCallable
{
public:
  ProjectionUpdater(const ref_ptr<PerspectiveCamera> &cam,
      GLfloat fov, GLfloat near, GLfloat far)
  : EventCallable(), cam_(cam), fov_(fov), near_(near), far_(far) { }

  virtual void call(EventObject *evObject, void*) {
    OGLEApplication *app = (OGLEApplication*)evObject;
    GLfloat aspect = app->glWidth()/(GLfloat)app->glHeight();
    cam_->updateProjection(fov_, near_, far_, aspect);
  }

protected:
  ref_ptr<PerspectiveCamera> cam_;
  GLfloat fov_, near_, far_;
};
class GUIProjectionUpdater : public EventCallable
{
public:
  GUIProjectionUpdater(const ref_ptr<OrthoCamera> &cam)
  : EventCallable(), cam_(cam) { }

  virtual void call(EventObject *evObject, void*) {
    OGLEApplication *app = (OGLEApplication*)evObject;
    cam_->updateProjection(app->glWidth(), app->glHeight());
  }
protected:
  ref_ptr<OrthoCamera> cam_;
};

class FramebufferClear : public State
{
public:
  FramebufferClear() : State() {}
  virtual void enable(RenderState *rs) {
    glClearColor(0.0,0.0,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
  }
};


// create application window and set up OpenGL
ref_ptr<OGLEFltkApplication> initApplication(
    int argc, char** argv, const string &windowTitle)
{
  // create and show application window
  ref_ptr<RenderTree> tree = ref_ptr<RenderTree>::manage(new RenderTree);
  ref_ptr<OGLEFltkApplication> app = ref_ptr<OGLEFltkApplication>::manage(
      new OGLEFltkApplication(tree,argc,argv));
  app->set_windowTitle(windowTitle);
  app->show();

  // set the render state that is used during tree traversal
  tree->set_renderState(ref_ptr<RenderState>::manage(new RenderState));

  return app;
}

ref_ptr<PerspectiveCamera> createPerspectiveCamera(
    OGLEApplication *app,
    GLfloat fov=45.0f,
    GLfloat near=0.1f,
    GLfloat far=200.0f,
    GLboolean useMouseManipulator=GL_TRUE)
{
  ref_ptr<PerspectiveCamera> cam =
      ref_ptr<PerspectiveCamera>::manage(new PerspectiveCamera);

  ref_ptr<ProjectionUpdater> projUpdater =
      ref_ptr<ProjectionUpdater>::manage(new ProjectionUpdater(cam, fov, near, far));
  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventCallable>::cast(projUpdater));
  projUpdater->call(app, NULL);

  if(useMouseManipulator) {
    ref_ptr<TestCamManipulator> camManipulator =
        ref_ptr<TestCamManipulator>::manage(new TestCamManipulator(*app, cam));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));
    camManipulator->setStepLength(0.0f,0.0f);
    camManipulator->set_radius(9.0f, 0.0f);
  }

  return cam;
}

// Blits fbo attachment to screen
void setBlitToScreen(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &texture,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitTexToScreen(fbo, texture, app->glSizePtr(), attachment));
  app->renderTree()->rootNode()->addChild(
      ref_ptr<StateNode>::manage(new StateNode(blitState)));
}
void setBlitToScreen(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, app->glSizePtr(), attachment));
  app->renderTree()->rootNode()->addChild(
      ref_ptr<StateNode>::manage(new StateNode(blitState)));
}

ref_ptr<TextureCube> createStaticReflectionMap(
    OGLEFltkApplication *app,
    const string &file,
    const GLboolean flipBackFace,
    const GLenum textureFormat,
    const GLfloat aniso=2.0f)
{
  ref_ptr<TextureCube> reflectionMap = TextureLoader::loadCube(
      file,flipBackFace,GL_FALSE,textureFormat);
  reflectionMap->set_aniso(aniso);
  reflectionMap->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  reflectionMap->setupMipmaps(GL_DONT_CARE);
  reflectionMap->set_wrapping(GL_CLAMP_TO_EDGE);

  return reflectionMap;
}

class PickerAnimation : public Animation {
public:
  PickerAnimation(
      const ref_ptr<StateNode> &meshNode,
      GLuint maxPickedObjects=999)
  : Animation(), meshNode_(meshNode)
  {
    picker_ = ref_ptr<PickingGeom>::manage(
        new PickingGeom(maxPickedObjects));

    dt_ = 0.0;
    pickInterval_ = 50.0;
  }
  void set_pickInterval(GLdouble interval)
  {
    pickInterval_ = interval;
  }

  virtual GLboolean useGLAnimation() const  { return GL_TRUE; }
  virtual GLboolean useAnimation() const { return GL_FALSE; }
  virtual void animate(GLdouble dt) {}

  virtual void glAnimate(GLdouble dt) {
    dt_ += dt;
    if(dt_ < pickInterval_) { return; }
    dt_ = 0.0;

    // TODO: picker needs mouse position uniform

    const MeshState *lastPicked = picker_->pickedMesh();
    picker_->enable();
    RenderTree::traverse(picker_.get(), meshNode_.get(), dt);
    picker_->disable();
    const MeshState *picked = picker_->pickedMesh();
    if(lastPicked != picked) {
      cout << "Pick selection changed to " << picked << endl;
    }
  }
protected:
  ref_ptr<PickingGeom> picker_;
  ref_ptr<StateNode> meshNode_;
  GLdouble dt_;
  GLdouble pickInterval_;
};

void createPicker(
    const ref_ptr<StateNode> &meshNode,
    GLdouble interval=50.0,
    GLuint maxPickedObjects=999)
{
  ref_ptr<PickerAnimation> pickerAnim = ref_ptr<PickerAnimation>::manage(
      new PickerAnimation(meshNode, maxPickedObjects));
  pickerAnim->set_pickInterval(interval);
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(pickerAnim));
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
    OGLEApplication *app,
    GLfloat gBufferScaleW=1.0,
    GLfloat gBufferScaleH=1.0,
    GLenum colorBufferFormat=GL_RGBA,
    GLenum depthFormat=GL_DEPTH_COMPONENT24)
{
  // diffuse, specular, norWorld
  static const GLenum count[] = { 2, 1, 1 };
  static const GLenum formats[] = { GL_RGBA, GL_RGBA, GL_RGBA };
  static const GLenum internalFormats[] = { colorBufferFormat, GL_RGBA, GL_RGBA };
  static const GLenum clearBuffers[] = {
      GL_COLOR_ATTACHMENT2, // spec
      GL_COLOR_ATTACHMENT3  // norWorld
  };

  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject(
          app->glWidth()*gBufferScaleW,
          app->glHeight()*gBufferScaleH,
          depthFormat));
  ref_ptr<FBOState> gBufferState = ref_ptr<FBOState>::manage(new FBOState(fbo));

  for(GLuint i=0; i<sizeof(count)/sizeof(GLenum); ++i) {
    fbo->addTexture(count[i], formats[i], internalFormats[i]);
    // call glDrawBuffer
    gBufferState->addDrawBuffer(GL_COLOR_ATTACHMENT0+i+1);
  }
  // make sure buffer index of diffuse texture is set to 0
  gBufferState->joinStates(ref_ptr<State>::manage(
      new TextureSetBufferIndex(fbo->colorBuffer()[0], 0)));

  ClearColorData clearData;
  clearData.clearColor = Vec4f(0.0f);
  clearData.colorBuffers = std::vector<GLenum>(
      clearBuffers, clearBuffers + sizeof(clearBuffers)/sizeof(GLenum));
  gBufferState->setClearColor(clearData);
  if(depthFormat!=GL_NONE) {
    gBufferState->setClearDepth();
  }

  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventCallable>::manage(
      new FramebufferResizer(gBufferState,gBufferScaleW,gBufferScaleH)));

  return gBufferState;
}

ref_ptr<TransparencyState> createTBuffer(
    OGLEApplication *app,
    const ref_ptr<PerspectiveCamera> &cam,
    const ref_ptr<Texture> &depthTexture,
    TransparencyMode mode=TRANSPARENCY_MODE_FRONT_TO_BACK,
    GLfloat tBufferScaleW=1.0,
    GLfloat tBufferScaleH=1.0)
{
  TransparencyState *tBufferState = new TransparencyState(
      mode,
      app->glWidth()*tBufferScaleW,
      app->glHeight()*tBufferScaleH,
      depthTexture);

  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventCallable>::manage(
      new FramebufferResizer(tBufferState->fboState(),tBufferScaleW,tBufferScaleH)));

  return ref_ptr<TransparencyState>::manage(tBufferState);
}

ref_ptr<State> resolveTransparency(
    OGLEApplication *app,
    const ref_ptr<TransparencyState> &transparency,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<AccumulateTransparency> state =
      ref_ptr<AccumulateTransparency>::manage(new AccumulateTransparency(transparency->mode()));
  state->setColorTexture(transparency->colorTexture());
  state->setCounterTexture(transparency->counterTexture());

  ref_ptr<StateNode> node = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(state)));
  root->addChild(node);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  state->createShader(shaderConfigurer.cfg());

  return ref_ptr<State>::cast(state);
}

/////////////////////////////////////
//// Post Passes
/////////////////////////////////////

// Creates root node for states rendering the background of the scene
ref_ptr<StateNode> createPostPassNode(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBufferOntop(tex, baseAttachment);

  ref_ptr<StateNode> root = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));

  // no depth writing
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthWrite(GL_FALSE);
  depthState->set_useDepthTest(GL_FALSE);
  fboState->joinStates(ref_ptr<State>::cast(depthState));

  return root;
}

ref_ptr<BlurState> createBlurState(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root)
{
  const GLfloat bufferScale = 0.5f;
  const Vec2ui blurTextureSize(
      input->width()*bufferScale,
      input->height()*bufferScale );
  ref_ptr<BlurState> blur =
      ref_ptr<BlurState>::manage(new BlurState(input, blurTextureSize));
  blur->set_sigma(4.0f);
  blur->set_numPixels(12.0f);

  app->addShaderInput(blur->sigma(), 0.0f, 25.0f, 3);
  app->addShaderInput(blur->numPixels(), 0.0f, 50.0f, 0);

  ref_ptr<StateNode> blurNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(blur)));
  root->addChild(blurNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(blurNode.get());
  blur->createShader(shaderConfigurer.cfg());

  // resize with window
  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventCallable>::manage(
      new FramebufferResizer(blur->framebuffer(),bufferScale,bufferScale)));

  return blur;
}

ref_ptr<DepthOfField> createDoFState(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<Texture> &depthInput,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<DepthOfField> dof =
      ref_ptr<DepthOfField>::manage(new DepthOfField(input,blurInput,depthInput));

  app->addShaderInput(dof->blurRange(), 0.0f, 100.0f, 2);
  app->addShaderInput(dof->focalDistance(), 0.0f, 100.0f, 2);
  app->addShaderInput(dof->focalWidth(), 0.0f, 100.0f, 2);

  ref_ptr<StateNode> node = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(dof)));
  root->addChild(node);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  dof->createShader(shaderConfigurer.cfg());

  return dof;
}

ref_ptr<Tonemap> createTonemapState(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<Tonemap> tonemap =
      ref_ptr<Tonemap>::manage(new Tonemap(input, blurInput));

  app->addShaderInput(tonemap->blurAmount(), 0.0f, 1.0f, 3);
  app->addShaderInput(tonemap->effectAmount(), 0.0f, 1.0f, 3);
  app->addShaderInput(tonemap->exposure(), 0.0f, 50.0f, 3);
  app->addShaderInput(tonemap->gamma(), 0.0f, 10.0f, 2);
  app->addShaderInput(tonemap->radialBlurSamples(), 0.0f, 100.0f, 0);
  app->addShaderInput(tonemap->radialBlurStartScale(), 0.0f, 1.0f, 3);
  app->addShaderInput(tonemap->radialBlurScaleMul(), 0.0f, 1.0f, 4);
  app->addShaderInput(tonemap->vignetteInner(), 0.0f, 10.0f, 2);
  app->addShaderInput(tonemap->vignetteOuter(), 0.0f, 10.0f, 2);

  ref_ptr<StateNode> node = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(tonemap)));
  root->addChild(node);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  tonemap->createShader(shaderConfigurer.cfg());

  return tonemap;
}

ref_ptr<AntiAliasing> createAAState(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &input,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<AntiAliasing> aa = ref_ptr<AntiAliasing>::manage(new AntiAliasing(input));

  app->addShaderInput(aa->spanMax(), 0.0f, 100.0f, 2);
  app->addShaderInput(aa->reduceMul(), 0.0f, 100.0f, 2);
  app->addShaderInput(aa->reduceMin(), 0.0f, 100.0f, 2);
  app->addShaderInput(aa->luma(), 0.0f, 100.0f, 2);

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
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBufferOntop(tex, baseAttachment);

  ref_ptr<StateNode> root = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));

  // no depth writing
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthWrite(GL_FALSE);
  fboState->joinStates(ref_ptr<State>::cast(depthState));

  return root;
}

// Creates sky box mesh
ref_ptr<DynamicSky> createSky(OGLEApplication *app, const ref_ptr<StateNode> &root)
{
  ref_ptr<DynamicSky> sky = ref_ptr<DynamicSky>::manage(new DynamicSky);
  sky->setSunElevation(0.8, 30.0, -20.0);
  //sky->set_updateInterval(1000.0);
  //sky->set_timeScale(0.0001);
  sky->set_dayTime(0.5); // middle of the day
  sky->setEarth();

  ref_ptr<TextureCube> milkyway = TextureLoader::loadCube(
      "res/textures/cube-milkyway.png", GL_FALSE, GL_FALSE, GL_RGB);
  milkyway->set_wrapping(GL_CLAMP_TO_EDGE);
  sky->setStarMap(ref_ptr<Texture>::cast(milkyway));
  sky->setStarMapBrightness(1.0f);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  sky->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(sky)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "sky.skyBox");

  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(sky));

  return sky;
}

ref_ptr<SkyBox> createSkyCube(
    OGLEFltkApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<SkyBox> mesh = ref_ptr<SkyBox>::manage(new SkyBox);
  mesh->setCubeMap(reflectionMap);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  mesh->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "sky.skyBox");

  return mesh;
}

ref_ptr<SnowParticles> createSnow(
    OGLEFltkApplication *app,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root,
    GLuint numSnowFlakes = 5000)
{
  ref_ptr<SnowParticles> particles =
      ref_ptr<SnowParticles>::manage(new SnowParticles(numSnowFlakes));
  ref_ptr<Texture> tex = TextureLoader::load("res/textures/flare.jpg");
  particles->set_particleTexture(tex);
  particles->set_depthTexture(depthTexture);
  particles->createBuffer();

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(particles)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  particles->createShader(shaderConfigurer.cfg());

  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(particles));

  app->addShaderInput(particles->gravity(), -100.0f, 100.0f, 1);
  app->addShaderInput(particles->dampingFactor(), 0.0f, 10.0f, 3);
  app->addShaderInput(particles->noiseFactor(), 0.0f, 10.0f, 3);
  app->addShaderInput(particles->cloudPosition(), -10.0f, 10.0f, 2);
  app->addShaderInput(particles->cloudRadius(), 0.0f, 100.0f, 2);
  app->addShaderInput(particles->particleSize(), 0.0f, 1.0f, 5);
  app->addShaderInput(particles->particleMass(), 0.0f, 10.0f, 3);
  app->addShaderInput(particles->brightness(), 0.0f, 1.0f, 3);
  app->addShaderInput(particles->softScale(), 0.0f, 100.0f, 2);

  return particles;
}

/////////////////////////////////////
//// Shading States
/////////////////////////////////////

// Creates deferred shading state and add to render tree
ref_ptr<DeferredShading> createShadingPass(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &gBuffer,
    const ref_ptr<StateNode> &root,
    ShadowMap::FilterMode shadowFiltering=ShadowMap::SINGLE)
{
  ref_ptr<DeferredShading> shading =
      ref_ptr<DeferredShading>::manage(new DeferredShading);

  shading->setDirFiltering(shadowFiltering);
  shading->setPointFiltering(shadowFiltering);
  shading->setSpotFiltering(shadowFiltering);

  ref_ptr<Texture> gDiffuseTexture = gBuffer->colorBuffer()[0];
  ref_ptr<Texture> gSpecularTexture = gBuffer->colorBuffer()[1];
  ref_ptr<Texture> gNorWorldTexture = gBuffer->colorBuffer()[2];
  ref_ptr<Texture> gDepthTexture = gBuffer->depthTexture();
  shading->set_gBuffer(
      gDepthTexture, gNorWorldTexture,
      gDiffuseTexture, gSpecularTexture);

  ref_ptr<FBOState> fboState =
      ref_ptr<FBOState>::manage(new FBOState(gBuffer));
  fboState->addDrawBufferUpdate(gDiffuseTexture, GL_COLOR_ATTACHMENT0);
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

ref_ptr<PointLight> createPointLight(OGLEFltkApplication *app,
    const Vec3f &pos=Vec3f(-4.0f, 1.0f, 0.0f),
    const Vec3f &diffuse=Vec3f(0.1f, 0.2f, 0.95f),
    const Vec2f &radius=Vec2f(7.5,8.0))
{
  ref_ptr<PointLight> pointLight =
      ref_ptr<PointLight>::manage(new PointLight);
  pointLight->set_position(pos);
  pointLight->set_diffuse(diffuse);
  pointLight->set_innerRadius(radius.x);
  pointLight->set_outerRadius(radius.y);

  app->addShaderInput(pointLight->position(), -100.0f, 100.0f, 2);
  app->addShaderInput(pointLight->diffuse(), 0.0f, 1.0f, 2);
  app->addShaderInput(pointLight->specular(), 0.0f, 1.0f, 2);
  app->addShaderInput(pointLight->radius(), 0.0f, 100.0f, 1);

  return pointLight;
}

ref_ptr<SpotLight> createSpotLight(OGLEFltkApplication *app,
    const Vec3f &pos=Vec3f(0.0f,6.5f,0.0f),
    const Vec3f &dir=Vec3f(0.0001f,-1.0f,0.0001f),
    const Vec3f &diffuse=Vec3f(0.95f,0.0f,0.0f),
    const Vec2f &radius=Vec2f(8.5f,10.5f),
    const Vec2f &coneAngles=Vec2f(34.0f,35.0f))
{
  ref_ptr<SpotLight> l = ref_ptr<SpotLight>::manage(new SpotLight);
  l->set_position(pos);
  l->set_spotDirection(dir);
  l->set_diffuse(diffuse);
  l->set_innerConeAngle(coneAngles.x);
  l->set_outerConeAngle(coneAngles.y);
  l->set_innerRadius(radius.x);
  l->set_outerRadius(radius.y);

  app->addShaderInput(l->position(), -100.0f, 100.0f, 2);
  app->addShaderInput(l->spotDirection(), -1.0f, 1.0f, 2);
  app->addShaderInput(l->coneAngle(), 0.0f, 1.0f, 5);
  app->addShaderInput(l->diffuse(), 0.0f, 1.0f, 2);
  app->addShaderInput(l->specular(), 0.0f, 1.0f, 2);
  app->addShaderInput(l->radius(), 0.0f, 100.0f, 1);

  return l;
}

ref_ptr<DirectionalShadowMap> createSunShadow(
    const ref_ptr<DynamicSky> &sky,
    const ref_ptr<PerspectiveCamera> &cam,
    const ref_ptr<Frustum> &frustum,
    const GLuint shadowMapSize=1024,
    const GLenum internalFormat=GL_DEPTH_COMPONENT16,
    const GLenum pixelType=GL_BYTE,
    const GLfloat shadowSplitWeight=0.5)
{
  DirectionalShadowMap *sm = new DirectionalShadowMap(
      sky->sun(),
      frustum, cam,
      shadowMapSize,
      shadowSplitWeight,
      internalFormat,
      pixelType);
  ref_ptr<DirectionalShadowMap> smRef = ref_ptr<DirectionalShadowMap>::manage(sm);

  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(smRef));

  return smRef;
}

ref_ptr<PointShadowMap> createPointShadow(
    OGLEApplication *app,
    const ref_ptr<PointLight> &l,
    const ref_ptr<PerspectiveCamera> &cam,
    const GLuint shadowMapSize=512,
    const GLenum internalFormat=GL_DEPTH_COMPONENT16,
    const GLenum pixelType=GL_BYTE)
{
  PointShadowMap *sm = new PointShadowMap(
      l, cam,
      shadowMapSize,
      internalFormat,
      pixelType);
  ref_ptr<PointShadowMap> smRef = ref_ptr<PointShadowMap>::manage(sm);
  sm->set_isFaceVisible(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_FALSE);

  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(smRef));

  return smRef;
}

ref_ptr<SpotShadowMap> createSpotShadow(
    OGLEApplication *app,
    const ref_ptr<SpotLight> &l,
    const ref_ptr<PerspectiveCamera> &cam,
    const GLuint shadowMapSize=512,
    const GLenum internalFormat=GL_DEPTH_COMPONENT16,
    const GLenum pixelType=GL_BYTE)
{
  SpotShadowMap *sm = new SpotShadowMap(
      l, cam,
      shadowMapSize,
      internalFormat,
      pixelType);
  ref_ptr<SpotShadowMap> smRef = ref_ptr<SpotShadowMap>::manage(sm);

  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(smRef));

  return smRef;
}

ref_ptr<SkyLightShaft> createSkyLightShaft(
    OGLEFltkApplication *app,
    const ref_ptr<DirectionalLight> &sun,
    const ref_ptr<Texture> &colorTexture,
    const ref_ptr<Texture> &depthTexture,
    const ref_ptr<StateNode> &root)
{
  ref_ptr<SkyLightShaft> ray = ref_ptr<SkyLightShaft>::manage(
      new SkyLightShaft(sun,colorTexture,depthTexture));

  ref_ptr<StateNode> rayNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(ray)));
  root->addChild(rayNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(rayNode.get());
  ray->createShader(shaderConfigurer.cfg());

  app->addShaderInput(ray->scatteringDensity(), 0.0f, 1.0f, 2);
  app->addShaderInput(ray->scatteringSamples(), 0.0f, 100.0f, 0);
  app->addShaderInput(ray->scatteringExposure(), 0.0f, 1.0f, 2);
  app->addShaderInput(ray->scatteringDecay(), 0.0f, 1.0f, 2);
  app->addShaderInput(ray->scatteringWeight(), 0.0f, 1.0f, 2);

  return ray;
}

/////////////////////////////////////
//// Mesh Factory
/////////////////////////////////////

class AnimationRangeUpdater : public EventCallable
{
public:
  AnimationRangeUpdater(const BoneAnimRange *animRanges, GLuint numAnimationRanges)
  : EventCallable(), animRanges_(animRanges), numAnimationRanges_(numAnimationRanges) {}

  void call(EventObject *ev, void *data)
  {
    NodeAnimation *anim = (NodeAnimation*)ev;
    Vec2d newRange = animRanges_[rand()%numAnimationRanges_].range;
    anim->setAnimationIndexActive(0, newRange + Vec2d(-1.0, -1.0) );
  }

protected:
  const BoneAnimRange *animRanges_;
  GLuint numAnimationRanges_;
};

// Loads Meshes from File using Assimp. Optionally Bone animations are loaded.
void createAssimpMesh(
    OGLEApplication *app,
    const ref_ptr<StateNode> &root,
    const string &modelFile,
    const string &texturePath,
    const Mat4f &meshRotation,
    const Vec3f &meshTranslation,
    const BoneAnimRange *animRanges=NULL,
    GLuint numAnimationRanges=0,
    GLdouble ticksPerSecond=20.0)
{
  AssimpImporter importer(modelFile, texturePath);
  list< ref_ptr<MeshState> > meshes = importer.loadMeshes();
  ref_ptr<NodeAnimation> boneAnim;

  if(animRanges && numAnimationRanges>0) {
    boneAnim = importer.loadNodeAnimation(
        GL_TRUE, ANIM_BEHAVIOR_LINEAR, ANIM_BEHAVIOR_LINEAR, ticksPerSecond);
  }

  for(list< ref_ptr<MeshState> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<MeshState> &mesh = *it;

    ref_ptr<Material> material = importer.getMeshMaterial(mesh.get());
    material->setConstantUniforms(GL_TRUE);
    mesh->joinStates(ref_ptr<State>::cast(material));

    ref_ptr<ModelTransformation> modelMat =
        ref_ptr<ModelTransformation>::manage(new ModelTransformation);
    modelMat->set_modelMat(meshRotation, 0.0f);
    modelMat->translate(meshTranslation, 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);
    mesh->joinStates(ref_ptr<State>::cast(modelMat));

    if(boneAnim.get()) {
      list< ref_ptr<AnimationNode> > meshBones =
          importer.loadMeshBones(mesh.get(), boneAnim.get());
      ref_ptr<BonesState> bonesState = ref_ptr<BonesState>::manage(new BonesState(
          meshBones, importer.numBoneWeights(mesh.get())));
      mesh->joinStates(ref_ptr<State>::cast(bonesState));
      AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(bonesState));
    }

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh->joinStates(ref_ptr<State>::cast(shaderState));

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(mesh)));
    root->addChild(meshNode);

    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), "mesh");
  }

  if(boneAnim.get()) {
    ref_ptr<EventCallable> animStopped = ref_ptr<EventCallable>::manage(
        new AnimationRangeUpdater(animRanges,numAnimationRanges));
    boneAnim->connect(NodeAnimation::ANIMATION_STOPPED, animStopped);
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(boneAnim));
    animStopped->call(boneAnim.get(), NULL);
  }
}

void createConeMesh(OGLEApplication *app, const ref_ptr<StateNode> &root)
{
  ClosedCone::Config cfg;
  cfg.levelOfDetail = 3;
  cfg.isBaseRequired = GL_TRUE;
  cfg.isNormalRequired = GL_TRUE;
  cfg.height = 3.0;
  cfg.radius = 1.0;
  ref_ptr<MeshState> mesh = ref_ptr<MeshState>::manage(new ClosedCone(cfg));

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
void createFloorMesh(OGLEApplication *app,
    const ref_ptr<StateNode> &root,
    const GLfloat &height=-2.0,
    const Vec3f &posScale=Vec3f(20.0f),
    const Vec2f &texcoScale=Vec2f(5.0f))
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
  ref_ptr<MeshState> floor = ref_ptr<MeshState>::manage(new Rectangle(meshCfg));

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

  ref_ptr<Texture> colMap_ = TextureLoader::load("res/textures/brick/color.jpg");
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
  texState->setMapTo(MAP_TO_COLOR);
  texState->set_blendMode(BLEND_MODE_SRC);
  material->addTexture(texState);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  floor->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(floor)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "mesh");
}

ref_ptr<MeshState> createBox(OGLEApplication *app, const ref_ptr<StateNode> &root)
{
    Box::Config cubeConfig;
    cubeConfig.texcoMode = Box::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 0.5f, 0.5f);
    ref_ptr<MeshState> mesh = ref_ptr<MeshState>::manage(new Box(cubeConfig));

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

    return mesh;
}

ref_ptr<MeshState> createSphere(OGLEApplication *app, const ref_ptr<StateNode> &root)
{
    Sphere::Config sphereConfig;
    sphereConfig.texcoMode = Sphere::TEXCO_MODE_NONE;
    ref_ptr<MeshState> mesh = ref_ptr<MeshState>::manage(new Sphere(sphereConfig));

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

ref_ptr<MeshState> createQuad(OGLEApplication *app, const ref_ptr<StateNode> &root)
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
  ref_ptr<MeshState> mesh =
      ref_ptr<MeshState>::manage(new Rectangle(quadConfig));

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

ref_ptr<MeshState> createReflectionSphere(
    OGLEFltkApplication *app,
    const ref_ptr<TextureCube> &reflectionMap,
    const ref_ptr<StateNode> &root)
{
  GLfloat radi[] = { 1.0 };
  Vec3f pos[] = { Vec3f(0.0f) };
  ref_ptr<MeshState> mesh = ref_ptr<MeshState>::manage(
      new SpriteSphere(radi,pos,sizeof(radi)/sizeof(GLfloat)));

  ref_ptr<ModelTransformation> modelMat =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelMat->translate(Vec3f(0.0f), 0.0f);
  mesh->joinStates(ref_ptr<State>::cast(modelMat));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  material->set_shading( Material::NO_SHADING );

  ref_ptr<TextureState> refractionTexture = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(reflectionMap)));
  refractionTexture->setMapTo(MAP_TO_COLOR);
  refractionTexture->set_blendMode(BLEND_MODE_SRC);
  refractionTexture->set_mapping(MAPPING_REFRACTION);
  material->addTexture(refractionTexture);

  ref_ptr<TextureState> reflectionTexture = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(reflectionMap)));
  reflectionTexture->setMapTo(MAP_TO_COLOR);
  reflectionTexture->set_blendMode(BLEND_MODE_MIX);
  reflectionTexture->set_blendFactor(0.35f);
  reflectionTexture->set_mapping(MAPPING_REFLECTION);
  material->addTexture(reflectionTexture);

  mesh->joinStates(ref_ptr<State>::cast(material));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  mesh->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "sprite_sphere");

  return mesh;
}

/////////////////////////////////////
//// GUI Factory
/////////////////////////////////////

class UpdateFPS : public Animation
{
public:
  UpdateFPS(const ref_ptr<TextureMappedText> &widget)
  : Animation(), widget_(widget), frameCounter_(0), sumDtMiliseconds_(0.0f) { }

  virtual GLboolean useGLAnimation() const  { return GL_TRUE; }
  virtual GLboolean useAnimation() const { return GL_FALSE; }

  virtual void animate(GLdouble dt) {}

  virtual void glAnimate(GLdouble dt) {
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
void createFPSWidget(OGLEApplication *app, const ref_ptr<StateNode> &root)
{
  FreeTypeFont& font = FontManager::get().getFont("res/fonts/arial.ttf", 12, 96);
  font.texture()->bind();
  font.texture()->set_filter(GL_LINEAR,GL_LINEAR);

  ref_ptr<TextureMappedText> widget =
      ref_ptr<TextureMappedText>::manage(new TextureMappedText(font, 12.0));
  widget->set_fgColor(Vec4f(1.0f));
  widget->set_bgColor(Vec4f(0.0f, 0.0f, 0.0f, 0.5f));
  widget->set_value(L"0 FPS");

  ref_ptr<ModelTransformation> modelTransformation =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelTransformation->translate( Vec3f( 4.0, 4.0, 0.0 ), 0.0f );
  widget->joinStates(ref_ptr<State>::cast(modelTransformation));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  widget->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> widgetNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(widget)));
  root->addChild(widgetNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(widgetNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "texture_mapped_text");

  AnimationManager::get().addAnimation(ref_ptr<Animation>::manage(new UpdateFPS(widget)));
}

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    const ref_ptr<Texture> &tex,
    GLenum baseAttachment)
{
  ref_ptr<OrthoCamera> cam = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  ref_ptr<StateNode> guiRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));

  // update gui camera projection when window size changes
  ref_ptr<GUIProjectionUpdater> projUpdater = ref_ptr<GUIProjectionUpdater>::manage(
      new GUIProjectionUpdater(cam));
  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventCallable>::cast(projUpdater));
  projUpdater->call(app, NULL);
  // enable fbo and call DrawBuffer()
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBufferOntop(tex,baseAttachment);
  cam->joinStates(ref_ptr<State>::cast(fboState));
  // alpha blend GUI widgets with scene
  cam->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  // no depth testing for gui
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  cam->joinStates(ref_ptr<State>::cast(depthState));

  return guiRoot;
}

// Creates root node for states rendering the HUD
ref_ptr<StateNode> createHUD(OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum baseAttachment)
{
  // TODO: ortho cam needed ?
  ref_ptr<OrthoCamera> cam = ref_ptr<OrthoCamera>::manage(new OrthoCamera);
  ref_ptr<StateNode> guiRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));

  // update gui camera projection when window size changes
  ref_ptr<GUIProjectionUpdater> projUpdater = ref_ptr<GUIProjectionUpdater>::manage(
      new GUIProjectionUpdater(cam));
  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventCallable>::cast(projUpdater));
  projUpdater->call(app, NULL);
  // enable fbo and call DrawBuffer()
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBuffer(baseAttachment);
  cam->joinStates(ref_ptr<State>::cast(fboState));
  // alpha blend GUI widgets with scene
  cam->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  // no depth testing for gui
  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);
  cam->joinStates(ref_ptr<State>::cast(depthState));

  return guiRoot;
}

#endif /* FACTORY_H_ */
