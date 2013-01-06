
#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/render-tree/blur-node.h>
#include <ogle/render-tree/shader-configurer.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

class MotionBlurNode : public StateNode
{
public:
  MotionBlurNode(
      TestRenderTree *renderTree,
      const ref_ptr<Texture> &input,
      const ref_ptr<MeshState> &orthoQuad)
  : StateNode(),
    renderTree_(renderTree),
    input_(input)
  {
    numMotionBlurSamples_ = ref_ptr<ShaderInput1i>::manage(new ShaderInput1i("numMotionBlurSamples"));
    numMotionBlurSamples_->setUniformData(10);
    numMotionBlurSamples_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(numMotionBlurSamples_));

    velocityScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("velocityScale"));
    velocityScale_->setUniformData(0.25f);
    velocityScale_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(velocityScale_));

    lastViewProjectionMat_ = ref_ptr<ShaderInputMat4>::manage(
        new ShaderInputMat4("lastViewProjectionMatrix"));
    lastViewProjectionMat_->setUniformData(Mat4f::identity());
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(lastViewProjectionMat_));

    shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
    shader_->joinStates(ref_ptr<State>::cast(orthoQuad));
    state_->joinStates( ref_ptr<State>::cast(shader_) );
  }

  void set_numSamples(GLint numSamples) {
    numMotionBlurSamples_->setVertex1i(0,numSamples);
  }
  const ref_ptr<ShaderInput1i>& numSamples() const {
    return numMotionBlurSamples_;
  }

  void set_velocityScale(GLfloat velocityScale) {
    velocityScale_->setVertex1f(0,velocityScale);
  }
  const ref_ptr<ShaderInput1f>& velocityScale() const {
    return velocityScale_;
  }

  virtual void set_parent(StateNode *parent)
  {
    StateNode::set_parent(parent);
    shader_->createShader(ShaderConfigurer::configure(this), "motion-blur");
  }

  virtual void disable(RenderState *rs) {
    StateNode::disable(rs);
    // remember last view projection
    ref_ptr<PerspectiveCamera> &cam = renderTree_->perspectiveCamera();
    ShaderInputMat4 *m = cam->viewProjectionUniform().get();
    lastViewProjectionMat_->setUniformData(m->getVertex16f(0));
  }
  ref_ptr<ShaderInputMat4> lastViewProjectionMat_;
  TestRenderTree *renderTree_;

  ref_ptr<ShaderState> shader_;
  ref_ptr<Texture> input_;
  ref_ptr<ShaderInput1i> numMotionBlurSamples_;
  ref_ptr<ShaderInput1f> velocityScale_;
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("MotionBlur test");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_FALSE,
      Vec4f(0.0f)
  );

  renderTree->setLight();
  camManipulator->set_radius(2.0f, 0.0);
  camManipulator->setStepLength(0.005f, 0.0);

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  {
    Sphere::Config sphereConfig;
    sphereConfig.texcoMode = Sphere::TEXCO_MODE_NONE;
    sphereConfig.levelOfDetail = 5;
    ref_ptr<MeshState> meshState = ref_ptr<MeshState>::manage(new Sphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_ruby();

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

  ref_ptr<FBOState> motionBlurFBO = ref_ptr<FBOState>::manage(new FBOState(fboState->fbo()));
  motionBlurFBO->addDrawBuffer(GL_COLOR_ATTACHMENT1);
  ref_ptr<StateNode> motionBlurParent = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(motionBlurFBO)));

  ref_ptr<TextureState> inputTexState = ref_ptr<TextureState>::manage(
      new TextureStateNoChannel(sceneTexture));
  inputTexState->set_name("inputTexture");
  motionBlurParent->state()->joinStates(ref_ptr<State>::cast(inputTexState));

  ref_ptr<MotionBlurNode> motionBlurNode = ref_ptr<MotionBlurNode>::manage(
      new MotionBlurNode(renderTree,sceneTexture->texture(), renderTree->orthoQuad()));
  application->addShaderInput(motionBlurNode->numSamples(), 0, 40);
  application->addShaderInput(motionBlurNode->velocityScale(), 0.0f, 10.0f, 2);
  parentNode->addChild(ref_ptr<StateNode>::cast(motionBlurParent));
  motionBlurParent->addChild(ref_ptr<StateNode>::cast(motionBlurNode));

  ///////////
  ///////////

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  renderTree->addSkyBox("res/textures/cube-stormydays.jpg");
  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  return application->mainLoop();
}
