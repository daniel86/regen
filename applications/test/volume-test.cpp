
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/blend-state.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Hello World!");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      800,600,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  application->setLight();

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    cubeConfig.isNormalRequired = GL_TRUE;
    cubeConfig.posScale = Vec3f(1.0f, 1.0f, 1.0f);

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);

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
    transfer->fillColorPositive_->set_value( Vec3f( 0.0f, 0.0f, 0.6f ) );
    transfer->texelFactor_->set_value( 0.4f );
    texState->set_transfer(ref_ptr<TexelTransfer>::cast(transfer));
    material->joinStates(ref_ptr<State>::cast(texState));

    ref_ptr<StateNode> meshNode = application->addMesh(
        ref_ptr<MeshState>::manage(new UnitCube(cubeConfig)),
        modelMat, material);

    ref_ptr<State> alphaBlending = ref_ptr<State>::manage(new BlendState);
    meshNode->state()->joinStates(alphaBlending);
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  application->addSkyBox("res/textures/cube-clouds");
  application->setShowFPS();

  // TODO: screen blit must know screen width/height
  application->setBlitToScreen(
      fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  return 0;
}
