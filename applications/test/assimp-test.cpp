
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/shadows/directional-shadow-map.h>
#include <ogle/shadows/spot-shadow-map.h>
#include <ogle/shadows/point-shadow-map.h>
#include <ogle/states/assimp-importer.h>
#include <ogle/states/mesh-state.h>
#include <ogle/textures/image-texture.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/string-util.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

#define USE_SUN_LIGHT
#ifdef USE_SUN_LIGHT
  #define USE_SUN_SHADOW
#endif
#define USE_SPOT_LIGHT
#ifdef USE_SPOT_LIGHT
  #define USE_SPOT_SHADOW
#endif
#define USE_POINT_LIGHT
#ifdef USE_POINT_LIGHT
  #define USE_POINT_SHADOW
#endif

const string transferTBNNormal =
"void transferTBNNormal(inout vec4 texel) {\n"
"#if SHADER_STAGE==fs\n"
"    vec3 T = in_tangent;\n"
"    vec3 N = (gl_FrontFacing ? in_norWorld : -in_norWorld);\n"
"    vec3 B = in_binormal;\n"
"    mat3 tbn = mat3(T,B,N);"
"    texel.xyz = normalize( tbn * texel.xyz );\n"
"#endif\n"
"}";

class AnimStoppedHandler : public EventCallable
{
public:
  map< string, Vec2d > animationRanges_;
  AnimStoppedHandler(const map< string, Vec2d > &animationRanges)
  : EventCallable(),
    animationRanges_(animationRanges)
  {
  }
  void call(EventObject *ev, void *data)
  {
    NodeAnimation *anim = (NodeAnimation*)ev;

    GLint i = rand()%animationRanges_.size();
    GLint index = 0;
    for(map< string, Vec2d >::iterator
        it=animationRanges_.begin(); it!=animationRanges_.end(); ++it)
    {
      if(index==i) {
        anim->setAnimationIndexActive(0,
            it->second + Vec2d(-1.0, -1.0) );
        break;
      }
      ++index;
    }
  }
};

static void updateSunShadow_(void *data) {
  DirectionalShadowMap *sm = (DirectionalShadowMap*)data;
  sm->updateLightDirection();
}
static void updateSpotShadow_(void *data) {
  SpotShadowMap *sm = (SpotShadowMap*)data;
  sm->updateLight();
}
static void updatePointShadow_(void *data) {
  PointShadowMap *sm = (PointShadowMap*)data;
  sm->updateLight();
}

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;
  const GLuint shadowMapSize = 2048;
  const GLenum internalFormat = GL_DEPTH_COMPONENT24;
  const GLenum pixelType = GL_FLOAT;
  const GLfloat shadowSplitWeight = 0.75;

  DirectionalShadowMap::set_numSplits(3);

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("Assimp Model and Bones");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));
  camManipulator->setStepLength(0.0f,0.0f);
  camManipulator->set_radius(9.0f, 0.0f);

  ref_ptr<PerspectiveCamera> &sceneCamera = renderTree->perspectiveCamera();
  ref_ptr<Frustum> sceneFrustum = ref_ptr<Frustum>::manage(new Frustum);
  sceneFrustum->setProjection(
      sceneCamera->fovUniform()->getVertex1f(0),
      sceneCamera->aspect(),
      sceneCamera->nearUniform()->getVertex1f(0),
      sceneCamera->farUniform()->getVertex1f(0));

  // TODO: correct value
  GLuint maxNumBones = 39;

#ifdef USE_SUN_LIGHT
  ref_ptr<DirectionalLight> sunLight =
      ref_ptr<DirectionalLight>::manage(new DirectionalLight);
  sunLight->set_direction(Vec3f(1.0f,1.0f,1.0f));
  sunLight->set_ambient(Vec3f(0.15f));
  sunLight->set_diffuse(Vec3f(0.35f));
  application->addShaderInput(sunLight->ambient(), 0.0f, 1.0f, 2);
  application->addShaderInput(sunLight->diffuse(), 0.0f, 1.0f, 2);
  application->addShaderInput(sunLight->specular(), 0.0f, 1.0f, 2);
  application->addShaderInput(sunLight->direction(), -1.0f, 1.0f, 2);
  renderTree->setLight(ref_ptr<Light>::cast(sunLight));
#endif
#ifdef USE_SUN_SHADOW
  // add shadow maps to the sun light
  ref_ptr<DirectionalShadowMap> sunShadow = ref_ptr<DirectionalShadowMap>::manage(
      new DirectionalShadowMap(sunLight, sceneFrustum, sceneCamera,
          shadowMapSize, maxNumBones, shadowSplitWeight, internalFormat, pixelType));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(sunShadow));
  application->addValueChangedHandler(
      sunLight->direction()->name(), updateSunShadow_, sunShadow.get());
#endif

#ifdef USE_POINT_LIGHT
  ref_ptr<PointLight> pointLight =
      ref_ptr<PointLight>::manage(new PointLight);
  pointLight->set_position(Vec3f(0.0f, 5.0f, 4.0f));
  pointLight->set_diffuse(Vec3f(0.1f, 0.7f, 0.15f));
  pointLight->set_ambient(Vec3f(0.0f));
  pointLight->set_constantAttenuation(0.0f);
  pointLight->set_linearAttenuation(0.0f);
  pointLight->set_quadricAttenuation(0.02f);
  application->addShaderInput(pointLight->position(), -100.0f, 100.0f, 2);
  application->addShaderInput(pointLight->ambient(), 0.0f, 1.0f, 2);
  application->addShaderInput(pointLight->diffuse(), 0.0f, 1.0f, 2);
  application->addShaderInput(pointLight->specular(), 0.0f, 1.0f, 2);
  application->addShaderInput(pointLight->attenuation(), 0.0f, 1.0f, 3);
  renderTree->setLight(ref_ptr<Light>::cast(pointLight));
#endif
#ifdef USE_POINT_SHADOW
  // add shadow maps to the sun light
  ref_ptr<PointShadowMap> pointShadow = ref_ptr<PointShadowMap>::manage(
      new PointShadowMap(pointLight, sceneCamera,
          shadowMapSize, maxNumBones, internalFormat, pixelType));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(pointShadow));
  application->addValueChangedHandler(
      pointLight->position()->name(), updatePointShadow_, pointShadow.get());;
#endif

#ifdef USE_SPOT_LIGHT
  ref_ptr<SpotLight> spotLight =
      ref_ptr<SpotLight>::manage(new SpotLight);
  spotLight->set_position(Vec3f(-8.0f,4.0f,8.0f));
  spotLight->set_spotDirection(Vec3f(1.0f,-1.0f,-1.0f));
  spotLight->set_ambient(Vec3f(0.0f));
  spotLight->set_diffuse(Vec3f(0.1f,0.36f,0.36f));
  spotLight->set_innerConeAngle(35.0f);
  spotLight->set_outerConeAngle(30.0f);
  spotLight->set_constantAttenuation(0.0022f);
  spotLight->set_linearAttenuation(0.0011f);
  spotLight->set_quadricAttenuation(0.0026f);
  application->addShaderInput(spotLight->position(), -100.0f, 100.0f, 2);
  application->addShaderInput(spotLight->ambient(), 0.0f, 1.0f, 2);
  application->addShaderInput(spotLight->diffuse(), 0.0f, 1.0f, 2);
  application->addShaderInput(spotLight->specular(), 0.0f, 1.0f, 2);
  application->addShaderInput(spotLight->spotDirection(), -1.0f, 1.0f, 2);
  application->addShaderInput(spotLight->attenuation(), 0.0f, 1.0f, 3);
  application->addShaderInput(spotLight->coneAngle(), 0.0f, 1.0f, 5);
  renderTree->setLight(ref_ptr<Light>::cast(spotLight));
#endif
#ifdef USE_SPOT_SHADOW
  // add shadow maps to the sun light
  ref_ptr<SpotShadowMap> spotShadow = ref_ptr<SpotShadowMap>::manage(
      new SpotShadowMap(spotLight, sceneCamera,
          shadowMapSize, internalFormat, pixelType));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(spotShadow));
  application->addValueChangedHandler(
      spotLight->position()->name(), updateSpotShadow_, spotShadow.get());
  application->addValueChangedHandler(
      spotLight->spotDirection()->name(), updateSpotShadow_, spotShadow.get());
  application->addValueChangedHandler(
      spotLight->coneAngle()->name(), updateSpotShadow_, spotShadow.get());
#endif

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.7f,0.6f,0.5f,1.0f)
  );

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  {
    string modelPath = "res/models/psionic/dwarf/x";
    string modelName = "dwarf2.x";

    AssimpImporter importer(
        modelPath + "/" + modelName,
        modelPath);

    list< ref_ptr<MeshState> > meshes = importer.loadMeshes();

    Mat4f transformation = xyzRotationMatrix(0.0f, M_PI, 0.0f);
    for(list< ref_ptr<MeshState> >::iterator
        it=meshes.begin(); it!=meshes.end(); ++it)
    {
      ref_ptr<MeshState> mesh = *it;

      material = importer.getMeshMaterial(mesh.get());
      material->setConstantUniforms(GL_TRUE);

      modelMat = ref_ptr<ModelTransformationState>::manage(
          new ModelTransformationState);
      modelMat->set_modelMat(transformation, 0.0f);
      modelMat->translate(Vec3f(0.0f, -2.0f, 0.0f), 0.0f);
      modelMat->setConstantUniforms(GL_TRUE);

      ref_ptr<BonesState> bonesState =
          importer.loadMeshBones(mesh.get());
      if(bonesState.get()==NULL) {
        WARN_LOG("No bones state!");
      } else {
        modelMat->joinStates(ref_ptr<State>::cast(bonesState));
      }

      ref_ptr<StateNode> meshNode = renderTree->addMesh(mesh, modelMat, material);

    }

    // mapping from different types of animations
    // to matching ticks
    map< string, Vec2d > animationRanges;
    animationRanges["none"] = Vec2d( -1.0, -1.0 );
    animationRanges["complete"] = Vec2d( 0.0, 361.0 );
    animationRanges["run"] = Vec2d( 16.0, 26.0 );
    animationRanges["jump"] = Vec2d( 28.0, 40.0 );
    animationRanges["jumpSpot"] = Vec2d( 42.0, 54.0 );
    animationRanges["crouch"] = Vec2d( 56.0, 59.0 );
    animationRanges["crouchLoop"] = Vec2d( 60.0, 69.0 );
    animationRanges["getUp"] = Vec2d( 70.0, 74.0 );
    animationRanges["battleIdle1"] = Vec2d( 75.0, 88.0 );
    animationRanges["battleIdle2"] = Vec2d( 90.0, 110.0 );
    animationRanges["attack1"] = Vec2d( 112.0, 126.0 );
    animationRanges["attack2"] = Vec2d( 128.0, 142.0 );
    animationRanges["attack3"] = Vec2d( 144.0, 160.0 );
    animationRanges["attack4"] = Vec2d( 162.0, 180.0 );
    animationRanges["attack5"] = Vec2d( 182.0, 192.0 );
    animationRanges["block"] = Vec2d( 194.0, 210.0 );
    animationRanges["dieFwd"] = Vec2d( 212.0, 227.0 );
    animationRanges["dieBack"] = Vec2d( 230.0, 251.0 );
    animationRanges["yes"] = Vec2d( 253.0, 272.0 );
    animationRanges["no"] = Vec2d( 274.0, 290.0 );
    animationRanges["idle1"] = Vec2d( 292.0, 325.0 );
    animationRanges["idle2"] = Vec2d( 327.0, 360.0 );

    bool forceChannelStates=true;
    AnimationBehaviour forcedPostState = ANIM_BEHAVIOR_LINEAR;
    AnimationBehaviour forcedPreState = ANIM_BEHAVIOR_LINEAR;
    double defaultTicksPerSecond=20.0;
    ref_ptr<NodeAnimation> boneAnim = importer.loadNodeAnimation(
        forceChannelStates,
        forcedPostState,
        forcedPreState,
        defaultTicksPerSecond);
    boneAnim->set_timeFactor(1.0);

    ref_ptr<EventCallable> animStopped = ref_ptr<EventCallable>::manage(
        new AnimStoppedHandler(animationRanges) );
    boneAnim->connect( NodeAnimation::ANIMATION_STOPPED, animStopped );

    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(boneAnim));

    animStopped->call(boneAnim.get(), NULL);
  }
  {
    UnitSphere::Config sphereConfig;
    sphereConfig.posScale = Vec3f(3.0f);
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;

    ref_ptr<MeshState> mesh =
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(-4.0f, 0.0f, -3.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    material = ref_ptr<Material>::manage(new Material);
    material->set_gold();
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(mesh, modelMat, material);
  }
  {
    UnitQuad::Config quadConfig;
    quadConfig.levelOfDetail = 2;
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_TRUE;
    // XXX: something wrong when using tangents here...
    //quadConfig.isTangentRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.0*M_PI, 0.0*M_PI, 1.0*M_PI);
    quadConfig.posScale = Vec3f(20.0f);
    quadConfig.texcoScale = Vec2f(5.0f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, -2.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    material = ref_ptr<Material>::manage(new Material);
    material->set_ambient(Vec3f(0.3f));
    material->set_diffuse(Vec3f(0.7f));
    material->setConstantUniforms(GL_TRUE);

    ref_ptr<Texture> colMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/brick/color.jpg"));
    ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->setMapTo(MAP_TO_COLOR);
    texState->set_blendMode(BLEND_MODE_SRC);
    material->addTexture(texState);

    /*
    ref_ptr<Texture> norMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/brick/normal.jpg"));
    texState = ref_ptr<TextureState>::manage(new TextureState(norMap_));
    texState->set_name("normalTexture");
    texState->setMapTo(MAP_TO_NORMAL);
    texState->set_blendMode(BLEND_MODE_SRC);
    texState->set_transferFunction(transferTBNNormal, "transferTBNNormal");
    material->addTexture(texState);
    */

    renderTree->addMesh(quad, modelMat, material);
  }

#ifdef USE_SUN_SHADOW
  sunShadow->addCaster(renderTree->perspectivePass());
#endif
#ifdef USE_SPOT_SHADOW
  spotShadow->addCaster(renderTree->perspectivePass());
#endif
#ifdef USE_POINT_SHADOW
  pointShadow->addCaster(renderTree->perspectivePass());
#endif

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
