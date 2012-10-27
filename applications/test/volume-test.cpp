
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/blend-state.h>
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
  application->set_windowTitle("Volume Renderer");
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

  ref_ptr<Light> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

  // volume uses transparency the sky box would use depth test against.
  // so we add the sky box before the volume
  renderTree->addSkyBox("res/textures/cube-interstellar.jpg");

  ref_ptr<Material> material;

  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.isNormalRequired = GL_TRUE;
    cubeConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);

    /*
    material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::NO_SHADING );
    ref_ptr<RAWTexture3D> tex = ref_ptr<RAWTexture3D>::manage(new RAWTexture3D());
    RAWTextureFile rawFile;
    rawFile.path = "res/textures/teapot.raw";
    rawFile.bytesPerComponent = 8;
    rawFile.numComponents = 1;
    rawFile.width = 256;
    rawFile.height = 256;
    rawFile.depth = 256;
    tex->loadRAWFile(rawFile);
    tex->addMapTo(MAP_TO_VOLUME);

    ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
        new TextureState(ref_ptr<Texture>::cast(tex)));
    ref_ptr<ScalarToAlphaTransfer> transfer =
        ref_ptr<ScalarToAlphaTransfer>::manage( new ScalarToAlphaTransfer );
    transfer->fillColorPositive_->setUniformData( Vec3f( 0.0f, 0.0f, 0.6f ) );
    transfer->texelFactor_->setUniformData( 0.4f );
    texState->set_transfer(ref_ptr<TexelTransfer>::cast(transfer));
    material->joinStates(ref_ptr<State>::cast(texState));
    */

    material->setConstantUniforms(GL_TRUE);

    ref_ptr<StateNode> meshNode = renderTree->addMesh(
        ref_ptr<MeshState>::manage(new UnitCube(cubeConfig)),
        ref_ptr<ModelTransformationState>(),
        material);

    ref_ptr<State> alphaBlending = ref_ptr<State>::manage(new BlendState);
    meshNode->state()->joinStates(alphaBlending);
  }

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
