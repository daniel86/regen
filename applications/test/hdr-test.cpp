
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/textures/cube-image-texture.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "HDR test");

  GLenum mipmapFlag = GL_DONT_CARE;
  //GLenum textureFormat = GL_RGB8;
  GLenum textureFormat = GL_R11F_G11F_B10F;
  //GLenum textureFormat = GL_RGB16F;
  //GLenum textureFormat = GL_RGB32F;
  //GLenum textureFormat = GL_NONE;
  GLenum bufferFormat = GL_RGB16F;
  GLboolean flipBackFace = GL_TRUE;

  BlurConfig blurCfg;
  blurCfg.pixelsPerSide = 4;
  blurCfg.sigma = 3.0f;
  blurCfg.stepFactor = 1.0;

  GLfloat scaleX = 0.5f;
  GLfloat scaleY = 0.5f;

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      1.0f,1.0f,
      bufferFormat,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  application->setLight();
  application->camManipulator()->set_radius(2.0f, 0.0);
  application->camManipulator()->setStepLength(0.005f, 0.0);

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;
  const string skyImage = "res/textures/cube-grace.hdr";

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
    material->set_jade();
    material->set_reflection(0.30f);

    ref_ptr<Texture> skyTex = ref_ptr<Texture>::manage(
        new CubeImageTexture(skyImage, mipmapFlag, textureFormat, flipBackFace));
    skyTex->set_wrapping(GL_CLAMP_TO_EDGE);
    skyTex->set_mapping(MAPPING_REFLECTION_REFRACTION);
    skyTex->addMapTo(MAP_TO_COLOR);
    skyTex->set_filter(GL_LINEAR, GL_LINEAR);
    //skyTex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    //skyTex->setupMipmaps(GL_DONT_CARE);
    material->addTexture(skyTex);

    application->addMesh(meshState, modelMat, material);
  }
  application->addSkyBox(skyImage, mipmapFlag, textureFormat, flipBackFace);

  // render blurred scene in separate buffer
  ref_ptr<FBOState> blurBuffer = application->addBlurPass(blurCfg, scaleX, scaleY);

  // combine blurred and original scene
  ref_ptr<Texture> &blurTexture = blurBuffer->fbo()->firstColorBuffer();
  application->addTonemapPass(blurTexture, scaleX, scaleY);

  application->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  application->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT1);

  application->mainLoop();
  return 0;
}
