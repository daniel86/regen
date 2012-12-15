
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
"    texel = 0.015*(texel - vec4(1.0));\n"
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

class RotateCube : public Animation
{
public:
  RotateCube(ref_ptr<ModelTransformationState> &modelMat)
  : Animation(),
    modelMat_(modelMat)
  {
    rotation_ = identity4f();
  }
  virtual void animate(GLdouble dt)
  {
    if(enabled) {
      rotation_ = rotation_ * xyzRotationMatrix(0.001*dt, 0.002*dt, 0.0);
    }
  }
  virtual void updateGraphics(GLdouble dt)
  {
    modelMat_->set_modelMat(rotation_, dt);
  }
  ref_ptr<ModelTransformationState> modelMat_;
  Mat4f rotation_;
  GLboolean enabled;
};

enum NormalMapMode
{
  NM_MODE_NONE,
  NM_MODE_NORMAL_MAPPING,
  NM_MODE_PARALLAX_MAPPING,
  NM_MODE_TESSELATION,
  NM_MODE_LAST
};

TessPrimitive tessPrimitive = TESS_PRIMITVE_TRIANGLES;
GLuint tessVertices = 3;
TessVertexSpacing tessSpacing = TESS_SPACING_FRACTIONAL_ODD;
TessVertexOrdering tessOrdering = TESS_ORDERING_CW;
TessLodMetric tessMetric = TESS_LOD_CAMERA_DISTANCE_INVERSE;

// TODO: cube/sphere tangents
// #define CUBE_MODEL
// #define SPHERE_MODEL
#define QUAD_MODEL

OGLEFltkApplication *application;
TestRenderTree *renderTree;
ref_ptr<Texture> colMap_;
ref_ptr<Texture> norMap_;
ref_ptr<Texture> heightMap_;
ref_ptr<RotateCube> rotateAnimation_;
ref_ptr<MeshState> mesh_;
ref_ptr<StateNode> meshNode_;
ref_ptr<Material> material;
NormalMapMode currentMode_;

void setMode(NormalMapMode mode)
{
  currentMode_ = mode;

  switch(mode) {
  case NM_MODE_NONE:
    application->set_windowTitle("Color map only");
    break;
  case NM_MODE_NORMAL_MAPPING:
    application->set_windowTitle("Normal Mapping");
    break;
  case NM_MODE_PARALLAX_MAPPING:
    application->set_windowTitle("Parallax Mapping");
    break;
  case NM_MODE_TESSELATION:
    application->set_windowTitle("Tesselation");
    break;
  case NM_MODE_LAST:
    application->set_windowTitle("XXX");
    break;
  }

  Tesselation tessCfg(tessPrimitive, tessVertices);
  tessCfg.ordering = tessOrdering;
  tessCfg.spacing = tessSpacing;
  tessCfg.lodMetric = tessMetric;

  material = ref_ptr<Material>::manage(new Material);
  material->setConstantUniforms(GL_TRUE);

#ifdef CUBE_MODEL
  UnitCube::Config cubeConfig;
  cubeConfig.texcoMode = UnitCube::TEXCO_MODE_UV;
  cubeConfig.isNormalRequired = GL_TRUE;
  cubeConfig.isTangentRequired = GL_TRUE;
  cubeConfig.posScale = Vec3f(0.5f);
  ref_ptr<MeshState> mesh =
      ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));
#endif
#ifdef SPHERE_MODEL
  UnitSphere::Config sphereConfig;
  sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_UV;
  sphereConfig.isNormalRequired = GL_TRUE;
  sphereConfig.isTangentRequired = GL_TRUE;
  sphereConfig.levelOfDetail = 3;
  ref_ptr<MeshState> mesh =
      ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));
#endif
#ifdef QUAD_MODEL
  UnitQuad::Config quadConfig;
  quadConfig.levelOfDetail = 2;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_TRUE;
  quadConfig.isTangentRequired = GL_TRUE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.0*M_PI, 0.0*M_PI, 1.0*M_PI);
  quadConfig.posScale = Vec3f(0.65f);
  ref_ptr<MeshState> mesh =
      ref_ptr<MeshState>::manage(new UnitQuad(quadConfig));
  material->set_twoSided(GL_TRUE);
#endif

  ref_ptr<ModelTransformationState> modelMat;
  modelMat = ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
  modelMat->translate(Vec3f(0.0f,-0.75f,0.0f), 0.0f);
  if(rotateAnimation_.get()!=NULL) {
    rotateAnimation_->modelMat_ = modelMat;
  }
  else {
    rotateAnimation_ = ref_ptr<RotateCube>::manage(new RotateCube(modelMat));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(rotateAnimation_));
  }

  if(mode == NM_MODE_TESSELATION) {
    ref_ptr<TesselationState> tessState =
        ref_ptr<TesselationState>::manage(new TesselationState(tessCfg));
    tessState->set_lodFactor(0.4f);
    mesh->set_primitive(GL_PATCHES);
    material->joinStates(ref_ptr<State>::cast(tessState));
  }

  ref_ptr<TextureState> colorMapState =
      ref_ptr<TextureState>::manage(new TextureState(colMap_));
  colorMapState->set_blendMode(BLEND_MODE_SRC);
  colorMapState->setMapTo(MAP_TO_COLOR);
  colorMapState->set_name("colorTexture");
  material->addTexture(colorMapState);

  if(mode != NM_MODE_NONE) {
    ref_ptr<TextureState> normalMapState =
        ref_ptr<TextureState>::manage(new TextureState(norMap_));
    normalMapState->set_name("normalTexture");
    normalMapState->set_blendMode(BLEND_MODE_SRC);
    normalMapState->setMapTo(MAP_TO_NORMAL);
    normalMapState->set_transferFunction(transferTBNNormal, "transferTBNNormal");
    if(mode == NM_MODE_PARALLAX_MAPPING) {
      normalMapState->set_mappingFunction(parallaxMapping, "texco_parallax");
    }
    material->addTexture(normalMapState);
  }

  if(mode > NM_MODE_NORMAL_MAPPING) {
    ref_ptr<TextureState> heightMapState =
        ref_ptr<TextureState>::manage(new TextureState(heightMap_));
    heightMapState->set_name("heightTexture");
    if(mode != NM_MODE_PARALLAX_MAPPING) {
      heightMapState->set_blendMode(BLEND_MODE_ADD);
      heightMapState->setMapTo(MAP_TO_HEIGHT);
      heightMapState->set_transferFunction(transferBrickHeight, "transferBrickHeight");
    }
    material->addTexture(heightMapState);
  }

  if(meshNode_.get()) {
    renderTree->removeMesh(meshNode_);
  }
  meshNode_ = renderTree->addMesh(mesh, modelMat, material);
}

class NMKeyEventHandler : public EventCallable
{
public:
  NMKeyEventHandler() : EventCallable() {}
  void call(EventObject *ev, void *data)
  {
    OGLEApplication::KeyEvent *keyEv = (OGLEApplication::KeyEvent*)data;
    if(!keyEv->isUp) { return; }

    if(keyEv->key == ' ') {
      int mode = (int)currentMode_+1;
      if(mode == NM_MODE_LAST) { mode = NM_MODE_NONE; }
      setMode((NormalMapMode)mode);
    }
    if(keyEv->key == 'm') {
      if(material->fillMode() == GL_LINE) {
        material->set_fillMode(GL_FILL);
      } else {
        material->set_fillMode(GL_LINE);
      }
    }
    if(keyEv->key == 'r') {
      rotateAnimation_->enabled = !rotateAnimation_->enabled;
    }
  }
};

int main(int argc, char** argv)
{
  renderTree = new TestRenderTree;

  application = new OGLEFltkApplication(renderTree, argc, argv);
  application->set_windowTitle("Normal Mapping");
  application->show();

  ref_ptr<PerspectiveCamera> &cam = renderTree->perspectiveCamera();
  cam->set_direction(Vec3f(0.0,0.0,-1.0));
  cam->set_position(Vec3f(0.0,0.0,1.0));
  cam->updatePerspective(0.0f);
  cam->update(0.0f);

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.6f, 0.5f, 0.4f, 0.0f)
  );

  ref_ptr<DirectionalLight> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

  colMap_ = ref_ptr<Texture>::manage(new ImageTexture("res/textures/brick2/color.jpg"));
  norMap_ = ref_ptr<Texture>::manage(new ImageTexture("res/textures/brick2/normal.jpg"));
  heightMap_ = ref_ptr<Texture>::manage(new ImageTexture("res/textures/brick2/height.jpg"));

  setMode(NM_MODE_TESSELATION);
  ref_ptr<NMKeyEventHandler> keyHandler =
      ref_ptr<NMKeyEventHandler>::manage(new NMKeyEventHandler);
  application->connect(OGLEApplication::KEY_EVENT,
      ref_ptr<EventCallable>::cast(keyHandler));

  renderTree->setShowFPS();
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
  return application->mainLoop();
}
