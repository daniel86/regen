
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/textures/cube-image-texture.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/render-tree/blur-node.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

class DOFNode : public StateNode
{
public:
  DOFNode(
      ref_ptr<Texture> &input,
      ref_ptr<Texture> &depthTexture,
      ref_ptr<Texture> &blurTexture,
      ref_ptr<MeshState> &orthoQuad)
  : StateNode(),
    input_(input),
    blurTexture_(blurTexture),
    depthTexture_(depthTexture)
  {
    focalDistance_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("focalDistance"));
    focalDistance_->setUniformData(10.0f);
    focalDistance_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(focalDistance_));

    focalWidth_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("focalWidth"));
    focalWidth_->setUniformData(2.5f);
    focalWidth_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(focalWidth_));

    blurRange_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurRange"));
    blurRange_->setUniformData(5.0f);
    blurRange_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(blurRange_));

    shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
    shader_->joinStates(ref_ptr<State>::cast(orthoQuad));
    state_->joinStates( ref_ptr<State>::cast(shader_) );
  }

  void set_focalDistance(GLfloat focalDistance) {
    focalDistance_->setVertex1f(0,focalDistance);
  }
  ref_ptr<ShaderInput1f>& focalDistance() {
    return focalDistance_;
  }
  void set_focalWidth(GLfloat focalWidth) {
    focalWidth_->setVertex1f(0,focalWidth);
  }
  ref_ptr<ShaderInput1f>& focalWidth() {
    return focalWidth_;
  }
  void set_blurRange(GLfloat blurRange) {
    blurRange_->setVertex1f(0,blurRange);
  }
  ref_ptr<ShaderInput1f>& blurRange() {
    return blurRange_;
  }

  virtual void set_parent(StateNode *parent)
  {
    StateNode::set_parent(parent);

    ShaderConfig shaderCfg;
    configureShader(&shaderCfg);
    shader_->createShader(shaderCfg, "depth-of-field");
  }
  ref_ptr<ShaderState> shader_;
  ref_ptr<Texture> input_;
  ref_ptr<Texture> blurTexture_;
  ref_ptr<Texture> depthTexture_;
  ref_ptr<ShaderInput1f> focalDistance_;
  ref_ptr<ShaderInput1f> focalWidth_;
  ref_ptr<ShaderInput1f> blurRange_;
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("HDR test");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.0f)
  );

  renderTree->setLight();
  camManipulator->set_radius(2.0f, 0.0);
  camManipulator->setStepLength(0.005f, 0.0);

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    sphereConfig.levelOfDetail = 5;
    ref_ptr<MeshState> meshState = ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_chrome();

    renderTree->addMesh(meshState, modelMat, material);
  }

  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);

  ref_ptr<StateNode> parentNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(depthState)));
  renderTree->globalStates()->addChild(parentNode);
  // disable depth test/write
  depthState->joinStates(ref_ptr<State>::cast(renderTree->perspectiveCamera()));
  // bind scene texture to a texture channel
  ref_ptr<TextureState> sceneTexture = ref_ptr<TextureState>::manage(
      new TextureState(renderTree->sceneTexture()));
  sceneTexture->set_name("sceneTexture");
  depthState->joinStates(ref_ptr<State>::cast(sceneTexture));
  // bind scene depth texture to a texture channel
  ref_ptr<TextureState> depthTexture = ref_ptr<TextureState>::manage(new TextureState(
      ref_ptr<Texture>::cast(renderTree->sceneDepthTexture())));
  depthTexture->set_name("depthTexture");
  depthState->joinStates(ref_ptr<State>::cast(depthTexture));

  /////////////
  /////////////

  ref_ptr<BlurNode> blurNode = ref_ptr<BlurNode>::manage(new BlurNode(
      sceneTexture->texture(), renderTree->orthoQuad(), 0.5f));
  application->addShaderInput(blurNode->sigma(), 0.0f, 100.0f, 2);
  application->addShaderInput(blurNode->numPixels(), 0.0f, 100.0f, 0);
  ref_ptr<Texture> &blurTexture = blurNode->blurredTexture();
  parentNode->addChild(ref_ptr<StateNode>::cast(blurNode));

  /////////////
  /////////////

  ref_ptr<FBOState> dofFBO = ref_ptr<FBOState>::manage(new FBOState(fboState->fbo()));
  dofFBO->addDrawBuffer(GL_COLOR_ATTACHMENT1);
  ref_ptr<StateNode> dofParent = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(dofFBO)));

  ref_ptr<TextureState> blurTexState = ref_ptr<TextureState>::manage(
      new TextureState(blurTexture));
  blurTexState->set_name("blurTexture");
  dofParent->state()->joinStates(ref_ptr<State>::cast(blurTexState));
  ref_ptr<TextureState> inputTexState = ref_ptr<TextureState>::manage(
      new TextureStateNoChannel(sceneTexture));
  inputTexState->set_name("inputTexture");
  dofParent->state()->joinStates(ref_ptr<State>::cast(inputTexState));

  ref_ptr<DOFNode> dofNode = ref_ptr<DOFNode>::manage(
      new DOFNode(sceneTexture->texture(), depthTexture->texture(), blurTexture, renderTree->orthoQuad()));
  application->addShaderInput(dofNode->blurRange(), 0.0f, 100.0f, 2);
  application->addShaderInput(dofNode->focalDistance(), 0.0f, 100.0f, 2);
  application->addShaderInput(dofNode->focalWidth(), 0.0f, 100.0f, 2);
  parentNode->addChild(ref_ptr<StateNode>::cast(dofParent));
  dofParent->addChild(ref_ptr<StateNode>::cast(dofNode));

  renderTree->addSkyBox("res/textures/cube-stormydays.jpg");
  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  return application->mainLoop();
}
