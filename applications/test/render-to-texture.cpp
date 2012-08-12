
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Hello World!");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      800,600,
      GL_RGBA,GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f)
  );

  application->setLight();

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.5f, 0.0f), 0.0f);
    application->addMesh(
        ref_ptr<AttributeState>::manage(new UnitSphere(sphereConfig)),
        modelMat,
        ref_ptr<Material>::manage(new Material));
  }

  //application->setShowFPS();

  application->setBlitToScreen(
      fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  return 0;
}
