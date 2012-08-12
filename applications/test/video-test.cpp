
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Hello World!");

  application->setClearScreenColor(Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f));
  application->globalStates()->state()->addEnabler(
      ref_ptr<Callable>::manage(new ClearDepthState));

  application->setLight();
  application->perspectiveCamera()->set_isAudioListener(true);

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitQuad::Config quadConfig;
    quadConfig.levelOfDetail = 0;
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.5*M_PI, 0.0f, 0.0f);
    quadConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);
    ref_ptr<AttributeState> quad =
        ref_ptr<AttributeState>::manage(new UnitQuad(quadConfig));

    ref_ptr<VideoTexture> v = ref_ptr<VideoTexture>::manage(new VideoTexture);
    v->set_file("res/textures/video.avi");
    v->set_repeat( true );
    v->addMapTo(MAP_TO_DIFFUSE);
    ref_ptr<AudioSource> audio = v->audioSource();

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.5f, 0.0f), 0.0f);
    modelMat->set_audioSource( audio );

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading( Material::PHONG_SHADING );
    material->set_twoSided(true);
    material->addTexture(ref_ptr<Texture>::cast(v));

    //quad->set_isSprite(true);
    application->addMesh(quad, modelMat, material);

    v->play();
  }

  //application->setShowFPS();

  application->mainLoop();
  return 0;
}
