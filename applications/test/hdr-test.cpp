
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "HDR test");

  application->setClearScreenColor(Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f));
  application->globalStates()->state()->addEnabler(
      ref_ptr<Callable>::manage(new ClearDepthState));

  application->setLight();

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  {
    /*
    ref_ptr<Texture2D> img2 = ref_ptr<Texture2D>::manage(new CubeImageTexture);
    img2->setGLResources( *img1.get() );
    img2->set_wrapping(GL_CLAMP_TO_EDGE);
    img2->set_mapping(MAPPING_REFLECTION_REFRACTION);
    img2->addMapTo(MAP_TO_COLOR);
    img2->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    img2->setupMipmaps(GL_DONT_CARE);


    ref_ptr<UnitSphere> sphere = ref_ptr<UnitSphere>::manage(new UnitSphere);
    sphere->createVertexData(
          4, //levelOfDetail
          (Vec3f){1.0,1.0,1.0}, //scale
          (Vec2f){1.0,1.0}, //uvScale
          -1);
    sphere->set_vboUsage(VertexBufferObject::USAGE_STATIC);
    sphere->set_useAlpha(true);
    sphere->joinStates(img2);
    sphere->material().set_shading( Material::NO_SHADING );
    sphere->material().set_jade();
    sphere->material().set_reflection(0.2f);
    sphere->translate( (Vec3f){0.0, 0.0, 0.0}, 0.0f );
    scene_->addMesh(sphere);

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

  application->setShowFPS();

  application->mainLoop();
  return 0;
}
