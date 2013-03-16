
#include "factory.h"
using namespace ogle;

#define USE_HUD

class VolumeLoader : public EventHandler, public Animation
{
public:
  enum VolumeMode
  {
    VOLUME_MODE_ALPHA_BLEND = 0,
    VOLUME_MODE_MAX_INTENSITY,
    VOLUME_MODE_FIRST_MAX_INTENSITY,
    VOLUME_MODE_AVG_INTENSITY,
    VOLUME_MODE_LAST
  };

  VolumeLoader(QtApplication *app, const ref_ptr<StateNode> &root)
  : EventHandler(), Animation(), app_(app)
  {
    rotateEnabled_ = GL_TRUE;
    rotation_ = Mat4f::identity();

    Box::Config meshCfg;
    meshCfg.texcoMode = Box::TEXCO_MODE_NONE;
    meshCfg.isNormalRequired = GL_FALSE;
    meshCfg.posScale = Vec3f(1.0f);
    ref_ptr<Mesh> mesh = ref_ptr<Mesh>::manage(new Box(meshCfg));

    u_rayStep = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("rayStep"));
    u_rayStep->setUniformData(0.02);
    mesh->joinShaderInput(ref_ptr<ShaderInput>::cast(u_rayStep));

    u_densityThreshold = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("densityThreshold"));
    u_densityThreshold->setUniformData(0.125);
    mesh->joinShaderInput(ref_ptr<ShaderInput>::cast(u_densityThreshold));

    u_densityScale = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("densityScale"));
    u_densityScale->setUniformData(2.0);
    mesh->joinShaderInput(ref_ptr<ShaderInput>::cast(u_densityScale));

    modelMat_ = ref_ptr<ModelTransformation>::manage(new ModelTransformation);
    modelMat_->translate(Vec3f(0.0f,-0.75f,0.0f), 0.0f);
    mesh->joinStates(ref_ptr<State>::cast(modelMat_));

    shaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh->joinStates(ref_ptr<State>::cast(shaderState_));

    node_ = ref_ptr<StateNode>::manage(new StateNode(ref_ptr<State>::cast(mesh)));
    root->addChild(node_);

    app->addGenericData("VolumeRenderer",
        ref_ptr<ShaderInput>::cast(u_rayStep),
        Vec4f(0.001f), Vec4f(0.1f), Vec4i(2),
        "Step size along the ray that intersects the volume.");
    app->addGenericData("VolumeRenderer",
        ref_ptr<ShaderInput>::cast(u_densityThreshold),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Density samples below threshold are ignored.");
    app->addGenericData("VolumeRenderer",
        ref_ptr<ShaderInput>::cast(u_densityScale),
        Vec4f(0.0f), Vec4f(2.0f), Vec4i(2),
        "Each density sample is scaled with this factor.");

    setMode(VOLUME_MODE_MAX_INTENSITY);
    setVolumeFile(0);
    createShader();
  }

  void createShader()
  {
    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(node_.get());
    shaderState_->createShader(shaderConfigurer.cfg(), "volume");
  }

  void setMode(VolumeMode mode)
  {
    mode_ = mode;
    shaderState_->shaderDefine("USE_MAX_INTENSITY", "FALSE");
    shaderState_->shaderDefine("USE_FIRST_MAXIMUM", "FALSE");
    shaderState_->shaderDefine("USE_AVERAGE_INTENSITY", "FALSE");
    switch(mode) {
    case VOLUME_MODE_ALPHA_BLEND:
      app_->set_windowTitle("Raycasting: alpha blending");
      shaderState_->shaderDefine("USE_MAX_INTENSITY", "TRUE");
      break;
    case VOLUME_MODE_MAX_INTENSITY:
      app_->set_windowTitle("Raycasting: max intensity");
      shaderState_->shaderDefine("USE_FIRST_MAXIMUM", "TRUE");
      break;
    case VOLUME_MODE_FIRST_MAX_INTENSITY:
      app_->set_windowTitle("Raycasting: first max intensity");
      shaderState_->shaderDefine("USE_AVERAGE_INTENSITY", "TRUE");
      break;
    case VOLUME_MODE_AVG_INTENSITY:
      app_->set_windowTitle("Raycasting: average intensity");
      break;
    case VOLUME_MODE_LAST:
      break;
    }
  }
  void nextMode()
  {
    GLint mode = (GLint)mode_+1;
    if(mode == VOLUME_MODE_LAST) { mode = 0; }
    setMode((VolumeMode)mode);
  }

  void setVolumeFile(GLint index)
  {
    fileIndex_ = index;

    ref_ptr<Texture> transferTex, tex;
    if(index==0) {
      tex = TextureLoader::loadRAW("res/textures/bonsai.raw", Vec3ui(256u), 1, 8);
      transferTex = TextureLoader::load("res/textures/bonsai-transfer.png");
      shaderState_->shaderDefine("SWITCH_VOLUME_Y", "FALSE");
    }
    else if(index==1) {
      tex = TextureLoader::loadRAW("res/textures/stent8.raw", Vec3ui(512u,512u,174u), 1, 8);
      transferTex = TextureLoader::load("res/textures/stent-transfer.png");
      shaderState_->shaderDefine("SWITCH_VOLUME_Y", "TRUE");
    }
    else if(index==2) {
      tex = TextureLoader::loadRAW("res/textures/backpack8.raw", Vec3ui(512u,512u,373u), 1, 8);
      transferTex = TextureLoader::load("res/textures/backpack-transfer.png");
      shaderState_->shaderDefine("SWITCH_VOLUME_Y", "TRUE");
    }
    tex->bind();
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->set_wrapping(GL_CLAMP_TO_EDGE);
    tex->set_wrappingW(GL_CLAMP_TO_EDGE);


    if(volumeTexState_.get()) {
      modelMat_->disjoinStates(ref_ptr<State>::cast(volumeTexState_));
    }
    volumeTexState_ = ref_ptr<TextureState>::manage(new TextureState(tex));
    volumeTexState_->set_name("volumeTexture");
    modelMat_->joinStates(ref_ptr<State>::cast(volumeTexState_));

    if(transferTexState_.get()) {
      modelMat_->disjoinStates(ref_ptr<State>::cast(transferTexState_));
    }
    transferTexState_ = ref_ptr<TextureState>::manage(new TextureState(transferTex));
    transferTexState_->set_name("transferTexture");
    modelMat_->joinStates(ref_ptr<State>::cast(transferTexState_));
  }
  void nextVolumeFile()
  {
    setVolumeFile((fileIndex_+1)%3);
  }

  virtual void call(EventObject *ev, void *data)
  {
    OGLEApplication::KeyEvent *keyEv = (OGLEApplication::KeyEvent*)data;
    if(!keyEv->isUp) { return; }

    if(keyEv->key == ' ') {
      nextMode();
      createShader();
    }
    else if(keyEv->key == 'm') {
      nextVolumeFile();
      createShader();
    }
    else if(keyEv->key == 'r') {
      rotateEnabled_ = !rotateEnabled_;
    }
  }

  virtual void animate(GLdouble dt)
  {
    if(rotateEnabled_) {
      rotation_ = rotation_ * Mat4f::rotationMatrix(0.0, 0.002345*dt, 0.0);
    }
  }
  virtual void glAnimate(RenderState *rs, GLdouble dt)
  {
    modelMat_->set_modelMat(rotation_, dt);
  }
  virtual GLboolean useGLAnimation() const
  {
    return GL_TRUE;
  }
  virtual GLboolean useAnimation() const
  {
    return GL_TRUE;
  }

protected:
  QtApplication *app_;

  ref_ptr<StateNode> node_;
  ref_ptr<TextureState> volumeTexState_;
  ref_ptr<TextureState> transferTexState_;
  ref_ptr<ShaderState> shaderState_;
  ref_ptr<ModelTransformation> modelMat_;

  ref_ptr<ShaderInput1f> u_rayStep;
  ref_ptr<ShaderInput1f> u_densityThreshold;
  ref_ptr<ShaderInput1f> u_densityScale;

  GLboolean rotateEnabled_;
  Mat4f rotation_;
  VolumeMode mode_;
  GLint fileIndex_;
};

int main(int argc, char** argv)
{
  ref_ptr<QtApplication> app = initApplication(argc,argv,"Volume RayCasting");

  // create a root node for everything that needs camera as input
  ref_ptr<Camera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 0.0f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 4.0f );
  manipulator->set_degree( 0.0f );
  manipulator->setStepLength( M_PI*0.0 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  const TBuffer::Mode alphaMode = TBuffer::MODE_FRONT_TO_BACK;
  ref_ptr<Texture> gDepthTexture;
  ref_ptr<TBuffer> tBufferState = createTBuffer(app.get(), cam, gDepthTexture, alphaMode);
  ref_ptr<StateNode> tBufferNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(tBufferState)));
  tBufferState->fboState()->fbo()->createDepthTexture(GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, GL_UNSIGNED_BYTE);
  tBufferState->fboState()->setClearDepth();
  switch(alphaMode) {
  case TBuffer::MODE_BACK_TO_FRONT:
    tBufferState->joinStatesFront(ref_ptr<State>::manage(
        new SortByModelMatrix(tBufferNode, cam, GL_FALSE)));
    break;
  case TBuffer::MODE_FRONT_TO_BACK:
    tBufferState->joinStatesFront(ref_ptr<State>::manage(
        new SortByModelMatrix(tBufferNode, cam, GL_TRUE)));
    break;
  default:
    break;
  }
  sceneRoot->addChild(tBufferNode);
  ref_ptr<FrameBufferObject> fbo = tBufferState->fboState()->fbo();
  ref_ptr<VolumeLoader> volume = ref_ptr<VolumeLoader>::manage(
      new VolumeLoader(app.get(), tBufferNode));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(volume));
  app->connect(OGLEApplication::KEY_EVENT, ref_ptr<EventHandler>::cast(volume));

  ref_ptr<FBOState> postPassState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  ref_ptr<StateNode> postPassNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(postPassState)));
  sceneRoot->addChild(postPassNode);

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), fbo, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), fbo, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
