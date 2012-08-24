
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/textures/cube-image-texture.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "HDR test");

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
  application->camManipulator()->set_radius(2.0f, 0.0);
  application->camManipulator()->setStepLength(0.005f, 0.0);

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;
  const string skyImagePath = "res/textures/cube-clouds";
  const string skyImageExt = "png";

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
    material->set_reflection(0.2f);

    ref_ptr<Texture> skyTex = ref_ptr<Texture>::manage(
        new CubeImageTexture(skyImagePath, skyImageExt));
    skyTex->set_wrapping(GL_CLAMP_TO_EDGE);
    skyTex->set_mapping(MAPPING_REFLECTION_REFRACTION);
    skyTex->addMapTo(MAP_TO_COLOR);
    skyTex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    skyTex->setupMipmaps(GL_DONT_CARE);
    material->addTexture(skyTex);

    application->addMesh(meshState, modelMat, material);

    // TODO: use hdr cubemap (some issues loading it with devil)
    // TODO: hdr post passes (blur+tonemap)
    /*
    {
      HDRConfig hdrCfg;
      hdrCfg.blurCfg.pixelsPerSide = 5;
      hdrCfg.blurCfg.sigma = 3.0f;
      hdrCfg.blurCfg.stepFactor = 1.25f;
      hdr_ = ref_ptr<HDRRenderer>::manage(
          new HDRRenderer(scene_,hdrCfg,GL_RGBA16F) );
    }
     */
  }

  application->addSkyBox(skyImagePath, skyImageExt);
  application->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  application->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  return 0;
}
