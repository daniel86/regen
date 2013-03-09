
#include "factory.h"
using namespace ogle;

#define USE_SPOT_LIGHT
#define USE_POINT_LIGHT
//#define USE_SKY
#define USE_HUD
//#define USE_FXAA
//#define USE_SNOW

void createBox(OGLEFltkApplication *app, const ref_ptr<StateNode> &root,
    const Vec3f &position, const GLfloat &alpha)
{
  Box::Config cfg;
  cfg.texcoMode = Box::TEXCO_MODE_NONE;
  cfg.posScale = Vec3f(1.0f, 1.0f, 0.1f);

  ref_ptr<MeshState> mesh = ref_ptr<MeshState>::manage(new Box(cfg));

  ref_ptr<ModelTransformation> modelMat =
      ref_ptr<ModelTransformation>::manage(new ModelTransformation);
  modelMat->translate(position, 0.0f);
  mesh->joinStates(ref_ptr<State>::cast(modelMat));

  ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
  static int materialIndex = 0; materialIndex = (materialIndex+1)%4;
  switch( materialIndex ) {
  case 0: material->set_pewter(); break;
  case 1: material->set_ruby(); break;
  case 2: material->set_jade(); break;
  default: material->set_gold(); break;
  }
  mesh->joinStates(ref_ptr<State>::cast(material));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  mesh->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());

  if(alpha<0.999) {
    material->alpha()->setUniformData(alpha);
    app->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);
    shaderState->createShader(shaderConfigurer.cfg(), "transparent_mesh");
  }
  else {
    shaderState->createShader(shaderConfigurer.cfg(), "mesh");
  }
}

int main(int argc, char** argv)
{
  ref_ptr<OGLEFltkApplication> app = initApplication(argc,argv,"Transparency");

  // create a root node for everything that needs camera as input
  ref_ptr<Camera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 2.2f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 9.0f );
  manipulator->set_degree( M_PI*0.1 );
  manipulator->setStepLength( M_PI*0.0 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  ref_ptr<SpotLight> spotLight = createSpotLight(app.get());
  spotLight->set_specular(Vec3f(0.0));
  spotLight->set_diffuse(Vec3f(0.6));
  spotLight->set_position(Vec3f(1.0,5.0,4.0));
  spotLight->set_spotDirection(Vec3f(-0.2,-0.5,-0.3));
  spotLight->set_innerRadius(9.0);
  spotLight->set_outerRadius(11.0);
  spotLight->coneAngle()->setVertex2f(0, Vec2f(0.9,0.8));
  ref_ptr<SpotShadowMap> spotShadow = createSpotShadow(app.get(), spotLight, cam, 1024);
  ShadowMap::FilterMode spotShadowFilter = ShadowMap::FILTERING_VSM;
  if(glsl_useShadowMoments(spotShadowFilter)) {
    spotShadow->setComputeMoments();
    spotShadow->setCullFrontFaces(GL_FALSE);
    spotShadow->createBlurFilter(3, 2.0, GL_FALSE);
  }

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get());
  ref_ptr<StateNode> gBufferParent = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gBufferState)));
  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage( new StateNode);
  sceneRoot->addChild(gBufferParent);
  gBufferParent->addChild(gBufferNode);
  spotShadow->addCaster(gBufferNode);

  ref_ptr<Texture> gDiffuseTexture = gBufferState->fbo()->colorBuffer()[0];
  ref_ptr<Texture> gDepthTexture = gBufferState->fbo()->depthTexture();
  sceneRoot->addChild(gBufferNode);
  createBox(app.get(), gBufferNode, Vec3f(0.0f, 0.49f, -0.25f), 1.0f);
  createFloorMesh(app.get(), gBufferNode,
      -0.5f, Vec3f(100.0f), Vec2f(40.0f), TRANSFER_TEXCO_PARALLAX);

  const TransparencyMode alphaMode = TRANSPARENCY_MODE_FRONT_TO_BACK;
  ref_ptr<TransparencyState> tBufferState = createTBuffer(app.get(), cam, gDepthTexture, alphaMode);
  ref_ptr<StateNode> tBufferNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(tBufferState)));
  switch(alphaMode) {
  case TRANSPARENCY_MODE_BACK_TO_FRONT:
    tBufferState->joinStatesFront(ref_ptr<State>::manage(
        new SortByModelMatrix(tBufferNode, cam, GL_FALSE)));
    break;
  case TRANSPARENCY_MODE_FRONT_TO_BACK:
    tBufferState->joinStatesFront(ref_ptr<State>::manage(
        new SortByModelMatrix(tBufferNode, cam, GL_TRUE)));
    break;
  default:
    break;
  }
  // TBuffer uses direct lighting
  tBufferState->addLight(
      ref_ptr<Light>::cast(spotLight),
      ref_ptr<ShadowMap>::cast(spotShadow),
      spotShadowFilter);
  sceneRoot->addChild(tBufferNode);
  spotShadow->addCaster(tBufferNode);
  createBox(app.get(), tBufferNode, Vec3f(0.0f, 0.49f, 1.0f), 0.5f);
  createBox(app.get(), tBufferNode, Vec3f(0.15f, 0.4f, -1.5f), 0.88f);
  createBox(app.get(), tBufferNode, Vec3f(0.0f, 0.3f, -2.75f), 0.66f);

  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gBufferState->fbo(), sceneRoot, spotShadowFilter);

  deferredShading->addLight(
      ref_ptr<Light>::cast(spotLight),
      ref_ptr<ShadowMap>::cast(spotShadow));
  {
    const ref_ptr<FilterSequence> &momentsFilter = spotShadow->momentsFilter();
    ShaderConfigurer _cfg;
    _cfg.addNode(sceneRoot.get());
    _cfg.addState(momentsFilter.get());
    momentsFilter->createShader(_cfg.cfg());
  }

  ref_ptr<FBOState> postPassState = ref_ptr<FBOState>::manage(
      new FBOState(gBufferState->fbo()));
  ref_ptr<StateNode> postPassNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(postPassState)));
  sceneRoot->addChild(postPassNode);

  // Combine TBuffer and shaded GBuffer
  ref_ptr<State> resolveAlpha = resolveTransparency(
      app.get(), tBufferState, postPassNode);
  resolveAlpha->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
      gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_TRUE)));

  ref_ptr<DirectShading> directShading =
      ref_ptr<DirectShading>::manage(new DirectShading);
  directShading->addLight(ref_ptr<Light>::cast(spotLight));
  ref_ptr<StateNode> directShadingNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(directShading)));
  postPassNode->addChild(directShadingNode);
#ifdef USE_SNOW
  ref_ptr<ParticleSnow> snowParticles = createSnow(
      app.get(), gDepthTexture, directShadingNode, 5000);
  snowParticles->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
      gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_TRUE)));
#endif

#ifdef USE_FXAA
  ref_ptr<AntiAliasing> aa = createAAState(
      app.get(), gDiffuseTexture, postPassNode);
  aa->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
      gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));
#endif

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createFPSWidget(app.get(), guiNode);
  //createTextureWidget(app.get(), guiNode,
  //    spotShadow->shadowMomentsUnfiltered(), Vec2ui(50u,0u), 200.0f);
  //createTextureWidget(app.get(), guiNode,
  //    spotShadow->shadowMoments(), Vec2ui(450u,0u), 200.0f);
#endif

  setBlitToScreen(app.get(), gBufferState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
