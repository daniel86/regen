
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
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

const string transferTBNNormal =
"void transferTBNNormal(inout vec4 texel) {\n"
"#if SHADER_STAGE==fs\n"
"    vec3 T = in_tangent;\n"
"    vec3 N = (gl_FrontFacing ? in_norWorld : -in_norWorld);\n"
"    vec3 B = in_binormal;\n"
"    mat3 tbn = mat3(T,B,N);"
"    texel.xyz = normalize( tbn * texel.xyz );\n"
"#endif\n"
"}";
const string transferBrickHeight =
"void transferBrickHeight(inout vec4 texel) {\n"
"    texel *= 0.05;\n"
"}";
// FIXME: color and normap can share this computation
const string parallaxMapping =
"#ifndef __TEXCO_PARALLAX\n"
"#define __TEXCO_PARALLAX\n"
"const float parallaxScale = 0.015;\n"
"const float parallaxBias = 0.0125;\n"
"vec2 texco_parallax(vec3 P, vec3 N_) {\n"
"    mat3 tbn = mat3(in_tangent,in_binormal,\n"
"       (gl_FrontFacing ? in_norWorld : -in_norWorld));\n"
"    vec2 offset = -normalize( tbn * in_posEye.xyz ).xy;\n"
"    float height = parallaxScale * texture(heightTexture, in_texco0).x - parallaxBias;\n"
"    return in_texco0 + height*offset;\n"
/*
"    vec2 texco = in_texco0;\n"
"    for(int i = 0; i < 4; i++) {\n"
"      float normalZ = texture(normalTexture, texco).z;\n"
"      float height = parallaxScale * texture(heightTexture, texco).x - parallaxBias;\n"
"      texco += height * normalZ * offset;\n"
"    }\n"
"    return texco;\n"
*/
"}\n"
"#endif";

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;
  ref_ptr<TextureState> texState;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("Normal/Height Mapping");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  camManipulator->setStepLength(0.0f,0.0f);
  camManipulator->set_degree(0.0f,0.0f);
  camManipulator->set_height(0.0f,0.0f);
  camManipulator->set_radius(5.0f, 0.0f);
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.4f)
  );

  ref_ptr<DirectionalLight> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

  ref_ptr<ModelTransformationState> modelMat;

  TessPrimitive tessPrimitive = TESS_PRIMITVE_TRIANGLES;
  GLuint tessVertices = 3;
  TessVertexSpacing tessSpacing = TESS_SPACING_FRACTIONAL_ODD;
  TessVertexOrdering tessOrdering = TESS_ORDERING_CW;
  TessLodMetric tessMetric = TESS_LOD_CAMERA_DISTANCE_INVERSE;
  Tesselation tessCfg(tessPrimitive, tessVertices);
  tessCfg.ordering = tessOrdering;
  tessCfg.spacing = tessSpacing;
  tessCfg.lodMetric = tessMetric;

  UnitQuad::Config quadConfig;
  quadConfig.levelOfDetail = 2;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_TRUE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.0*M_PI, 0.0*M_PI, 1.0*M_PI);
  quadConfig.posScale = Vec3f(2.0f, 2.0f, 2.0f);

  ref_ptr<Texture> colMap_ = ref_ptr<Texture>::manage(
      new ImageTexture("res/textures/brick/color.jpg"));
  ref_ptr<Texture> norMap_ = ref_ptr<Texture>::manage(
      new ImageTexture("res/textures/brick/normal.jpg"));
  ref_ptr<Texture> heightMap_ = ref_ptr<Texture>::manage(
      new ImageTexture("res/textures/brick/bump.jpg"));
  {
    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_UV;
    cubeConfig.isNormalRequired = GL_TRUE;
    cubeConfig.posScale = Vec3f(0.5f);

    ref_ptr<MeshState> mesh =
        ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f,-0.75f,0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->setConstantUniforms(GL_TRUE);

    texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->setMapTo(MAP_TO_COLOR);
    texState->set_blendMode(BLEND_MODE_SRC);
    material->addTexture(texState);

    renderTree->addMesh(mesh, modelMat, material);
  }
  {
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(1.0f, -1.0f, -2.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_twoSided(GL_TRUE);
    material->setConstantUniforms(GL_TRUE);

    texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->setMapTo(MAP_TO_COLOR);
    texState->set_blendMode(BLEND_MODE_SRC);
    material->addTexture(texState);

    renderTree->addMesh(quad, modelMat, material);
  }
  quadConfig.isTangentRequired = GL_TRUE;
  {
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(1.0f, -1.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_twoSided(GL_TRUE);
    material->setConstantUniforms(GL_TRUE);

    texState = ref_ptr<TextureState>::manage(new TextureState(norMap_));
    texState->set_name("normalTexture");
    texState->setMapTo(MAP_TO_NORMAL);
    texState->set_blendMode(BLEND_MODE_SRC);
    texState->set_transferFunction(transferTBNNormal, "transferTBNNormal");
    material->addTexture(texState);

    texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->set_name("colorTexture");
    texState->setMapTo(MAP_TO_COLOR);
    texState->set_blendMode(BLEND_MODE_SRC);
    material->addTexture(texState);

    renderTree->addMesh(quad, modelMat, material);
  }
  {
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(-1.0f, -1.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_twoSided(GL_TRUE);
    material->setConstantUniforms(GL_TRUE);

    texState = ref_ptr<TextureState>::manage(new TextureState(norMap_));
    texState->set_name("normalTexture");
    texState->setMapTo(MAP_TO_NORMAL);
    texState->set_blendMode(BLEND_MODE_SRC);
    texState->set_mappingFunction(parallaxMapping, "texco_parallax");
    texState->set_transferFunction(transferTBNNormal, "transferTBNNormal");
    material->addTexture(texState);

    texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->set_name("colorTexture");
    texState->setMapTo(MAP_TO_COLOR);
    texState->set_blendMode(BLEND_MODE_SRC);
    texState->set_mappingFunction(parallaxMapping, "texco_parallax");
    material->addTexture(texState);

    texState = ref_ptr<TextureState>::manage(new TextureState(heightMap_));
    texState->set_name("heightTexture");
    material->addTexture(texState);

    renderTree->addMesh(quad, modelMat, material);
  }
  {
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(-1.0f, -1.025f, 2.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_twoSided(GL_TRUE);
    material->setConstantUniforms(GL_TRUE);

    ref_ptr<TesselationState> tessState =
        ref_ptr<TesselationState>::manage(new TesselationState(tessCfg));
    tessState->set_lodFactor(20.0f);
    quad->set_primitive(GL_PATCHES);
    material->joinStates(ref_ptr<State>::cast(tessState));

    texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->setMapTo(MAP_TO_COLOR);
    texState->set_blendMode(BLEND_MODE_SRC);
    material->addTexture(texState);

    texState = ref_ptr<TextureState>::manage(new TextureState(norMap_));
    texState->setMapTo(MAP_TO_NORMAL);
    texState->set_blendMode(BLEND_MODE_SRC);
    texState->set_transferFunction(transferTBNNormal, "transferTBNNormal");
    material->addTexture(texState);

    texState = ref_ptr<TextureState>::manage(new TextureState(heightMap_));
    texState->setMapTo(MAP_TO_HEIGHT);
    texState->set_blendMode(BLEND_MODE_ADD);
    texState->set_transferFunction(transferBrickHeight,"transferBrickHeight");
    material->addTexture(texState);

    renderTree->addMesh(quad, modelMat, material);
  }
  {
    quadConfig.levelOfDetail = 7;
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));
    quadConfig.levelOfDetail = 2;

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(1.0f, -1.025f, 2.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_twoSided(GL_TRUE);
    material->setConstantUniforms(GL_TRUE);

    texState = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    texState->setMapTo(MAP_TO_COLOR);
    texState->set_blendMode(BLEND_MODE_SRC);
    material->addTexture(texState);

    texState = ref_ptr<TextureState>::manage(new TextureState(norMap_));
    texState->setMapTo(MAP_TO_NORMAL);
    texState->set_blendMode(BLEND_MODE_SRC);
    texState->set_transferFunction(transferTBNNormal, "transferTBNNormal");
    material->addTexture(texState);

    texState = ref_ptr<TextureState>::manage(new TextureState(heightMap_));
    texState->setMapTo(MAP_TO_HEIGHT);
    texState->set_blendMode(BLEND_MODE_ADD);
    texState->set_transferFunction(transferBrickHeight,"transferBrickHeight");
    material->addTexture(texState);

    renderTree->addMesh(quad, modelMat, material);
  }

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
