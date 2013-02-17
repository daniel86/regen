
#include "factory.h"

#define USE_SPOT_LIGHT
#define USE_POINT_LIGHT
#define USE_SKY
#define USE_HUD

int main(int argc, char** argv)
{
  static const string assimpMeshFile = "res/models/psionic/dwarf/x/dwarf2.x";
  static const string assimpMeshTexturesPath = "res/models/psionic/dwarf/x";
  static const BoneAnimRange animRanges[] = {
      (BoneAnimRange) {"none",        Vec2d(  -1.0,  -1.0 )},
      (BoneAnimRange) {"complete",    Vec2d(   0.0, 361.0 )},
      (BoneAnimRange) {"run",         Vec2d(  16.0,  26.0 )},
      (BoneAnimRange) {"jump",        Vec2d(  28.0,  40.0 )},
      (BoneAnimRange) {"jumpSpot",    Vec2d(  42.0,  54.0 )},
      (BoneAnimRange) {"crouch",      Vec2d(  56.0,  59.0 )},
      (BoneAnimRange) {"crouchLoop",  Vec2d(  60.0,  69.0 )},
      (BoneAnimRange) {"getUp",       Vec2d(  70.0,  74.0 )},
      (BoneAnimRange) {"battleIdle1", Vec2d(  75.0,  88.0 )},
      (BoneAnimRange) {"battleIdle2", Vec2d(  90.0, 110.0 )},
      (BoneAnimRange) {"attack1",     Vec2d( 112.0, 126.0 )},
      (BoneAnimRange) {"attack2",     Vec2d( 128.0, 142.0 )},
      (BoneAnimRange) {"attack3",     Vec2d( 144.0, 160.0 )},
      (BoneAnimRange) {"attack4",     Vec2d( 162.0, 180.0 )},
      (BoneAnimRange) {"attack5",     Vec2d( 182.0, 192.0 )},
      (BoneAnimRange) {"block",       Vec2d( 194.0, 210.0 )},
      (BoneAnimRange) {"dieFwd",      Vec2d( 212.0, 227.0 )},
      (BoneAnimRange) {"dieBack",     Vec2d( 230.0, 251.0 )},
      (BoneAnimRange) {"yes",         Vec2d( 253.0, 272.0 )},
      (BoneAnimRange) {"no",          Vec2d( 274.0, 290.0 )},
      (BoneAnimRange) {"idle1",       Vec2d( 292.0, 325.0 )},
      (BoneAnimRange) {"idle2",       Vec2d( 327.0, 360.0 )}
  };

  ref_ptr<OGLEFltkApplication> app = initApplication(argc,argv,"Assimp Model and Bones");
  // global config
  DirectionalShadowMap::set_numSplits(3);

  // create a root node for everything that needs camera as input
  ref_ptr<PerspectiveCamera> cam = createPerspectiveCamera(app.get());
  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->rootNode()->addChild(sceneRoot);

  // XXX frustum does not update when light attributes change
  ref_ptr<Frustum> frustum = ref_ptr<Frustum>::manage(new Frustum);
  frustum->setProjection(cam->fov(), cam->aspect(), cam->near(), cam->far());

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get());
  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gBufferState)));
  ref_ptr<Texture> gDiffuseTexture = gBufferState->fbo()->colorBuffer()[0];
  sceneRoot->addChild(gBufferNode);
  createAssimpMesh(
        app.get(), gBufferNode
      , assimpMeshFile
      , assimpMeshTexturesPath
      , Mat4f::rotationMatrix(0.0f,M_PI,0.0f)
      , Vec3f(0.0f,-2.0f,0.0f)
      , animRanges, sizeof(animRanges)/sizeof(BoneAnimRange)
  );
  createFloorMesh(app.get(), gBufferNode);

  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gBufferState->fbo(), sceneRoot);
  deferredShading->setAmbientLight(Vec3f(0.2f));

#ifdef USE_POINT_LIGHT
  ref_ptr<PointLight> pointLight = createPointLight(app.get());
  ref_ptr<PointShadowMap> pointShadow = createPointShadow(app.get(), pointLight, cam);
  pointShadow->addCaster(gBufferNode);
  deferredShading->addLight(pointLight, pointShadow);
#endif
#ifdef USE_SPOT_LIGHT
  ref_ptr<SpotLight> spotLight = createSpotLight(app.get());
  ref_ptr<SpotShadowMap> spotShadow = createSpotShadow(app.get(), spotLight, cam);
  spotShadow->addCaster(gBufferNode);
  deferredShading->addLight(spotLight, spotShadow);
#endif

#ifdef USE_SKY
  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);
  // add a sky box
  ref_ptr<DynamicSky> sky = createSky(app.get(), backgroundNode);
  ref_ptr<DirectionalShadowMap> sunShadow = createSunShadow(sky, cam, frustum);
  sunShadow->addCaster(gBufferNode);
  deferredShading->addLight(sky->sun(), sunShadow);
#endif

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->rootNode()->addChild(guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), gBufferState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}

#ifdef USE_RAIN
  {
    const GLuint numRainDrops = 5000;

    ref_ptr<RainParticles> particles =
        ref_ptr<RainParticles>::manage(new RainParticles(numRainDrops));
    particles->set_depthTexture(renderTree->sceneDepthTexture());
    //particles->loadIntensityTextureArray(
    //    "res/textures/rainTextures", "cv[0-9]+_vPositive_[0-9]+\\.dds");
    //particles->loadIntensityTexture("res/textures/rainTextures/cv0_vPositive_0000.dds");
    particles->loadIntensityTexture("res/textures/flare.jpg");
    particles->createBuffer();

    modelMat = ref_ptr<ModelTransformation>();
    ref_ptr<Material> material = ref_ptr<Material>();

    ref_ptr<StateNode> node = renderTree->addMesh(
        renderTree->backgroundPass(),
        ref_ptr<State>::cast(particles),
        modelMat, material, "");
    ShaderConfig shaderCfg = ShaderConfigurer::configure(node.get());

    particles->createShader(shaderCfg);
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(particles));

    application->addShaderInput(particles->gravity(), -100.0f, 100.0f, 1);
    application->addShaderInput(particles->dampingFactor(), 0.0f, 10.0f, 3);
    application->addShaderInput(particles->noiseFactor(), 0.0f, 10.0f, 3);
    application->addShaderInput(particles->cloudPosition(), -10.0f, 10.0f, 2);
    application->addShaderInput(particles->cloudRadius(), 0.1f, 100.0f, 2);
    application->addShaderInput(particles->particleMass(), 0.0f, 10.0f, 3);
    application->addShaderInput(particles->particleSize(), 0.0f, 10.0f, 3);
    application->addShaderInput(particles->streakSize(), 0.0f, 10.0f, 4);
    application->addShaderInput(particles->brightness(), 0.0f, 1.0f, 3);
    application->addShaderInput(particles->softScale(), 0.0f, 100.0f, 2);
  }
#endif
#if 0
  {
//#define USE_DENSITY_PARTICLES
    ref_ptr<FogState> fog = ref_ptr<FogState>::manage(new FogState());
    fog->set_useSkyScattering(GL_TRUE);
    fog->set_sceneFramebuffer(renderTree->sceneFBO());
    fog->set_colorTexture(renderTree->sceneTexture());
    fog->set_depthTexture(renderTree->sceneDepthTexture());

#ifdef USE_DENSITY_PARTICLES
    GLuint numParticles = 100;
    fog->setDensityParticles(numParticles);
#endif
#ifdef USE_SUN_LIGHT
    fog->addLight(sunLight);
#endif
#ifdef USE_SPOT_LIGHT
    fog->addLight(spotLight);
#endif
#ifdef USE_POINT_LIGHT
    fog->addLight(pointLight);
#endif
    fog->createBuffer(Vec2ui(512));

    ref_ptr<StateNode> node = renderTree->addMesh(
        renderTree->backgroundPass(),
        ref_ptr<State>::cast(fog),
        ref_ptr<ModelTransformation>(),
        ref_ptr<Material>(),
        "");

    application->addShaderInput(fog->fogStart(), 0.0f, 999.0f, 1);
    application->addShaderInput(fog->fogEnd(), 0.0f, 999.0f, 1);
    application->addShaderInput(fog->fogExposure(), 0.0f, 9.99f, 2);
    application->addShaderInput(fog->fogScale(), 0.0f, 1.0f, 4);
    application->addShaderInput(fog->constFogColor(), 0.0f, 1.0f, 4);
    application->addShaderInput(fog->constFogDensity(), 0.0f, 1.0f, 4);
#ifdef USE_SUN_LIGHT
    application->addShaderInput(fog->sunDiffuseExposure(), 0.0f, 9.99f, 2);
    application->addShaderInput(fog->sunScatteringExposure(), 0.0f, 9.99f, 2);
    application->addShaderInput(fog->sunScatteringDecay(), 0.0f, 1.0f, 3);
    application->addShaderInput(fog->sunScatteringWeight(), 0.0f, 1.0f, 3);
    application->addShaderInput(fog->sunScatteringDensity(), 0.0f, 9.99f, 3);
    application->addShaderInput(fog->sunScatteringSamples(), 0.0f, 100.0f, 0);
#endif
#if defined(USE_POINT_LIGHT) || defined(USE_SPOT_LIGHT)
    application->addShaderInput(fog->lightDiffuseExposure(), 0.0f, 9.99f, 2);
#endif
#ifdef USE_DENSITY_PARTICLES
    application->addShaderInput(fog->densityScale(), 0.0f, 10.0f, 3);
    application->addShaderInput(fog->intensityBias(), 0.0f, 10.0f, 3);
    application->addShaderInput(fog->emitVelocity(), 0.0f, 10.0f, 3);
    application->addShaderInput(fog->emitterRadius(), 0.0f, 100.0f, 2);
    application->addShaderInput(fog->emitRadius(), 0.0f, 9.0f, 2);
    application->addShaderInput(fog->emitDensity(), 0.0f, 10.0f, 3);
    application->addShaderInput(fog->emitLifetime(), 10.0f, 1000.0f, 1);
#endif

    ShaderConfig shaderCfg = ShaderConfigurer::configure(node.get());
    fog->createShaders(shaderCfg);
  }
#endif

