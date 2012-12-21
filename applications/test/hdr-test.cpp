
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/render-tree/blur-node.h>
#include <ogle/textures/texture-loader.h>
#include <ogle/animations/animation-manager.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

#define USE_HDR

class TonemapNode : public StateNode
{
public:
  TonemapNode(
      ref_ptr<Texture> &input,
      ref_ptr<Texture> &blurredInput,
      ref_ptr<MeshState> &orthoQuad)
  : StateNode(),
    blurredInput_(blurredInput),
    input_(input)
  {
    blurAmount_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurAmount"));
    blurAmount_->setUniformData(0.5f);
    blurAmount_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(blurAmount_));

    effectAmount_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("effectAmount"));
    effectAmount_->setUniformData(0.2f);
    effectAmount_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(effectAmount_));

    exposure_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("exposure"));
    exposure_->setUniformData(16.0f);
    exposure_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(exposure_));

    gamma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("gamma"));
    gamma_->setUniformData(0.5f);
    gamma_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(gamma_));

    shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
    shader_->joinStates(ref_ptr<State>::cast(orthoQuad));
    state_->joinStates( ref_ptr<State>::cast(shader_) );
  }

  void set_blurAmount(GLfloat blurAmount) {
    blurAmount_->setVertex1f(0,blurAmount);
  }
  ref_ptr<ShaderInput1f>& blurAmount() {
    return blurAmount_;
  }
  void set_effectAmount(GLfloat effectAmount) {
    effectAmount_->setVertex1f(0,effectAmount);
  }
  ref_ptr<ShaderInput1f>& effectAmount() {
    return effectAmount_;
  }
  void set_exposure(GLfloat exposure) {
    exposure_->setVertex1f(0,exposure);
  }
  ref_ptr<ShaderInput1f>& exposure() {
    return exposure_;
  }
  void set_gamma(GLfloat gamma) {
    gamma_->setVertex1f(0,gamma);
  }
  ref_ptr<ShaderInput1f>& gamma() {
    return gamma_;
  }

  virtual void set_parent(StateNode *parent)
  {
    StateNode::set_parent(parent);

    ShaderConfig shaderCfg;
    configureShader(&shaderCfg);
    shader_->createShader(shaderCfg, "tonemap");
  }
  ref_ptr<ShaderState> shader_;
  ref_ptr<Texture> blurredInput_;
  ref_ptr<Texture> input_;
  ref_ptr<ShaderInput1f> effectAmount_;
  ref_ptr<ShaderInput1f> blurAmount_;
  ref_ptr<ShaderInput1f> exposure_;
  ref_ptr<ShaderInput1f> gamma_;
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

#ifdef USE_HDR
  const string skyImage = "res/textures/cube-grace.hdr";
  const GLboolean flipBackFace = GL_TRUE;
  const GLenum textureFormat = GL_R11F_G11F_B10F;
  const GLenum bufferFormat = GL_RGB16F;
#else
  const string skyImage = "res/textures/cube-stormydays.jpg";
  const GLboolean flipBackFace = GL_FALSE;
  const GLenum textureFormat = GL_RGBA;
  const GLenum bufferFormat = GL_RGBA;
#endif
  const GLfloat aniso = 2.0f;

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      bufferFormat,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  renderTree->setLight();
  camManipulator->set_radius(2.0f, 0.0);
  camManipulator->setStepLength(0.005f, 0.0);

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  ref_ptr<TextureCube> skyTex = TextureLoader::loadCube(
      skyImage,flipBackFace,GL_FALSE,textureFormat);
  skyTex->set_aniso(aniso);
  skyTex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  skyTex->setupMipmaps(GL_DONT_CARE);
  skyTex->set_wrapping(GL_CLAMP_TO_EDGE);

  ref_ptr<TextureState> refractionTexture =
      ref_ptr<TextureState>::manage(new TextureState(ref_ptr<Texture>::cast(skyTex)));
  refractionTexture->setMapTo(MAP_TO_COLOR);
  refractionTexture->set_blendMode(BLEND_MODE_SRC);
  refractionTexture->set_mapping(MAPPING_REFRACTION);

  ref_ptr<TextureState> reflectionTexture =
      ref_ptr<TextureState>::manage(new TextureState(ref_ptr<Texture>::cast(skyTex)));
  reflectionTexture->setMapTo(MAP_TO_COLOR);
  reflectionTexture->set_blendMode(BLEND_MODE_MIX);
  reflectionTexture->set_blendFactor(0.35f);
  reflectionTexture->set_mapping(MAPPING_REFLECTION);

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    sphereConfig.levelOfDetail = 5;
    ref_ptr<MeshState> meshState = ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::NO_SHADING );
    material->addTexture(refractionTexture);
    material->addTexture(reflectionTexture);

    renderTree->addMesh(meshState, modelMat, material);
  }

  renderTree->addSkyBox(skyTex);

  ref_ptr<DepthState> depthState = ref_ptr<DepthState>::manage(new DepthState);
  depthState->set_useDepthTest(GL_FALSE);
  depthState->set_useDepthWrite(GL_FALSE);

  ref_ptr<StateNode> &parent = renderTree->globalStates();
  ref_ptr<StateNode> hdrNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(depthState)));
  parent->addChild(hdrNode);
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
  depthTexture->set_name("sceneDepthTexture");
  depthState->joinStates(ref_ptr<State>::cast(depthTexture));

  // TODO: downscale two times by factor 0.5
  ref_ptr<BlurNode> blurNode = ref_ptr<BlurNode>::manage(new BlurNode(
      sceneTexture->texture(), renderTree->orthoQuad(), 0.5f));
  blurNode->set_sigma(6.0f);
  blurNode->set_numPixels(14.0f);
  application->addShaderInput(blurNode->sigma(), 0.0f, 25.0f, 3);
  application->addShaderInput(blurNode->numPixels(), 0.0f, 50.0f, 0);
  ref_ptr<Texture> &blurTexture = blurNode->blurredTexture();
  hdrNode->addChild(ref_ptr<StateNode>::cast(blurNode));

#ifdef USE_HDR
  // TODO: more tonemap uniforms
  // tonemap switches back to scene FBO and renders to GL_COLOR_ATTACHMENT1
  ref_ptr<FBOState> tonemapFBO = ref_ptr<FBOState>::manage(new FBOState(fboState->fbo()));
  tonemapFBO->addDrawBuffer(GL_COLOR_ATTACHMENT1);
  ref_ptr<StateNode> tonemapParent = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(tonemapFBO)));

  ref_ptr<TextureState> blurTexState = ref_ptr<TextureState>::manage(
      new TextureState(blurTexture));
  blurTexState->set_name("blurTexture");
  tonemapParent->state()->joinStates(ref_ptr<State>::cast(blurTexState));
  ref_ptr<TextureState> inputTexState = ref_ptr<TextureState>::manage(
      new TextureStateNoChannel(sceneTexture));
  inputTexState->set_name("inputTexture");
  tonemapParent->state()->joinStates(ref_ptr<State>::cast(inputTexState));

  ref_ptr<TonemapNode> tonemapNode = ref_ptr<TonemapNode>::manage(
      new TonemapNode(sceneTexture->texture(), blurTexture, renderTree->orthoQuad()));
  application->addShaderInput(tonemapNode->blurAmount(), 0.0f, 1.0f, 3);
  application->addShaderInput(tonemapNode->effectAmount(), 0.0f, 1.0f, 3);
  application->addShaderInput(tonemapNode->exposure(), 0.0f, 50.0f, 3);
  application->addShaderInput(tonemapNode->gamma(), 0.0f, 10.0f, 2);
  hdrNode->addChild(ref_ptr<StateNode>::cast(tonemapParent));
  tonemapParent->addChild(ref_ptr<StateNode>::cast(tonemapNode));
#endif

  renderTree->setShowFPS();

#ifdef USE_HDR
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);
#else
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
#endif

  return application->mainLoop();
}
