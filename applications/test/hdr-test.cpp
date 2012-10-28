
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/textures/cube-image-texture.h>
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
  application->set_windowTitle("HDR test");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  const string skyImage = "res/textures/cube-grace.hdr";
  const GLboolean flipBackFace = GL_TRUE;
  const GLenum textureFormat = GL_R11F_G11F_B10F;
  const GLenum bufferFormat = GL_RGB16F;

  GLfloat scaleX = 0.5f;
  GLfloat scaleY = 0.5f;

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      bufferFormat,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  renderTree->setLight();
  camManipulator->set_radius(2.0f, 0.0);
  camManipulator->setStepLength(0.005f, 0.0);

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  ref_ptr<Texture> skyTex = ref_ptr<Texture>::manage(
      new CubeImageTexture(skyImage, textureFormat, flipBackFace));
  skyTex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  skyTex->setupMipmaps(GL_DONT_CARE);
  skyTex->set_wrapping(GL_CLAMP_TO_EDGE);

  ref_ptr<TextureState> refractionTexture =
      ref_ptr<TextureState>::manage(new TextureState(skyTex));
  refractionTexture->addMapTo(MAP_TO_COLOR);
  refractionTexture->set_blendMode(BLEND_MODE_SRC);
  refractionTexture->set_mapping(MAPPING_REFRACTION);

  ref_ptr<TextureState> reflectionTexture =
      ref_ptr<TextureState>::manage(new TextureState(skyTex));
  reflectionTexture->addMapTo(MAP_TO_COLOR);
  reflectionTexture->set_blendMode(BLEND_MODE_MIX);
  reflectionTexture->set_blendFactor(0.35f);
  reflectionTexture->set_mapping(MAPPING_REFLECTION);

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    sphereConfig.levelOfDetail = 5;
    ref_ptr<MeshState> meshState = ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::NO_SHADING );
    material->addTexture(refractionTexture);
    material->addTexture(reflectionTexture);

    renderTree->addMesh(meshState, modelMat, material);
  }

  renderTree->addSkyBox(skyTex);

  // render blurred scene in separate buffer
  ref_ptr<FBOState> blurBuffer = renderTree->addBlurPass(scaleX, scaleY);
  // combine blurred and original scene
  ref_ptr<Texture> &blurTexture = blurBuffer->fbo()->firstColorBuffer();
  //ref_ptr<Texture> blurTexture = renderTree->sceneTexture();

  // tonemap parameters are defined as const in shader,
  // but if we declare a shader input in the node we can use
  // these parameters as uniform or attribute.
  // you could change this uniform in event handlers
  ref_ptr<ShaderInput1f> exposureUniform =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("exposure"));
  exposureUniform->setUniformData(8.2f);
  ref_ptr<State> tonemapState = ref_ptr<State>::manage(
      new ShaderInputState(ref_ptr<ShaderInput>::cast(exposureUniform)));
  renderTree->addTonemapPass(blurTexture, scaleX, scaleY, tonemapState);

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  return application->mainLoop();
}
