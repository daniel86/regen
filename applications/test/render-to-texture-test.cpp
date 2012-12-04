
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/shadows/directional-shadow-map.h>
#include <ogle/shadows/spot-shadow-map.h>
#include <ogle/shadows/point-shadow-map.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/textures/image-texture.h>

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

static void updateSpotShadow_(void *data) {
  SpotShadowMap *sm = (SpotShadowMap*)data;
  sm->updateLight();
}

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
  application->set_windowTitle("RenderToTexture");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  camManipulator->setStepLength(0.0f,0.0f);
  camManipulator->set_height(3.0f,0.0f);
  camManipulator->set_degree(1.85*M_PI,0.0f);
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<PerspectiveCamera> &sceneCamera = renderTree->perspectiveCamera();
  ref_ptr<Frustum> sceneFrustum = ref_ptr<Frustum>::manage(new Frustum);
  sceneFrustum->setProjection(
      sceneCamera->fovUniform()->getVertex1f(0),
      sceneCamera->aspect(),
      sceneCamera->nearUniform()->getVertex1f(0),
      sceneCamera->farUniform()->getVertex1f(0));

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
  application->addShaderInput(spotLight->position(), -100.0f, 100.0f, 2);
  application->addShaderInput(spotLight->attenuation(), 0.0f, 1.0f, 3);
  application->addShaderInput(spotLight->coneAngle(), 0.0f, 1.0f, 5);
  renderTree->setLight(ref_ptr<Light>::cast(spotLight));
#endif
#ifdef USE_SPOT_SHADOW
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
  spotShadow->set_filteringMode(spotShadowFilter);
#endif

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      TRANSPARENCY_AVERAGE_SUM,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.7f,0.6f,0.5f,1.0f)
  );

  ref_ptr<ModelTransformationState> modelMat;

  ref_ptr<MeshState> mesh;
  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 1.0f, 0.1f);
    mesh = ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.5f, 1.0f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_pewter();
    material->set_alpha(0.25f);
    material->set_useAlpha(GL_TRUE);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);

    renderTree->addMesh(mesh, modelMat, material, "mesh.transparent", GL_TRUE);
  }
  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 1.0f, 0.1f);
    mesh = ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.5f, -0.25f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_ruby();

    renderTree->addMesh(mesh, modelMat,  material);
  }
  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 1.0f, 0.1f);
    mesh = ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.15f, 0.4f, -1.5f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_jade();
    material->set_alpha(0.75f);
    material->set_useAlpha(GL_TRUE);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);

    renderTree->addMesh(mesh, modelMat, material, "mesh.transparent", GL_TRUE);
  }
  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 1.0f, 0.1f);
    mesh = ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.3f, -2.75f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_gold();
    material->set_alpha(0.5f);
    material->set_useAlpha(GL_TRUE);
    application->addShaderInput(material->alpha(), 0.0f, 1.0f, 2);

    renderTree->addMesh(mesh, modelMat, material, "mesh.transparent", GL_TRUE);
  }
  {
    UnitQuad::Config quadConfig;
    quadConfig.levelOfDetail = 0;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.0*M_PI, 0.0*M_PI, 1.0*M_PI);
    quadConfig.posScale = Vec3f(10.0f, 10.0f, 10.0f);
    quadConfig.texcoScale = Vec2f(2.0f, 2.0f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, -0.5f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_chrome();
    material->set_specular(Vec3f(0.0f));
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

#ifdef USE_SPOT_SHADOW
  spotShadow->addCaster(renderTree->perspectivePass());
  // TODO: transparent mesh shadows.
  //    * need alpha intensity value
  //    * or opacity weighted color
  spotShadow->addCaster(renderTree->transparencyPass());
#endif

  renderTree->setShowFPS();

  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
