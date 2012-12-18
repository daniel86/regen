
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
"    texel = -0.05*texel;\n"
"}";
const string parallaxMapping =
"#ifndef __TEXCO_PARALLAX\n"
"#define __TEXCO_PARALLAX\n"
"const float parallaxScale = -0.05;\n"
"const float parallaxBias = 0.01;\n"
"vec2 texco_parallax(vec3 P, vec3 N_) {\n"
"    mat3 tbn = mat3(in_tangent,in_binormal,\n"
"       (gl_FrontFacing ? in_norWorld : -in_norWorld));\n"
"    vec2 offset = -normalize( tbn * in_posEye.xyz ).xy;\n"
#if 0
"    float height = parallaxScale * texture(heightTexture, in_texco0).x - parallaxBias;\n"
"    return in_texco0 + height*offset;\n"
#else
"    vec2 texco = in_texco0;\n"
"    for(int i = 0; i < 4; i++) {\n"
"      float normalZ = texture(normalTexture, texco).z;\n"
"      float height = parallaxScale * texture(heightTexture, texco).x - parallaxBias;\n"
"      texco += height * normalZ * offset;\n"
"    }\n"
"    return texco;\n"
#endif
"}\n"
"#endif";
const string reliefMapping =
"const float reliefDepth = 0.105;\n"
"float find_intersection(vec2 dp, vec2 ds, sampler2D tex) {\n"
"  const int linear_steps = 10;\n"
"  const int binary_steps = 5;\n"
"  float depth_step = 1.0 / linear_steps;\n"
"  float size = depth_step;\n"
"  float depth = 1.0;\n"
"  float best_depth = 1.0;\n"
"  for (int i = 0 ; i < linear_steps - 1 ; ++i) {\n"
"     depth -= size;\n"
"     vec4 t = texture2D(tex, dp + ds * depth);\n"
"     if (depth >= 1.0 - t.r) best_depth = depth;\n"
"  }\n"
"  depth = best_depth - size;\n"
"  for (int i = 0 ; i < binary_steps ; ++i) {\n"
"     size *= 0.5;\n"
"     vec4 t = texture2D(tex, dp + ds * depth);\n"
"     if (depth >= 1.0 - t.r) {\n"
"         best_depth = depth;\n"
"         depth -= 2 * size;\n"
"     }\n"
"     depth += size;\n"
"  }\n"
"  return best_depth;\n"
"}\n"
"vec2 texco_relief(vec3 P, vec3 N_) {\n"
"    vec3 T = in_tangent;\n"
"    vec3 N = (gl_FrontFacing ? in_norWorld : -in_norWorld);\n"
"    vec3 B = in_binormal;\n"
"    mat3 tbn = mat3(T,B,N);"
"\n"
"    vec3 eview =  normalize(in_posEye.xyz);\n"
"    vec3 tview = normalize( tbn * in_posEye.xyz );\n"
"    vec2 ds = tview.xy * reliefDepth / tview.z;\n"
"    vec2 dp = in_texco0;\n"
"    float dist = find_intersection(dp, ds, heightTexture);\n"
"    return dp + dist * ds;\n"
"}\n";

class RotateCube : public Animation
{
public:
  RotateCube(ref_ptr<ModelTransformationState> &modelMat)
  : Animation(),
    modelMat_(modelMat)
  {
    enabled = GL_TRUE;
    rotation_ = identity4f();
  }
  virtual void animate(GLdouble dt)
  {
    if(enabled) {
      rotation_ = rotation_ * xyzRotationMatrix(0.001352*dt, 0.002345*dt, 0.0);
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
  // TODO: parallax/relief mapping
#ifdef USE_PARALLAX_MAPPING
  NM_MODE_PARALLAX_MAPPING,
#endif
#ifdef USE_RELIEF_MAPPING
  NM_MODE_RELIEF_MAPPING,
#endif
  NM_MODE_TESSELATION,
  NM_MODE_LAST
};

TessPrimitive tessPrimitive = TESS_PRIMITVE_TRIANGLES;
GLuint tessVertices = 3;
TessVertexSpacing tessSpacing = TESS_SPACING_FRACTIONAL_ODD;
TessVertexOrdering tessOrdering = TESS_ORDERING_CCW;
TessLodMetric tessMetric = TESS_LOD_CAMERA_DISTANCE_INVERSE;

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
#ifdef USE_PARALLAX_MAPPING
  case NM_MODE_PARALLAX_MAPPING:
    application->set_windowTitle("Parallax Mapping");
    break;
#endif
#ifdef USE_RELIEF_MAPPING
  case NM_MODE_RELIEF_MAPPING:
    application->set_windowTitle("Relief Mapping");
    break;
#endif
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

  UnitCube::Config cubeConfig;
  cubeConfig.texcoMode = UnitCube::TEXCO_MODE_UV;
  cubeConfig.isNormalRequired = GL_TRUE;
  cubeConfig.isTangentRequired = GL_TRUE;
  cubeConfig.posScale = Vec3f(0.25f);
  ref_ptr<MeshState> mesh =
      ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));

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
    tessState->set_lodFactor(5.0f);
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
#ifdef USE_PARALLAX_MAPPING
    if(mode == NM_MODE_PARALLAX_MAPPING) {
      normalMapState->set_mappingFunction(parallaxMapping, "texco_parallax");
    }
#endif
#ifdef USE_RELIEF_MAPPING
    if(mode == NM_MODE_RELIEF_MAPPING) {
      normalMapState->set_mappingFunction(reliefMapping, "texco_relief");
    }
#endif
    material->addTexture(normalMapState);
  }

  if(mode > NM_MODE_NORMAL_MAPPING) {
    ref_ptr<TextureState> heightMapState =
        ref_ptr<TextureState>::manage(new TextureState(heightMap_));
    heightMapState->set_name("heightTexture");
    if(mode == NM_MODE_TESSELATION) {
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
  cam->set_direction(Vec3f(0.0,0.0,1.0));
  cam->set_position(Vec3f(0.0,0.0,-1.0));
  cam->updatePerspective(0.0f);
  cam->update(0.0f);

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.7f,0.6f,0.5f,1.0f)
  );

  ref_ptr<DirectionalLight> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

  colMap_ = ref_ptr<Texture>::manage(new ImageTexture("res/textures/relief/color.jpg"));
  norMap_ = ref_ptr<Texture>::manage(new ImageTexture("res/textures/relief/normal.png"));
  heightMap_ = ref_ptr<Texture>::manage(new ImageTexture("res/textures/relief/height.png"));

  setMode(NM_MODE_TESSELATION);
  ref_ptr<NMKeyEventHandler> keyHandler =
      ref_ptr<NMKeyEventHandler>::manage(new NMKeyEventHandler);
  application->connect(OGLEApplication::KEY_EVENT,
      ref_ptr<EventCallable>::cast(keyHandler));

  renderTree->setShowFPS();
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
  return application->mainLoop();
}
