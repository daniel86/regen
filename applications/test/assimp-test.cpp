
#include "factory.h"
using namespace ogle;

#define USE_SPOT_LIGHT
#define USE_POINT_LIGHT
#define USE_SKY
//#define USE_LIGHT_SHAFTS
#define USE_VOLUME_FOG
#define USE_RAIN
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

  // create a root node for everything that needs camera as input
  ref_ptr<PerspectiveCamera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 1.2f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 20.0f );
  manipulator->set_degree( 0.0f );
  manipulator->setStepLength( M_PI*0.0 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  ref_ptr<Frustum> frustum = ref_ptr<Frustum>::manage(new Frustum);
  frustum->setProjection(cam->fov(), cam->aspect(), cam->near(), cam->far());

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get());
  ref_ptr<StateNode> gBufferParent = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gBufferState)));
  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage( new StateNode);
  sceneRoot->addChild(gBufferParent);
  gBufferParent->addChild(gBufferNode);

  ref_ptr<Texture> gDiffuseTexture = gBufferState->fbo()->colorBuffer()[0];
  ref_ptr<Texture> gDepthTexture = gBufferState->fbo()->depthTexture();
  sceneRoot->addChild(gBufferNode);
  createAssimpMesh(
        app.get(), gBufferNode
      , assimpMeshFile
      , assimpMeshTexturesPath
      , Mat4f::rotationMatrix(0.0f,M_PI,0.0f)
      , Vec3f(0.0f,-2.0f,0.0f)
      , animRanges, sizeof(animRanges)/sizeof(BoneAnimRange)
  );
  createFloorMesh(app.get(), gBufferNode,
      -2.0, Vec3f(20.0f), Vec2f(4.0f), TRANSFER_TEXCO_RELIEF);

  const GLboolean useAmbientLight = GL_TRUE;
  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gBufferState->fbo(), sceneRoot, ShadowMap::FILTERING_NONE, useAmbientLight);
  deferredShading->ambientState()->ambientLight()->setVertex3f(0,Vec3f(0.2f));
  deferredShading->dirShadowState()->set_numShadowLayer(3);

#ifdef USE_POINT_LIGHT
  ref_ptr<PointLight> pointLight = createPointLight(app.get());
  pointLight->set_position(Vec3f(-5.0f,7.0f,0.0f));
  pointLight->set_diffuse(Vec3f(0.2f,0.1f,0.6f));
  pointLight->set_innerRadius(10.0);
  pointLight->set_outerRadius(20.0);
  ref_ptr<PointShadowMap> pointShadow = createPointShadow(app.get(), pointLight, cam, 512);
  pointShadow->addCaster(gBufferNode);
  deferredShading->addLight(
      ref_ptr<Light>::cast(pointLight),
      ref_ptr<ShadowMap>::cast(pointShadow));
#endif
#ifdef USE_SPOT_LIGHT
  ref_ptr<SpotLight> spotLight = createSpotLight(app.get());
  spotLight->set_position(Vec3f(3.0f,8.0f,4.0f));
  spotLight->set_spotDirection(Vec3f(-0.37f,-0.95f,-0.46f));
  spotLight->set_diffuse(Vec3f(0.4f,0.3f,0.4f));
  spotLight->set_innerRadius(10.0);
  spotLight->set_outerRadius(21.0);
  spotLight->coneAngle()->setVertex2f(0, Vec2f(0.98, 0.9));
  ref_ptr<SpotShadowMap> spotShadow = createSpotShadow(app.get(), spotLight, cam, 512);
  spotShadow->addCaster(gBufferNode);
  deferredShading->addLight(
      ref_ptr<Light>::cast(spotLight),
      ref_ptr<ShadowMap>::cast(spotShadow));
#endif

  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);
#ifdef USE_SKY
  // add a sky box
  ref_ptr<DynamicSky> sky = createSky(app.get(), backgroundNode);
  ref_ptr<DirectionalShadowMap> sunShadow = createSunShadow(sky, cam, frustum, 1024, 3);
  sunShadow->addCaster(gBufferNode);
  deferredShading->addLight(
      ref_ptr<Light>::cast(sky->sun()),
      ref_ptr<ShadowMap>::cast(sunShadow));
#endif

  ref_ptr<StateNode> postPassNode = createPostPassNode(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(postPassNode);

#ifdef USE_VOLUME_FOG
  ref_ptr<VolumetricFog> volumeFog = createVolumeFog(app.get(), gDepthTexture, postPassNode);
#ifdef USE_SPOT_LIGHT
  ref_ptr<ShaderInput1f> spotExposure =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogExposure"));
  ref_ptr<ShaderInput2f> spotRadiusScale =
      ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogRadiusScale"));
  ref_ptr<ShaderInput2f> spotConeScale =
      ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogConeScale"));
  spotExposure->setUniformData(1.0);
  spotRadiusScale->setUniformData(Vec2f(1.0));
  spotConeScale->setUniformData(Vec2f(1.0));
  app->addShaderInput(spotExposure, 0.0, 10.0, 2);
  app->addShaderInput(spotRadiusScale, 0.0, 10.0, 2);
  app->addShaderInput(spotConeScale, 0.0, 10.0, 2);

  volumeFog->addLight(spotLight,
      spotExposure, spotRadiusScale, spotConeScale);
#endif
#ifdef USE_POINT_LIGHT
  ref_ptr<ShaderInput1f> pointExposure =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogExposure"));
  ref_ptr<ShaderInput2f> pointRadiusScale =
      ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogRadiusScale"));
  pointExposure->setUniformData(5.0);
  pointRadiusScale->setUniformData(Vec2f(0.0,0.2));
  app->addShaderInput(pointExposure, 0.0, 10.0, 2);
  app->addShaderInput(pointRadiusScale, 0.0, 10.0, 2);

  volumeFog->addLight(pointLight, pointExposure, pointRadiusScale);
#endif
#endif

  ref_ptr<DirectShading> directShading =
      ref_ptr<DirectShading>::manage(new DirectShading);
#ifdef USE_SPOT_LIGHT
  directShading->addLight(ref_ptr<Light>::cast(spotLight));
#endif
#ifdef USE_POINT_LIGHT
  directShading->addLight(ref_ptr<Light>::cast(pointLight));
#endif
#ifdef USE_SKY
  directShading->addLight(ref_ptr<Light>::cast(sky->sun()));
#endif
  ref_ptr<StateNode> directShadingNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(directShading)));
  postPassNode->addChild(directShadingNode);
#ifdef USE_RAIN
  ref_ptr<RainParticles> rain = createRain(
      app.get(), gDepthTexture, directShadingNode, 5000);
  rain->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
      gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_TRUE)));
#endif

#ifdef USE_LIGHT_SHAFTS
  ref_ptr<SkyLightShaft> sunRay = createSkyLightShaft(
      app.get(), sky->sun(), gDiffuseTexture, gDepthTexture, postPassNode);
  sunRay->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
      gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));
#endif

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), gBufferState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
