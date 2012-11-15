
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/animations/animation-manager.h>

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
  application->set_windowTitle("Hello World");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  renderTree->setClearScreenColor(Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f));
  renderTree->globalStates()->state()->addEnabler(
      ref_ptr<Callable>::manage(new ClearDepthState));

  ref_ptr<Light> &light = renderTree->setLight();
  application->addShaderInput(light->position(), -100.0f, 100.0f, 2);
  application->addShaderInput(light->ambient(), 0.0f, 1.0f, 2);
  application->addShaderInput(light->diffuse(), 0.0f, 1.0f, 2);
  application->addShaderInput(light->specular(), 0.0f, 1.0f, 2);
  application->addShaderInput(light->spotDirection(), -100.0f, 100.0f, 2);
  application->addShaderInput(light->spotExponent(), -100.0f, 100.0f, 2);
  application->addShaderInput(light->constantAttenuation(), 0.0f, 1.0f, 3);
  application->addShaderInput(light->linearAttenuation(), 0.0f, 1.0f, 3);
  application->addShaderInput(light->quadricAttenuation(), 0.0f, 1.0f, 3);
  application->addShaderInput(light->innerConeAngle(), 0.0f, 360.0f, 2);
  application->addShaderInput(light->outerConeAngle(), 0.0f, 360.0f, 2);

  camManipulator->setStepLength(0.0f,0.0f);
  camManipulator->set_degree(0.0f,0.0f);

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;

    ref_ptr<MeshState> mesh =
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 2.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    material = ref_ptr<Material>::manage(new Material);
    material->set_gold();
    material->setConstantUniforms(GL_TRUE);
    application->addShaderInput(material->ambient(), 0.0f, 1.0f, 2);
    application->addShaderInput(material->diffuse(), 0.0f, 1.0f, 2);
    application->addShaderInput(material->specular(), 0.0f, 1.0f, 2);
    application->addShaderInput(material->emission(), 0.0f, 1.0f, 2);
    application->addShaderInput(material->shininess(), 0.0f, 200.0f, 2);
    application->addShaderInput(material->shininessStrength(), 0.0f, 200.0f, 2);
    application->addShaderInput(material->darkness(), 0.0f, 1.0f, 2);
    application->addShaderInput(material->roughness(), 0.0f, 1.0f, 2);

    renderTree->addMesh(mesh, modelMat, material);
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
    modelMat->translate(Vec3f(0.0f, -2.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::PHONG_SHADING );
    material->set_chrome();
    material->set_twoSided(GL_TRUE);
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

  renderTree->setShowFPS();

  return application->mainLoop();
}
