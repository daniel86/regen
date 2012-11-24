
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
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

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

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

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  ref_ptr<DirectionalLight> dirLight =
      ref_ptr<DirectionalLight>::manage(new DirectionalLight);
  dirLight->set_direction(Vec3f(1.0f,1.0f,1.0f));
  dirLight->set_ambient(Vec3f(0.15f));
  dirLight->set_diffuse(Vec3f(0.35f));
  application->addShaderInput(dirLight->direction(), -1.0f, 1.0f, 2);
  application->addShaderInput(dirLight->ambient(), 0.0f, 1.0f, 2);
  application->addShaderInput(dirLight->diffuse(), 0.0f, 1.0f, 2);
  application->addShaderInput(dirLight->specular(), 0.0f, 1.0f, 2);
  renderTree->setLight(ref_ptr<Light>::cast(dirLight));

  ref_ptr<PointLight> pointLight =
      ref_ptr<PointLight>::manage(new PointLight);
  pointLight->set_position(Vec3f(-1.5f,0.8f,-1.5f));
  pointLight->set_diffuse(Vec3f(0.1f, 0.35f, 0.15f));
  pointLight->set_constantAttenuation(0.02f);
  pointLight->set_linearAttenuation(0.05f);
  pointLight->set_quadricAttenuation(0.07f);
  application->addShaderInput(pointLight->position(), -100.0f, 100.0f, 2);
  application->addShaderInput(pointLight->ambient(), 0.0f, 1.0f, 2);
  application->addShaderInput(pointLight->diffuse(), 0.0f, 1.0f, 2);
  application->addShaderInput(pointLight->specular(), 0.0f, 1.0f, 2);
  application->addShaderInput(pointLight->attenuation(), 0.0f, 1.0f, 3);
  renderTree->setLight(ref_ptr<Light>::cast(pointLight));

  ref_ptr<SpotLight> spotLight =
      ref_ptr<SpotLight>::manage(new SpotLight);
  spotLight->set_position(Vec3f(3.7f,2.22f,-3.7f));
  spotLight->set_spotDirection(Vec3f(-1.0f,-0.7f,0.76f));
  spotLight->set_diffuse(Vec3f(0.91f,0.51f,0.8f));
  spotLight->set_constantAttenuation(0.022f);
  spotLight->set_linearAttenuation(0.011f);
  spotLight->set_quadricAttenuation(0.026f);
  application->addShaderInput(spotLight->position(), -100.0f, 100.0f, 2);
  application->addShaderInput(spotLight->ambient(), 0.0f, 1.0f, 2);
  application->addShaderInput(spotLight->diffuse(), 0.0f, 1.0f, 2);
  application->addShaderInput(spotLight->specular(), 0.0f, 1.0f, 2);
  application->addShaderInput(spotLight->spotDirection(), -1.0f, 1.0f, 2);
  application->addShaderInput(spotLight->attenuation(), 0.0f, 1.0f, 3);
  application->addShaderInput(spotLight->coneAngle(), 0.0f, 360.0f, 2);
  renderTree->setLight(ref_ptr<Light>::cast(spotLight));

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
    material->set_twoSided(GL_TRUE);
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  renderTree->addSkyBox("res/textures/cube-stormydays.jpg");
  renderTree->setShowFPS();

  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
