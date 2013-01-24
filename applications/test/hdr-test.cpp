
#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/render-tree/blur-node.h>
#include <ogle/textures/texture-loader.h>
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

#define USE_HDR

class TonemapNode : public StateNode
{
public:
  TonemapNode(
      const ref_ptr<Texture> &input,
      const ref_ptr<Texture> &blurredInput)
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

    radialBlurSamples_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("radialBlurSamples"));
    radialBlurSamples_->setUniformData(30.0f);
    radialBlurSamples_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(radialBlurSamples_));

    radialBlurStartScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("radialBlurStartScale"));
    radialBlurStartScale_->setUniformData(1.0f);
    radialBlurStartScale_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(radialBlurStartScale_));

    radialBlurScaleMul_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("radialBlurScaleMul"));
    radialBlurScaleMul_->setUniformData(0.9f);
    radialBlurScaleMul_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(radialBlurScaleMul_));

    vignetteInner_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("vignetteInner"));
    vignetteInner_->setUniformData(0.7f);
    vignetteInner_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(vignetteInner_));

    vignetteOuter_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("vignetteOuter"));
    vignetteOuter_->setUniformData(1.5f);
    vignetteOuter_->set_isConstant(GL_TRUE);
    state_->joinShaderInput(ref_ptr<ShaderInput>::cast(vignetteOuter_));

    shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
    shader_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
    state_->joinStates(ref_ptr<State>::cast(shader_));
  }

  void set_blurAmount(GLfloat blurAmount) {
    blurAmount_->setVertex1f(0,blurAmount);
  }
  const ref_ptr<ShaderInput1f>& blurAmount() const {
    return blurAmount_;
  }
  void set_effectAmount(GLfloat effectAmount) {
    effectAmount_->setVertex1f(0,effectAmount);
  }
  const ref_ptr<ShaderInput1f>& effectAmount() const {
    return effectAmount_;
  }
  void set_exposure(GLfloat exposure) {
    exposure_->setVertex1f(0,exposure);
  }
  const ref_ptr<ShaderInput1f>& exposure() const {
    return exposure_;
  }
  void set_gamma(GLfloat gamma) {
    gamma_->setVertex1f(0,gamma);
  }
  const ref_ptr<ShaderInput1f>& gamma() const {
    return gamma_;
  }
  void set_radialBlurSamples(GLfloat v) {
    radialBlurSamples_->setVertex1f(0,v);
  }
  const ref_ptr<ShaderInput1f>& radialBlurSamples() const {
    return radialBlurSamples_;
  }
  void set_radialBlurStartScale(GLfloat v) {
    radialBlurStartScale_->setVertex1f(0,v);
  }
  const ref_ptr<ShaderInput1f>& radialBlurStartScale() const {
    return radialBlurStartScale_;
  }
  void set_radialBlurScaleMul(GLfloat v) {
    radialBlurScaleMul_->setVertex1f(0,v);
  }
  const ref_ptr<ShaderInput1f>& radialBlurScaleMul() const {
    return radialBlurScaleMul_;
  }
  void set_vignetteInner(GLfloat v) {
    vignetteInner_->setVertex1f(0,v);
  }
  const ref_ptr<ShaderInput1f>& vignetteInner() const {
    return vignetteInner_;
  }
  void set_vignetteOuter(GLfloat v) {
    vignetteOuter_->setVertex1f(0,v);
  }
  const ref_ptr<ShaderInput1f>& vignetteOuter() const {
    return vignetteOuter_;
  }

  virtual void set_parent(StateNode *parent)
  {
    StateNode::set_parent(parent);
    shader_->createShader(ShaderConfigurer::configure(this), "tonemap");
  }
  ref_ptr<ShaderState> shader_;
  ref_ptr<Texture> blurredInput_;
  ref_ptr<Texture> input_;
  ref_ptr<ShaderInput1f> effectAmount_;
  ref_ptr<ShaderInput1f> blurAmount_;
  ref_ptr<ShaderInput1f> exposure_;
  ref_ptr<ShaderInput1f> gamma_;
  ref_ptr<ShaderInput1f> radialBlurSamples_;
  ref_ptr<ShaderInput1f> radialBlurStartScale_;
  ref_ptr<ShaderInput1f> radialBlurScaleMul_;
  ref_ptr<ShaderInput1f> vignetteInner_;
  ref_ptr<ShaderInput1f> vignetteOuter_;
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
    Sphere::Config sphereConfig;
    sphereConfig.texcoMode = Sphere::TEXCO_MODE_NONE;
    sphereConfig.levelOfDetail = 5;
    ref_ptr<MeshState> meshState = ref_ptr<MeshState>::manage(new Sphere(sphereConfig));

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
  ref_ptr<BlurNode> blurNode = ref_ptr<BlurNode>::manage(new BlurNode(sceneTexture->texture(), 0.5f));
  blurNode->set_sigma(6.0f);
  blurNode->set_numPixels(14.0f);
  application->addShaderInput(blurNode->sigma(), 0.0f, 25.0f, 3);
  application->addShaderInput(blurNode->numPixels(), 0.0f, 50.0f, 0);
  const ref_ptr<Texture> &blurTexture = blurNode->blurredTexture();
  hdrNode->addChild(ref_ptr<StateNode>::cast(blurNode));

#ifdef USE_HDR
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
      new TonemapNode(sceneTexture->texture(), blurTexture));
  application->addShaderInput(tonemapNode->blurAmount(), 0.0f, 1.0f, 3);
  application->addShaderInput(tonemapNode->effectAmount(), 0.0f, 1.0f, 3);
  application->addShaderInput(tonemapNode->exposure(), 0.0f, 50.0f, 3);
  application->addShaderInput(tonemapNode->gamma(), 0.0f, 10.0f, 2);

  application->addShaderInput(tonemapNode->radialBlurSamples(), 0.0f, 100.0f, 0);
  application->addShaderInput(tonemapNode->radialBlurStartScale(), 0.0f, 1.0f, 3);
  application->addShaderInput(tonemapNode->radialBlurScaleMul(), 0.0f, 1.0f, 4);

  application->addShaderInput(tonemapNode->vignetteInner(), 0.0f, 10.0f, 2);
  application->addShaderInput(tonemapNode->vignetteOuter(), 0.0f, 10.0f, 2);
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
