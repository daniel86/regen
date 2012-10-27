
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
  application->set_windowTitle("FXAA Test");
  application->show();

  ref_ptr<Animation> cameraManipulator = ref_ptr<Animation>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(cameraManipulator);

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  renderTree->setLight();

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    renderTree->addMesh(
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig)),
        modelMat,
        ref_ptr<Material>::manage(new Material));
  }
/*
  FXAA::Config aaCfg;
  aaCfg.spanMax = 8.0;
  aaCfg.reduceMin = 1.0/128.0;
  aaCfg.reduceMul = 1.0/8.0;
  aaCfg.edgeThreshold = 1.0/8.0;
  aaCfg.edgeThresholdMin = 1.0/16.0;
  renderTree->addAntiAliasingPass(aaCfg);
  */

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  renderTree->addSkyBox("res/textures/cube-violentdays.jpg");
  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  return application->mainLoop();
}
