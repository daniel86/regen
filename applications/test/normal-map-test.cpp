
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/states/debug-normal.h>
#include <ogle/states/tesselation-state.h>
#include <ogle/textures/image-texture.h>
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
  static GLboolean useTesselation = GL_TRUE;

  TestRenderTree *renderTree = new TestRenderTree;
  ref_ptr<TextureState> texState;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("NormalMap + HeightMap + Tesselation");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.4f)
  );

  ref_ptr<Light> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

  ref_ptr<ModelTransformationState> modelMat;

  {
    // add a brick textured quad

    TessPrimitive tessPrimitive = TESS_PRIMITVE_TRIANGLES;
    GLuint tessVertices = 3;
    TessVertexSpacing tessSpacing = TESS_SPACING_FRACTIONAL_ODD;
    TessVertexOrdering tessOrdering = TESS_ORDERING_CW;
    TessLodMetric tessMetric = TESS_LOD_CAMERA_DISTANCE_INVERSE;

    UnitQuad::Config quadConfig;
    if(useTesselation) {
      quadConfig.levelOfDetail = 2;
    } else {
      quadConfig.levelOfDetail = 7;
    }
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.isTangentRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(1.5*M_PI, 0.0f, 0.0f);
    quadConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(1.1f, 0.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);

    if(useTesselation) {
      Tesselation tessCfg(tessPrimitive, tessVertices);
      tessCfg.ordering = tessOrdering;
      tessCfg.spacing = tessSpacing;
      tessCfg.lodMetric = tessMetric;
      ref_ptr<TesselationState> tessState =
          ref_ptr<TesselationState>::manage(new TesselationState(tessCfg));
      tessState->set_lodFactor(20.0f);
      quad->set_primitive(GL_PATCHES);
      material->joinStates(ref_ptr<State>::cast(tessState));
    }

    ref_ptr<Texture> colMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/brick/color.jpg"));
    texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->addMapTo(MAP_TO_DIFFUSE);
    material->addTexture(texState);

    ref_ptr<Texture> norMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/brick/normal.jpg"));
    texState = ref_ptr<TextureState>::manage(new TextureState(norMap_));
    texState->set_texelFactor(1.0f);
    texState->addMapTo(MAP_TO_NORMAL);
    material->addTexture(texState);

    ref_ptr<Texture> heightMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/brick/bump.jpg"));
    texState = ref_ptr<TextureState>::manage(new TextureState(heightMap_));
    texState->addMapTo(MAP_TO_HEIGHT);
    texState->set_texelFactor(0.1f);
    material->addTexture(texState);

    material->set_shading( Material::PHONG_SHADING );
    material->set_shininess(0.0);
    material->set_twoSided(true);
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

  {
    // add a terrain textured quad

    TessPrimitive tessPrimitive = TESS_PRIMITVE_TRIANGLES;
    GLuint tessVertices = 3;
    TessVertexSpacing tessSpacing = TESS_SPACING_FRACTIONAL_ODD;
    TessVertexOrdering tessOrdering = TESS_ORDERING_CW;
    TessLodMetric tessMetric = TESS_LOD_CAMERA_DISTANCE_INVERSE;

    UnitQuad::Config quadConfig;
    if(useTesselation) {
      quadConfig.levelOfDetail = 2;
    } else {
      quadConfig.levelOfDetail = 7;
    }
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.isTangentRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(1.5*M_PI, 0.0f, 0.0f);
    quadConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(-1.1f, 0.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);

    if(useTesselation) {
      Tesselation tessCfg(tessPrimitive, tessVertices);
      tessCfg.ordering = tessOrdering;
      tessCfg.spacing = tessSpacing;
      tessCfg.lodMetric = tessMetric;
      ref_ptr<TesselationState> tessState =
          ref_ptr<TesselationState>::manage(new TesselationState(tessCfg));
      tessState->set_lodFactor(20.0f);
      quad->set_primitive(GL_PATCHES);
      material->joinStates(ref_ptr<State>::cast(tessState));
    }

    ref_ptr<Texture> colMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/terrain/color.jpg"));
    texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->addMapTo(MAP_TO_DIFFUSE);
    material->addTexture(texState);

    ref_ptr<Texture> norMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/terrain/normal.jpg"));
    texState = ref_ptr<TextureState>::manage(new TextureState(norMap_));
    texState->set_texelFactor(1.0f);
    texState->addMapTo(MAP_TO_NORMAL);
    material->addTexture(texState);

    ref_ptr<Texture> heightMap_ = ref_ptr<Texture>::manage(
        new ImageTexture("res/textures/terrain/height.jpg"));
    texState = ref_ptr<TextureState>::manage(new TextureState(heightMap_));
    texState->addMapTo(MAP_TO_HEIGHT);
    texState->set_texelFactor(0.5f);
    material->addTexture(texState);

    material->set_shading( Material::PHONG_SHADING );
    material->set_shininess(0.0);
    material->set_twoSided(true);
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
