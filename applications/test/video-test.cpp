
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>
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
  ref_ptr<TextureState> texState;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("libav Video Texture + OpenAL sound");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      TRANSPARENCY_NONE,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f)
  );

  ref_ptr<DirectionalLight> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

  renderTree->perspectiveCamera()->set_isAudioListener(true);
  camManipulator->setStepLength(0.0f,0.0f);
  camManipulator->set_degree(0.0f,0.0f);
  camManipulator->set_height(0.0f,0.0f);
  camManipulator->set_radius(5.0f, 0.0f);

  ref_ptr<ModelTransformationState> modelMat;

  ref_ptr<VideoTexture> v = ref_ptr<VideoTexture>::manage(new VideoTexture);
  v->set_file("res/textures/video.avi");
  v->set_repeat( true );
  ref_ptr<AudioSource> audio = v->audioSource();

  texState = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(v)));
  texState->setMapTo(MAP_TO_COLOR);

  {
    UnitQuad::Config quadConfig;
    quadConfig.levelOfDetail = 0;
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 1.0*M_PI);
    quadConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);
    quadConfig.texcoScale = Vec2f(1.0f, -1.0f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 2.5f), 0.0f);
    modelMat->set_audioSource( audio );
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::NO_SHADING );
    material->set_twoSided(GL_TRUE);
    material->addTexture(texState);
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  v->play();

  return application->mainLoop();
}
