
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/animations/vbo-morph-animation.h>
#include <ogle/animations/animation-manager.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Simple FBO");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  application->setLight();

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    sphereConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);
    ref_ptr<MeshState> meshState =
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.5f, 0.0f), 0.0f);

    application->addMesh(meshState, modelMat, ref_ptr<Material>::manage(new Material));

    ref_ptr<VBOElasticMorpher> morpher =
        ref_ptr<VBOElasticMorpher>::manage(new VBOElasticMorpher(false));
    const GLdouble springConstant=5.0;
    const GLdouble vertexMass=0.0001;
    const GLdouble friction=0.0001;
    const GLdouble positionThreshold=0.001;
    morpher->setElasticParams(springConstant,
        vertexMass, friction, positionThreshold);

    ref_ptr<VBOMorphAnimation> morphAnim =
        ref_ptr<VBOMorphAnimation>::manage(new VBOMorphAnimation(meshState));
    morphAnim->set_morpher(ref_ptr<VBOMorpher>::cast(morpher));

    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(morphAnim));

    morphAnim->addCubeTarget(2.0f);
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  application->addSkyBox("res/textures/cube-clouds");
  application->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  application->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  return 0;
}
