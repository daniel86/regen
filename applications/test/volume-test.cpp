
#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/av/video-texture.h>
#include <ogle/textures/texture-loader.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/cull-state.h>
#include <ogle/animations/animation-manager.h>

#include <applications/application-config.h>
#include <applications/fltk-ogle-application.h>

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

class RotateCube : public Animation
{
public:
  RotateCube(ref_ptr<ModelTransformationState> &modelMat)
  : Animation(),
    modelMat_(modelMat)
  {
    enabled = GL_TRUE;
    rotation_ = identity4f();
  }
  virtual void animate(GLdouble dt) {
    if(enabled) {
      rotation_ = rotation_ * xyzRotationMatrix(0.0, 0.002345*dt, 0.0);
    }
  }
  virtual void glAnimate(GLdouble dt) {
    modelMat_->set_modelMat(rotation_, dt);
  }
  virtual GLboolean useGLAnimation() const {
    return GL_TRUE;
  }
  virtual GLboolean useAnimation() const {
    return GL_TRUE;
  }
  ref_ptr<ModelTransformationState> modelMat_;
  Mat4f rotation_;
  GLboolean enabled;
};

enum VolumeMode
{
  VOLUME_MODE_ALPHA_BLEND = 0,
  VOLUME_MODE_MAX_INTENSITY,
  VOLUME_MODE_FIRST_MAX_INTENSITY,
  VOLUME_MODE_AVG_INTENSITY,
  VOLUME_MODE_LAST
};

OGLEFltkApplication *application;
TestRenderTree *renderTree;
ref_ptr<Texture> volumeMap_;
ref_ptr<Texture> transferMap_;
ref_ptr<RotateCube> rotateAnimation_;
ref_ptr<MeshState> mesh_;
ref_ptr<StateNode> meshNode_;
ref_ptr<Material> material;
VolumeMode currentMode_;

ref_ptr<ShaderInput1f> u_rayStep;
ref_ptr<ShaderInput1f> u_densityThreshold;
ref_ptr<ShaderInput1f> u_densityScale;

const int numVolumeFiles = 3;
int lastVolumeFile_ = 0;
GLboolean switchY = GL_FALSE;
void setVolumeFile(int index)
{
  lastVolumeFile_ = index;

  GLuint bytesPerComponent = 8;
  GLuint numComponents = 1;
  string filePath;
  Vec3ui size;
  if(index==0) {
    filePath = "res/textures/bonsai.raw";
    size = Vec3ui(256u);
    transferMap_ = TextureLoader::load("res/textures/bonsai-transfer.png");
    switchY = GL_FALSE;
  }
  else if(index==1) {
    filePath = "res/textures/stent8.raw";
    size = Vec3ui(512u,512u,174u);
    transferMap_ = TextureLoader::load("res/textures/stent-transfer.png");
    switchY = GL_TRUE;
  }
  else if(index==2) {
    filePath = "res/textures/backpack8.raw";
    size = Vec3ui(512u,512u,373u);
    transferMap_ = TextureLoader::load("res/textures/backpack-transfer.png");
    switchY = GL_TRUE;
  }

  volumeMap_ = TextureLoader::loadRAW(
      filePath,size,numComponents,bytesPerComponent);
  volumeMap_->bind();
  volumeMap_->set_filter(GL_LINEAR, GL_LINEAR);
  volumeMap_->set_wrapping(GL_CLAMP_TO_EDGE);
  volumeMap_->set_wrappingW(GL_CLAMP_TO_EDGE);
}

void setMode(VolumeMode mode)
{
  currentMode_ = mode;

  switch(mode) {
  case VOLUME_MODE_ALPHA_BLEND:
    application->set_windowTitle("Raycasting: alpha blending");
    break;
  case VOLUME_MODE_MAX_INTENSITY:
    application->set_windowTitle("Raycasting: max intensity");
    break;
  case VOLUME_MODE_FIRST_MAX_INTENSITY:
    application->set_windowTitle("Raycasting: first max intensity");
    break;
  case VOLUME_MODE_AVG_INTENSITY:
    application->set_windowTitle("Raycasting: average intensity");
    break;
  case VOLUME_MODE_LAST:
    break;
  }

  material = ref_ptr<Material>::manage(new Material);
  material->set_shading( Material::NO_SHADING );
  material->setConstantUniforms(GL_TRUE);

  Box::Config cubeConfig;
  cubeConfig.texcoMode = Box::TEXCO_MODE_NONE;
  cubeConfig.isNormalRequired = GL_FALSE;
  cubeConfig.posScale = Vec3f(1.0f);
  ref_ptr<MeshState> mesh =
      ref_ptr<MeshState>::manage(new Box(cubeConfig));

  ref_ptr<ModelTransformationState> modelMat;
  modelMat = ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
  modelMat->translate(Vec3f(0.0f,-0.75f,0.0f), 0.0f);
  if(rotateAnimation_.get()!=NULL) {
    rotateAnimation_->modelMat_ = modelMat;
  }
  else {
    rotateAnimation_ = ref_ptr<RotateCube>::manage(new RotateCube(modelMat));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(rotateAnimation_));
  }

  ref_ptr<TextureState> volumeMapState =
      ref_ptr<TextureState>::manage(new TextureState(volumeMap_));
  volumeMapState->set_name("volumeTexture");
  material->addTexture(volumeMapState);

  if(transferMap_.get()) {
    ref_ptr<TextureState> transferMapState =
        ref_ptr<TextureState>::manage(new TextureState(transferMap_));
    transferMapState->set_name("transferTexture");
    material->addTexture(transferMapState);
  }

  material->joinShaderInput(ref_ptr<ShaderInput>::cast(u_rayStep));
  material->joinShaderInput(ref_ptr<ShaderInput>::cast(u_densityThreshold));
  material->joinShaderInput(ref_ptr<ShaderInput>::cast(u_densityScale));

  switch(mode) {
  case VOLUME_MODE_MAX_INTENSITY:
    material->shaderDefine("USE_MAX_INTENSITY", "TRUE");
    break;
  case VOLUME_MODE_FIRST_MAX_INTENSITY:
    material->shaderDefine("USE_FIRST_MAXIMUM", "TRUE");
    break;
  case VOLUME_MODE_AVG_INTENSITY:
    material->shaderDefine("USE_AVERAGE_INTENSITY", "TRUE");
    break;
  case VOLUME_MODE_ALPHA_BLEND:
  case VOLUME_MODE_LAST:
    break;
  }
  if(switchY) {
    material->shaderDefine("SWITCH_VOLUME_Y", "TRUE");
  }

  if(meshNode_.get()) {
    renderTree->removeMesh(meshNode_);
  }
  meshNode_ = renderTree->addMesh(mesh, modelMat, material, "volume", GL_TRUE);
  // force culling
  meshNode_->state()->joinStates(ref_ptr<State>::manage(new CullEnableState));
}

class VolumeKeyEventHandler : public EventCallable
{
public:
  VolumeKeyEventHandler() : EventCallable() {}
  void call(EventObject *ev, void *data)
  {
    OGLEApplication::KeyEvent *keyEv = (OGLEApplication::KeyEvent*)data;
    if(!keyEv->isUp) { return; }

    if(keyEv->key == ' ') {
      int mode = (int)currentMode_+1;
      if(mode == VOLUME_MODE_LAST) { mode = 0; }
      setMode((VolumeMode)mode);
    }
    else if(keyEv->key == 'm') {
      setVolumeFile((lastVolumeFile_+1)%numVolumeFiles);
      setMode(currentMode_);
    }
    else if(keyEv->key == 'r') {
      rotateAnimation_->enabled = !rotateAnimation_->enabled;
    }
  }
};

int main(int argc, char** argv)
{
  renderTree = new TestRenderTree;
  application = new OGLEFltkApplication(renderTree, argc, argv);
  application->show();

  ref_ptr<PerspectiveCamera> &cam = renderTree->perspectiveCamera();
  cam->set_direction(Vec3f(0.0,0.0,1.0));
  cam->set_position(Vec3f(0.0,0.0,-5.0));
  cam->updatePerspective(0.0f);
  cam->update(0.0f);

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGB,
      GL_DEPTH_COMPONENT16,
      GL_TRUE,
      GL_TRUE,
      Vec4f(1.0f)
  );
  renderTree->setTransparencyMode(TRANSPARENCY_MODE_AVERAGE_SUM);

  u_rayStep = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("rayStep"));
  u_rayStep->setUniformData(0.02);
  application->addShaderInput(u_rayStep, 0.001, 0.1, 5);

  u_densityThreshold = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("densityThreshold"));
  u_densityThreshold->setUniformData(0.125);
  application->addShaderInput(u_densityThreshold, 0.0, 1.0, 5);

  u_densityScale = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("densityScale"));
  u_densityScale->setUniformData(2.0);
  application->addShaderInput(u_densityScale, 0.0, 2.0, 3);

  setVolumeFile(0);
  setMode(VOLUME_MODE_ALPHA_BLEND);
  ref_ptr<VolumeKeyEventHandler> keyHandler =
      ref_ptr<VolumeKeyEventHandler>::manage(new VolumeKeyEventHandler);
  application->connect(OGLEApplication::KEY_EVENT,
      ref_ptr<EventCallable>::cast(keyHandler));

  renderTree->setShowFPS();
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
  return application->mainLoop();
}
