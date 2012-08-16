
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Hello World!");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      800,600,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  ref_ptr<Light> &light = application->setLight();
  light->setConstantUniforms(GL_TRUE);

  {
    GLuint numInstancesX = 100;
    GLuint numInstancesY = 100;
    GLuint numInstances = numInstancesX*numInstancesY;
    float instanceDistance = 2.25;

    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.posScale = Vec3f(1.0f, 2.0f, 1.0f);

    ref_ptr<ModelTransformationState> modelMat =
        ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
    Mat4f* instancedModelMat = new Mat4f[numInstances];
    float bufX = -0.5f*numInstancesX*instanceDistance -0.5f;
    float bufY = -0.5f*numInstancesY*instanceDistance -0.5f;
    Vec3f translation = (Vec3f) { bufX, 0.0f, bufY };
    unsigned int i = 0;
    for(unsigned int x=0; x<numInstancesX; ++x) {
      translation.x += instanceDistance;
      for(unsigned int y=0; y<numInstancesY; ++y) {
        translation.z += instanceDistance;
#define RANDOM (rand()%100)/100.0f
        instancedModelMat[i] = transpose(transformMatrix(
            Vec3f(0.0f , 2.0f*RANDOM*M_PI, 0.0f),
            translation + Vec3f(1.5f*(0.5f-RANDOM),0.0f,1.25f*(0.5f-RANDOM)),
            Vec3f(1.0f,1.0f,1.0f)
#undef RANDOM
        ));
        i += 1;
      }
      translation.z = bufY;
    }
    modelMat->modelMat()->setInstanceData(
        numInstances, 1, (byte*)instancedModelMat);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_pewter();
    material->setConstantUniforms(GL_TRUE);

    Vec4f *diffuse = new Vec4f[numInstances];
    for(unsigned int x=0; x<numInstances; ++x) {
#define RANDOM (rand()%100)/100.0f
      diffuse[x] = Vec4f(RANDOM,RANDOM,RANDOM,0.0f);
#undef RANDOM
    }
    material->set_diffuse( numInstances, 1, diffuse );

    application->addMesh(
        ref_ptr<MeshState>::manage(new UnitCube(cubeConfig)),
        modelMat, material);
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  application->addSkyBox("res/textures/cube-clouds");
  application->setShowFPS();

  // TODO: screen blit must know screen width/height
  application->setBlitToScreen(
      fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  return 0;
}
