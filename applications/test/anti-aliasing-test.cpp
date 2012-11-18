
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/states/shader-state.h>
#include <ogle/animations/animation-manager.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

class AANode : public StateNode
{
public:
  AANode(
      ref_ptr<Texture> &input,
      ref_ptr<MeshState> &orthoQuad)
  : StateNode(),
    input_(input)
  {
    spanMax_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("spanMax"));
    spanMax_->setUniformData(8.0f);
    spanMax_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(spanMax_));

    reduceMul_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("reduceMul"));
    reduceMul_->setUniformData(1.0f/8.0f);
    reduceMul_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(reduceMul_));

    reduceMin_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("reduceMin"));
    reduceMin_->setUniformData(1.0f/128.0f);
    reduceMin_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(reduceMin_));

    luma_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("luma"));
    luma_->setUniformData(Vec3f(0.299, 0.587, 0.114));
    luma_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(luma_));

    shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
    shader_->joinStates(ref_ptr<State>::cast(orthoQuad));
    state_->joinStates( ref_ptr<State>::cast(shader_) );
  }

  void set_spanMax(GLfloat spanMax) {
    spanMax_->setVertex1f(0,spanMax);
  }
  ref_ptr<ShaderInput1f>& spanMax() {
    return spanMax_;
  }
  void set_reduceMul(GLfloat reduceMul) {
    reduceMul_->setVertex1f(0,reduceMul);
  }
  ref_ptr<ShaderInput1f>& reduceMul() {
    return reduceMul_;
  }
  void set_reduceMin(GLfloat reduceMin) {
    reduceMin_->setVertex1f(0,reduceMin);
  }
  ref_ptr<ShaderInput1f>& reduceMin() {
    return reduceMin_;
  }
  void set_luma(const Vec3f &luma) {
    luma_->setVertex3f(0,luma);
  }
  ref_ptr<ShaderInput3f>& luma() {
    return luma_;
  }

  void set_numPixels(GLfloat numPixels);
  ref_ptr<ShaderInput1f> numPixels() const;
  virtual void set_parent(StateNode *parent)
  {
    StateNode::set_parent(parent);

    ShaderConfig shaderCfg;
    configureShader(&shaderCfg);
    shader_->createShader(shaderCfg, "fxaa");
  }
  ref_ptr<ShaderState> shader_;
  ref_ptr<Texture> input_;
  ref_ptr<ShaderInput1f> spanMax_;
  ref_ptr<ShaderInput1f> reduceMul_;
  ref_ptr<ShaderInput1f> reduceMin_;
  ref_ptr<ShaderInput3f> luma_;
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("FXAA");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  camManipulator->setStepLength(0.0f,0.0f);
  camManipulator->set_degree(0.0f,0.0f);
  camManipulator->set_radius(2.0f, 0.0f);
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  renderTree->setLight();

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    renderTree->addMesh(
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig)),
        modelMat,
        ref_ptr<Material>::manage(new Material));
  }

  renderTree->addSkyBox("res/textures/cube-stormydays.jpg");

  ref_ptr<State> drawBuffer = ref_ptr<State>::manage(new State);
  ref_ptr<DrawBufferState> drawBufferCallable_ =
      ref_ptr<DrawBufferState>::manage(new DrawBufferState);
  drawBufferCallable_->colorBuffers.push_back(GL_COLOR_ATTACHMENT1);
  drawBuffer->addEnabler(ref_ptr<Callable>::cast(drawBufferCallable_));
  ref_ptr<StateNode> aaParent = ref_ptr<StateNode>::manage(new StateNode(drawBuffer));

  ref_ptr<TextureState> inputTexState = ref_ptr<TextureState>::manage(
      new TextureState(renderTree->sceneTexture()));
  inputTexState->set_name("inputTexture");
  aaParent->state()->joinStates(ref_ptr<State>::cast(inputTexState));

  ref_ptr<AANode> aaNode = ref_ptr<AANode>::manage(
      new AANode(inputTexState->texture(), renderTree->orthoQuad()));
  application->addShaderInput(aaNode->luma(), 0.0f, 1.0f, 2);
  application->addShaderInput(aaNode->reduceMin(), 0.0f, 1.0f, 4);
  application->addShaderInput(aaNode->reduceMul(), 0.0f, 1.0f, 4);
  application->addShaderInput(aaNode->spanMax(), 0.0f, 100.0f, 1);
  renderTree->rootNode()->addChild(ref_ptr<StateNode>::cast(aaParent));
  aaParent->addChild(ref_ptr<StateNode>::cast(aaNode));

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  return application->mainLoop();
}
