
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/textures/cube-image-texture.h>
#include <ogle/animations/animation-manager.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

class MotionBlur : public ShaderState
{
public:
  ref_ptr<ShaderInputMat4> lastViewProjectionMat_;
  TestRenderTree *renderTree_;

  MotionBlur(TestRenderTree *renderTree)
  : ShaderState(),
    renderTree_(renderTree)
  {
    map<string, string> shaderConfig_;
    map<GLenum, string> shaderNames_;
    shaderNames_[GL_VERTEX_SHADER] = "motion-blur.vs";
    shaderNames_[GL_FRAGMENT_SHADER] = "motion-blur.fs";
    createSimple(shaderConfig_, shaderNames_);

    ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
    depthState->set_useDepthTest(GL_FALSE);
    depthState->set_useDepthWrite(GL_FALSE);
    joinStates(ref_ptr<State>::cast(depthState));

    lastViewProjectionMat_ = ref_ptr<ShaderInputMat4>::manage(
        new ShaderInputMat4("lastViewProjectionMatrix"));
    lastViewProjectionMat_->setUniformData(identity4f());
    joinShaderInput(ref_ptr<ShaderInput>::cast(lastViewProjectionMat_));
  }

  virtual void disable(RenderState *rs) {
    ShaderState::disable(rs);

    // remember last view projection
    ref_ptr<PerspectiveCamera> &cam = renderTree_->perspectiveCamera();
    ShaderInputMat4 *m = cam->viewProjectionUniform();
    lastViewProjectionMat_->setUniformData(m->getVertex16f(0));
  }

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
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    sphereConfig.levelOfDetail = 5;
    ref_ptr<MeshState> meshState = ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::PHONG_SHADING );

    renderTree->addMesh(meshState, modelMat, material);
  }

  {
    ref_ptr<MotionBlur> blurState = ref_ptr<MotionBlur>::manage(
        new MotionBlur(renderTree));
    ref_ptr<StateNode> blurNode = renderTree->addOrthoPass(
        ref_ptr<State>::cast(blurState));

    ShaderConfig shaderCfg;
    blurNode->configureShader(&shaderCfg);
    ref_ptr<Shader> shader = blurState->shader();
    if(shader->compile() && shader->link()) {
      shader->setInputs(shaderCfg.inputs());
    }
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  renderTree->addSkyBox("res/textures/cube-stormydays.jpg");
  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  return application->mainLoop();
}
