
#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/shadows/directional-shadow-map.h>
#include <ogle/shadows/spot-shadow-map.h>
#include <ogle/shadows/point-shadow-map.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/render-tree/shader-configurer.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

#define USE_SPOT_LIGHT
#ifdef USE_SPOT_LIGHT
  #define USE_SPOT_SHADOW
#endif

class AANode : public StateNode
{
public:
  AANode(
      const ref_ptr<Texture> &input,
      const ref_ptr<MeshState> &orthoQuad)
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
  const ref_ptr<ShaderInput1f>& spanMax() const {
    return spanMax_;
  }
  void set_reduceMul(GLfloat reduceMul) {
    reduceMul_->setVertex1f(0,reduceMul);
  }
  const ref_ptr<ShaderInput1f>& reduceMul() const {
    return reduceMul_;
  }
  void set_reduceMin(GLfloat reduceMin) {
    reduceMin_->setVertex1f(0,reduceMin);
  }
  const ref_ptr<ShaderInput1f>& reduceMin() const {
    return reduceMin_;
  }
  void set_luma(const Vec3f &luma) {
    luma_->setVertex3f(0,luma);
  }
  const ref_ptr<ShaderInput3f>& luma() const {
    return luma_;
  }

  virtual void set_parent(StateNode *parent)
  {
    StateNode::set_parent(parent);
    shader_->createShader(ShaderConfigurer::configure(this), "fxaa");
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
  const GLuint shadowMapSize = 2048;
  const GLenum internalFormat = GL_DEPTH_COMPONENT16;
  const GLenum pixelType = GL_BYTE;
  ShadowMap::FilterMode spotShadowFilter = ShadowMap::SINGLE;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("Testing transparency");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  camManipulator->setStepLength(0.0f,0.0f);
  camManipulator->set_height(3.0f,0.0f);
  camManipulator->set_degree(1.85*M_PI,0.0f);
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<PerspectiveCamera> &sceneCamera = renderTree->perspectiveCamera();
  ref_ptr<Frustum> sceneFrustum = ref_ptr<Frustum>::manage(new Frustum);
  sceneFrustum->setProjection(sceneCamera->fov(),
      sceneCamera->aspect(), sceneCamera->near(), sceneCamera->far());

#ifdef USE_SPOT_LIGHT
  ref_ptr<SpotLight> spotLight =
      ref_ptr<SpotLight>::manage(new SpotLight);
  spotLight->set_position(Vec3f(-1.5f,3.7f,4.5f));
  spotLight->set_spotDirection(Vec3f(0.4f,-1.0f,-1.0f));
  spotLight->set_diffuse(Vec3f(1.0f));
  spotLight->set_innerConeAngle(35.0f);
  spotLight->set_outerConeAngle(0.0f);
  spotLight->set_constantAttenuation(0.15f);
  spotLight->set_linearAttenuation(0.12f);
  spotLight->set_quadricAttenuation(0.015f);
  renderTree->setLight(ref_ptr<Light>::cast(spotLight));
#endif
#ifdef USE_SPOT_SHADOW
  ref_ptr<SpotShadowMap> spotShadow = ref_ptr<SpotShadowMap>::manage(
      new SpotShadowMap(spotLight, sceneCamera,
          shadowMapSize, internalFormat, pixelType));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(spotShadow));
  spotShadow->set_filteringMode(spotShadowFilter);
#endif

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGB,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_FALSE,
      Vec4f(0.0f)
  );
  renderTree->setTransparencyMode(TRANSPARENCY_MODE_FRONT_TO_BACK);

  renderTree->addDynamicSky();
  DynamicSky *sky = (DynamicSky*) renderTree->skyBox().get();
  application->addShaderInput(sky->rayleigh(), 0.0f, 10.0f, 2);
  application->addShaderInput(sky->mie(), 0.0f, 10.0f, 2);
  application->addShaderInput(sky->spotBrightness(), 0.0f, 1000.0f, 2);
  application->addShaderInput(sky->scatterStrength(), 0.0f, 0.1f, 4);
  application->addShaderInput(sky->absorbtion(), 0.0f, 1.0f, 2);

  ref_ptr<ModelTransformationState> modelMat;

  ref_ptr<MeshState> mesh;
  Box::Config cubeConfig;
  cubeConfig.texcoMode = Box::TEXCO_MODE_NONE;
  cubeConfig.posScale = Vec3f(1.0f, 1.0f, 0.1f);
  {
    mesh = ref_ptr<MeshState>::manage(new Box(cubeConfig));
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.49f, 1.0f), 0.0f);
    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_pewter();
    material->set_alpha(0.5f);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);
    renderTree->addMesh(mesh, modelMat, material, "mesh.transparent", GL_TRUE);
  }
  {
    mesh = ref_ptr<MeshState>::manage(new Box(cubeConfig));
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.49f, -0.25f), 0.0f);
    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_ruby();
    renderTree->addMesh(mesh, modelMat,  material);
  }
  {
    mesh = ref_ptr<MeshState>::manage(new Box(cubeConfig));
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.15f, 0.4f, -1.5f), 0.0f);
    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_jade();
    material->set_alpha(0.88f);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);
    renderTree->addMesh(mesh, modelMat, material, "mesh.transparent", GL_TRUE);
  }
  {
    mesh = ref_ptr<MeshState>::manage(new Box(cubeConfig));
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.3f, -2.75f), 0.0f);
    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_gold();
    material->set_alpha(0.66f);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);
    renderTree->addMesh(mesh, modelMat, material, "mesh.transparent", GL_TRUE);
  }
  {
    Rectangle::Config quadConfig;
    quadConfig.levelOfDetail = 0;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.0*M_PI, 0.0*M_PI, 1.0*M_PI);
    quadConfig.posScale = Vec3f(10.0f, 10.0f, 10.0f);
    quadConfig.texcoScale = Vec2f(2.0f, 2.0f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new Rectangle(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, -0.49f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_chrome();
    material->set_specular(Vec3f(0.0f));
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

#ifdef USE_SPOT_SHADOW
  spotShadow->addCaster(renderTree->perspectivePass());
  spotShadow->addCaster(renderTree->transparencyPass());
#endif

  ref_ptr<State> drawBuffer = ref_ptr<State>::manage(new State);
  ref_ptr<DrawBufferState> drawBufferCallable_ =
      ref_ptr<DrawBufferState>::manage(new DrawBufferState);
  drawBufferCallable_->colorBuffers.push_back(GL_COLOR_ATTACHMENT1);
  drawBuffer->joinStates(ref_ptr<State>::cast(drawBufferCallable_));
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

  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  return application->mainLoop();
}
