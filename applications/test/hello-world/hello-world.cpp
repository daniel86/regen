/*
 * hello-world.cpp
 *
 *  Created on: 09.08.2012
 *      Author: daniel
 */

#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Hello World!");

  application->setClearScreenColor(Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f));

  ref_ptr<ModelTransformationState> modelMat;

  {
    // add a cube
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(-2.0f, 0.0f, 0.0f), 0.0f);
    application->addMesh(
        ref_ptr<AttributeState>::manage(new UnitCube(cubeConfig)),
        modelMat);
  }

  {
    // add another cube
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(0.5f, 0.5f, 1.0f);
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(-1.5f, 0.0f, -1.5f), 0.0f);
    application->addMesh(
        ref_ptr<AttributeState>::manage(new UnitCube(cubeConfig)),
        modelMat);
  }

  {
    // add a sphere
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(1.0f, 0.0f, 0.0f), 0.0f);
    application->addMesh(
        ref_ptr<AttributeState>::manage(new UnitSphere(sphereConfig)),
        modelMat);
  }

  application->setShowFPS();

  application->mainLoop();
  return 0;
}
