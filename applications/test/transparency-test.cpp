
#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/meshes/rain-particles.h>
#include <ogle/meshes/snow-particles.h>
#include <ogle/shadows/directional-shadow-map.h>
#include <ogle/shadows/spot-shadow-map.h>
#include <ogle/shadows/point-shadow-map.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/render-tree/shader-configurer.h>
#include <ogle/textures/texture-loader.h>
#include <ogle/config.h>

#include <applications/application-config.h>
#include <applications/fltk-ogle-application.h>

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

#define USE_SPOT_LIGHT
#ifdef USE_SPOT_LIGHT
  #define USE_SPOT_SHADOW
#endif

#define USE_SNOW
// #define USE_RAIN

class AANode : public StateNode
{
public:
  AANode(const ref_ptr<Texture> &input)
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
    shader_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
    state_->joinStates(ref_ptr<State>::cast(shader_) );
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

  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
  application->set_windowTitle("Testing transparency");
  application->show();
  boost::filesystem::path shaderPath(PROJECT_SOURCE_DIR);
  shaderPath /= "applications";
  shaderPath /= "test";
  shaderPath /= "shader";
  OGLEApplication::setupGLSWPath(shaderPath);

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
    material->alpha()->setUniformData(0.5f);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);
    renderTree->addMesh(mesh, modelMat, material, "transparent-mesh", GL_TRUE);
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
    material->alpha()->setUniformData(0.88f);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);
    renderTree->addMesh(mesh, modelMat, material, "transparent-mesh", GL_TRUE);
  }
  {
    mesh = ref_ptr<MeshState>::manage(new Box(cubeConfig));
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.3f, -2.75f), 0.0f);
    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_gold();
    material->alpha()->setUniformData(0.66f);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);
    renderTree->addMesh(mesh, modelMat, material, "transparent-mesh", GL_TRUE);
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
    material->specular()->setUniformData(Vec3f(0.0f));
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

#ifdef USE_RAIN
  {
    const GLuint numRainDrops = 5000;
    const GLboolean useAlpha = GL_FALSE;

    ref_ptr<RainParticles> rainParticles =
        ref_ptr<RainParticles>::manage(new RainParticles(numRainDrops));
    rainParticles->createBuffer();

    modelMat = ref_ptr<ModelTransformationState>();
    ref_ptr<Material> material = ref_ptr<Material>();

    ref_ptr<StateNode> node = renderTree->addMesh(ref_ptr<MeshState>::cast(rainParticles),
        modelMat, material, "", useAlpha);
    ShaderConfig shaderCfg = ShaderConfigurer::configure(node.get());

    rainParticles->createShader(shaderCfg);
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(rainParticles));

    application->addShaderInput(rainParticles->dampingFactor(), 0.0f, 10.0f, 3);
    application->addShaderInput(rainParticles->noiseFactor(), 0.0f, 10.0f, 3);
    application->addShaderInput(rainParticles->strandSize(), 0.0f, 1.0f, 5);
    application->addShaderInput(rainParticles->cloudPosition(), -10.0f, 10.0f, 2);
    application->addShaderInput(rainParticles->gravity(), -100.0f, 100.0f, 1);
    application->addShaderInput(rainParticles->snowFlakeMass(), 0.0f, 10.0f, 3);
    //application->addShaderInput(rainParticles->maxNumParticleEmits(), 0.0f, 100.0f, 0);
  }
#endif

#ifdef USE_SNOW
  {
    const GLuint numSnowFlakes = 5000;

    ref_ptr<SnowParticles> particles =
        ref_ptr<SnowParticles>::manage(new SnowParticles(numSnowFlakes));
    ref_ptr<Texture> tex = TextureLoader::load("res/textures/flare.jpg");
    particles->set_snowFlakeTexture(tex);
    particles->set_depthTexture(renderTree->sceneDepthTexture());
    particles->createBuffer();

    modelMat = ref_ptr<ModelTransformationState>();
    ref_ptr<Material> material = ref_ptr<Material>();

    ref_ptr<StateNode> node = renderTree->addMesh(
        renderTree->backgroundPass(),
        ref_ptr<MeshState>::cast(particles),
        modelMat, material, "");
    ShaderConfig shaderCfg = ShaderConfigurer::configure(node.get());

    particles->createShader(shaderCfg);
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(particles));

    /*
    application->addShaderInput(particles->dampingFactor(), 0.0f, 10.0f, 3);
    application->addShaderInput(particles->noiseFactor(), 0.0f, 10.0f, 3);
    application->addShaderInput(particles->snowFlakeSize(), 0.0f, 1.0f, 5);
    application->addShaderInput(particles->cloudPosition(), -10.0f, 10.0f, 2);
    application->addShaderInput(particles->gravity(), -100.0f, 100.0f, 1);
    application->addShaderInput(particles->snowFlakeMass(), 0.0f, 10.0f, 3);
    application->addShaderInput(particles->softScale(), 0.0f, 100.0f, 2);
    */
  }
#endif

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

  ref_ptr<AANode> aaNode = ref_ptr<AANode>::manage(new AANode(inputTexState->texture()));
  renderTree->rootNode()->addChild(ref_ptr<StateNode>::cast(aaParent));
  aaParent->addChild(ref_ptr<StateNode>::cast(aaNode));

  renderTree->setShowFPS();

  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  return application->mainLoop();
}
