
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
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
  // we do not change any light properties so the uniforms
  // can be transformed to constants.
  light->setConstantUniforms(GL_TRUE);

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
    modelMat->translate(Vec3f(0.0f, 0.5f, 2.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    material = ref_ptr<Material>::manage(new Material);
    material->set_gold();
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(mesh, modelMat, material);
  }

  renderTree->setShowFPS();

  return application->mainLoop();
}
