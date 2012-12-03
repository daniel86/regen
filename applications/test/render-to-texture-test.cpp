
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

#define USE_SUN_LIGHT
#ifdef USE_SUN_LIGHT
  #define USE_SUN_SHADOW
#endif

static void updateSunShadow_(void *data) {
  DirectionalShadowMap *sm = (DirectionalShadowMap*)data;
  sm->updateLightDirection();
}

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;
  const GLuint shadowMapSize = 512;
  const GLenum internalFormat = GL_DEPTH_COMPONENT16;
  const GLenum pixelType = GL_BYTE;
  const GLfloat shadowSplitWeight = 0.75;
  ShadowMap::FilterMode sunShadowFilter = ShadowMap::PCF_GAUSSIAN;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("RenderToTexture");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<PerspectiveCamera> &sceneCamera = renderTree->perspectiveCamera();
  ref_ptr<Frustum> sceneFrustum = ref_ptr<Frustum>::manage(new Frustum);
  sceneFrustum->setProjection(
      sceneCamera->fovUniform()->getVertex1f(0),
      sceneCamera->aspect(),
      sceneCamera->nearUniform()->getVertex1f(0),
      sceneCamera->farUniform()->getVertex1f(0));

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
          shadowMapSize, shadowSplitWeight, internalFormat, pixelType));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(sunShadow));
  application->addValueChangedHandler(
      sunLight->direction()->name(), updateSunShadow_, sunShadow.get());
  sunShadow->set_filteringMode(sunShadowFilter);
#endif

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      TRANSPARENCY_NONE,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.0f)
  );

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 0.5f, 0.5f);

    ref_ptr<MeshState> mesh =
        ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(-2.0f, 0.75f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(mesh, modelMat);
  }
  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;

    ref_ptr<MeshState> sphere = ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.5f, 0.0f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_ruby();
    application->addShaderInput(material->ambient(), 0.0f, 1.0f, 2);
    application->addShaderInput(material->diffuse(), 0.0f, 1.0f, 2);
    application->addShaderInput(material->specular(), 0.0f, 1.0f, 2);
    application->addShaderInput(material->shininess(), 0.0f, 200.0f, 2);

    renderTree->addMesh(sphere, modelMat, material);
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

  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
