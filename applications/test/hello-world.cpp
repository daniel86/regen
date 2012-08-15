
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Hello World!");

  application->setClearScreenColor(Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f));
  application->globalStates()->state()->addEnabler(
      ref_ptr<Callable>::manage(new ClearDepthState));

  application->setLight();
  application->camManipulator()->setStepLength(0.0f,0.0f);
  application->camManipulator()->set_degree(0.0f,0.0f);

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 2.0f, 1.0f);
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    material = ref_ptr<Material>::manage(new Material);
    material->set_pewter();
    application->addMesh(
        ref_ptr<MeshState>::manage(new UnitCube(cubeConfig)),
        modelMat, material);
  }
  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.5f, 2.0f), 0.0f);

    material = ref_ptr<Material>::manage(new Material);
    material->set_chrome();
    application->addMesh(
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig)),
        modelMat, material);
  }
  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 0.5f, 0.5f);
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(-2.0f, 0.75f, 0.0f), 0.0f);

    application->addMesh(
        ref_ptr<MeshState>::manage(new UnitCube(cubeConfig)),
        modelMat);
  }

  application->setShowFPS();

  application->mainLoop();
  return 0;
}
