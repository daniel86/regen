
#include <regen/utility/filesystem.h>
#include "factory.h"
using namespace regen;

#define USE_HUD

class VolumeLoader : public EventHandler
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
  : EventHandler(), app_(app)
  {
    rotateEnabled_ = GL_TRUE;
    rotation_ = Mat4f::identity();

    Box::Config meshCfg;
    meshCfg.texcoMode = Box::TEXCO_MODE_NONE;
    meshCfg.isNormalRequired = GL_FALSE;
    meshCfg.posScale = Vec3f(1.0f);
    mesh = ref_ptr<Mesh>::manage(new Box(meshCfg));

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

    vao_ = ref_ptr<VAOState>::manage(new VAOState(shaderState_));
    mesh->joinStates(ref_ptr<State>::cast(vao_));

    node_ = ref_ptr<StateNode>::manage(new StateNode(ref_ptr<State>::cast(mesh)));
    root->addChild(node_);

    app->addShaderInput("VolumeRenderer",
        ref_ptr<ShaderInput>::cast(u_rayStep),
        Vec4f(0.001f), Vec4f(0.1f), Vec4i(4),
        "Step size along the ray that intersects the volume.");
    app->addShaderInput("VolumeRenderer",
        ref_ptr<ShaderInput>::cast(u_densityThreshold),
        Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
        "Density samples below threshold are ignored.");
    app->addShaderInput("VolumeRenderer",
        ref_ptr<ShaderInput>::cast(u_densityScale),
        Vec4f(0.0f), Vec4f(10.0f), Vec4i(2),
        "Each density sample is scaled with this factor.");

    setMode(VOLUME_MODE_FIRST_MAX_INTENSITY);
    setVolumeFile(0);
    createShader();

    rotation_ = Mat4f::rotationMatrix(0.0, 0.25*M_PI, 0.0);
    modelMat_->set_modelMat(rotation_, 0.0);
  }

  void createShader()
  {
    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(node_.get());
    shaderState_->createShader(shaderConfigurer.cfg(), "volume");
    vao_->updateVAO(RenderState::get(), mesh.get());
  }

  void setMode(VolumeMode mode)
  {
    mode_ = mode;
    shaderState_->shaderDefine("USE_MAX_INTENSITY", "FALSE");
    shaderState_->shaderDefine("USE_FIRST_MAXIMUM", "FALSE");
    shaderState_->shaderDefine("USE_AVERAGE_INTENSITY", "FALSE");
    switch(mode) {
    case VOLUME_MODE_ALPHA_BLEND:
      app_->toplevelWidget()->setWindowTitle("Raycasting: alpha blending");
      shaderState_->shaderDefine("USE_MAX_INTENSITY", "TRUE");
      break;
    case VOLUME_MODE_MAX_INTENSITY:
      app_->toplevelWidget()->setWindowTitle("Raycasting: max intensity");
      shaderState_->shaderDefine("USE_FIRST_MAXIMUM", "TRUE");
      break;
    case VOLUME_MODE_FIRST_MAX_INTENSITY:
      app_->toplevelWidget()->setWindowTitle("Raycasting: first max intensity");
      shaderState_->shaderDefine("USE_AVERAGE_INTENSITY", "TRUE");
      break;
    case VOLUME_MODE_AVG_INTENSITY:
      app_->toplevelWidget()->setWindowTitle("Raycasting: average intensity");
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
      tex = TextureLoader::loadRAW(filesystemPath(
          REGEN_SOURCE_DIR, "applications/res/textures/volumes/bonsai.raw"), Vec3ui(256u), 1, 8);
      transferTex = TextureLoader::load(filesystemPath(
          REGEN_SOURCE_DIR, "applications/res/textures/volumes/bonsai-transfer.png"));
      shaderState_->shaderDefine("SWITCH_VOLUME_Y", "FALSE");
    }
    tex->startConfig();
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->set_wrapping(GL_CLAMP_TO_EDGE);
    tex->stopConfig();

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

  void call(EventObject *ev, EventData *data)
  {
    Application::KeyEvent *keyEv = (Application::KeyEvent*)data;
    if(!keyEv->isUp) { return; }

    if(keyEv->key == ' ') {
      nextMode();
      createShader();
    }
    else if(keyEv->key == 'r') {
      rotateEnabled_ = !rotateEnabled_;
    }
  }

protected:
  QtApplication *app_;

  ref_ptr<StateNode> node_;
  ref_ptr<TextureState> volumeTexState_;
  ref_ptr<TextureState> transferTexState_;
  ref_ptr<ShaderState> shaderState_;
  ref_ptr<VAOState> vao_;
  ref_ptr<ModelTransformation> modelMat_;
  ref_ptr<Mesh> mesh;
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
  const TBuffer::Mode alphaMode = TBuffer::MODE_FRONT_TO_BACK;

  ref_ptr<QtApplication> app = initApplication(argc,argv);

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

  ref_ptr<Texture> gDepthTexture;

  ref_ptr<TBuffer> tTargetState = createTBuffer(app.get(), cam, gDepthTexture, alphaMode);
  ref_ptr<StateNode> tTargetNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(tTargetState)));
  sceneRoot->addChild(tTargetNode);

  ref_ptr<StateNode> tBufferNode = ref_ptr<StateNode>::manage(new StateNode);
  tTargetState->fboState()->fbo()->createDepthTexture(GL_TEXTURE_2D, GL_DEPTH_COMPONENT24, GL_UNSIGNED_BYTE);
  tTargetState->fboState()->setClearDepth();
  switch(alphaMode) {
  case TBuffer::MODE_BACK_TO_FRONT:
    tTargetState->joinStatesFront(ref_ptr<State>::manage(
        new SortByModelMatrix(tBufferNode, cam, GL_FALSE)));
    break;
  case TBuffer::MODE_FRONT_TO_BACK:
    tTargetState->joinStatesFront(ref_ptr<State>::manage(
        new SortByModelMatrix(tBufferNode, cam, GL_TRUE)));
    break;
  default:
    break;
  }
  tTargetNode->addChild(tBufferNode);
  ref_ptr<FrameBufferObject> fbo = tTargetState->fboState()->fbo();
  ref_ptr<VolumeLoader> volume = ref_ptr<VolumeLoader>::manage(
      new VolumeLoader(app.get(), tBufferNode));
  app->connect(Application::KEY_EVENT, ref_ptr<EventHandler>::cast(volume));

  ref_ptr<FBOState> postPassState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  ref_ptr<StateNode> postPassNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(postPassState)));
  sceneRoot->addChild(postPassNode);

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), fbo, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createLogoWidget(app.get(), guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), fbo, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
