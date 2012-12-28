
#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/states/tesselation-state.h>
#include <ogle/textures/texture-loader.h>
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
"    mat3 tbn = mat3(in_tangent,in_binormal,in_norWorld);"
"    texel.xyz = normalize( tbn * ( texel.xyz*2.0 - vec3(1.0) ) );\n"
"#endif\n"
"}";
const string transferBrickHeight =
"void transferBrickHeight(inout vec4 texel) {\n"
"    texel *= -0.05;\n"
"}";


const string parallaxMapping =
"#ifndef __TEXCO_PARALLAX\n"
"#define __TEXCO_PARALLAX\n"
"const float parallaxScale = 0.06;\n"
"const float parallaxBias = 0.01;\n"
"vec2 texco_parallax(vec3 P, vec3 N_) {\n"
"    mat3 tts = transpose( mat3(in_tangent,in_binormal,in_norWorld) );\n"
"    vec2 offset = -normalize( tts * (in_cameraPosition - in_posWorld) ).xy;\n"
"    offset.y = -offset.y;\n"
"    float height = parallaxScale * texture(heightTexture, in_texco0).x - parallaxBias;\n"
"    return in_texco0 + height*offset;\n"
"}\n"
"#endif";

const string steepParallaxMapping =
"#ifndef __TEXCO_PARALLAX\n"
"#define __TEXCO_PARALLAX\n"
"const float parallaxScale = 0.05;\n"
"const float parallaxSteps = 5.0;\n"
"vec2 texco_parallax(vec3 P, vec3 N_) {\n"
"    mat3 tts = transpose( mat3(in_tangent,in_binormal,in_norWorld) );\n"
"    vec3 offset = -normalize( tts * (in_cameraPosition - in_posWorld) );\n"
"    offset.y = -offset.y;\n"
"    float numSteps = mix(2.0*parallaxSteps, parallaxSteps, offset.z);\n"
"    float step = 1.0 / numSteps;\n"
"    vec2 delta = offset.xy * parallaxScale / (offset.z * numSteps);\n"
"    vec2 offsetCoord = in_texco0.xy;\n"
"    float NB = texture(heightTexture, offsetCoord).x;\n"
"    float height = 1.0;\n"
"    while (NB < height) {\n"
"        height -= step;\n"
"        offsetCoord += delta;\n"
"        NB = texture(heightTexture, offsetCoord).x;\n"
"    }\n"
"    return offsetCoord;\n"
"}\n"
"#endif";


const string reliefMapping =
"#ifndef __TEXCO_PARALLAX\n"
"#define __TEXCO_PARALLAX\n"
"const float reliefDepth = 0.05;\n"
"uniform mat4 in_viewMatrix;\n"
"uniform mat4 in_modelMatrix;\n"
"float find_intersection(vec2 dp, vec2 delta, sampler2D tex) {\n"
"  const int linear_steps = 10;\n"
"  const int binary_steps = 5;\n"
"  float depth_step = 1.0 / linear_steps;\n"
"  float size = depth_step;\n"
"  float depth = 1.0;\n"
"  float best_depth = 1.0;\n"
"  for (int i = 0 ; i < linear_steps - 1 ; ++i) {\n"
"     depth -= size;\n"
"     vec4 t = texture(tex, dp + delta * depth);\n"
"     if (depth >= 1.0 -  t.r) best_depth = depth;\n"
"  }\n"
"  depth = best_depth - size;\n"
"  for (int i = 0 ; i < binary_steps ; ++i) {\n"
"     size *= 0.5;\n"
"     vec4 t = texture(tex, dp + delta * depth);\n"
"     if (depth >= 1.0 - t.r) {\n"
"         best_depth = depth;\n"
"         depth -= 2 * size;\n"
"     }\n"
"     depth += size;\n"
"  }\n"
"  return best_depth;\n"
"}\n"
"vec2 texco_relief(vec3 P, vec3 N_) {\n"
"    mat3 tts = transpose( mat3(in_tangent,in_binormal,in_norWorld) );\n"
"    vec3 offset = -normalize( tts * (in_cameraPosition - in_posWorld) ).xyz;\n"
"    offset.y = -offset.y;\n"
"    vec2 delta = offset.xy * reliefDepth / offset.z;\n"
"    float dist = find_intersection(in_texco0, delta, heightTexture);\n"
"    return in_texco0 + dist * delta;\n"
"}\n"
"#endif";;

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
      rotation_ = rotation_ * xyzRotationMatrix(0.000135*dt, 0.000234*dt, 0.0);
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
  NM_MODE_STEEP_PARALLAX_MAPPING,
  NM_MODE_RELIEF_MAPPING,
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
  case NM_MODE_STEEP_PARALLAX_MAPPING:
    application->set_windowTitle("Steep Parallax Mapping");
    break;
  case NM_MODE_PARALLAX_MAPPING:
    application->set_windowTitle("Parallax Mapping");
    break;
  case NM_MODE_RELIEF_MAPPING:
    application->set_windowTitle("Relief Mapping");
    break;
  case NM_MODE_TESSELATION:
    application->set_windowTitle("Tesselation");
    break;
  case NM_MODE_LAST:
    application->set_windowTitle("XXX");
    break;
  }

  TesselationConfig tessCfg(tessPrimitive, tessVertices);
  tessCfg.ordering = tessOrdering;
  tessCfg.spacing = tessSpacing;
  tessCfg.lodMetric = tessMetric;

  material = ref_ptr<Material>::manage(new Material);
  material->setConstantUniforms(GL_TRUE);

  Box::Config cubeConfig;
  cubeConfig.texcoMode = Box::TEXCO_MODE_UV;
  cubeConfig.isNormalRequired = GL_TRUE;
  cubeConfig.isTangentRequired = GL_TRUE;
  cubeConfig.posScale = Vec3f(0.25f);
  cubeConfig.texcoScale = Vec2f(2.0f);
  ref_ptr<MeshState> mesh =
      ref_ptr<MeshState>::manage(new Box(cubeConfig));

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
    tessState->set_lodFactor(1.0f);
    mesh->set_primitive(GL_PATCHES);
    material->joinStates(ref_ptr<State>::cast(tessState));
  }

  ref_ptr<TextureState> colorMapState =
      ref_ptr<TextureState>::manage(new TextureState(colMap_));
  colorMapState->set_blendMode(BLEND_MODE_SRC);
  colorMapState->setMapTo(MAP_TO_COLOR);
  colorMapState->set_name("colorTexture");
    if(mode == NM_MODE_PARALLAX_MAPPING) {
      colorMapState->set_mappingFunction(parallaxMapping, "texco_parallax");
    }
    if(mode == NM_MODE_STEEP_PARALLAX_MAPPING) {
      colorMapState->set_mappingFunction(steepParallaxMapping, "texco_parallax");
    }
    if(mode == NM_MODE_RELIEF_MAPPING) {
      colorMapState->set_mappingFunction(reliefMapping, "texco_relief");
    }
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
    if(mode == NM_MODE_STEEP_PARALLAX_MAPPING) {
      normalMapState->set_mappingFunction(steepParallaxMapping, "texco_parallax");
    }
    if(mode == NM_MODE_RELIEF_MAPPING) {
      normalMapState->set_mappingFunction(reliefMapping, "texco_relief");
    }
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
  cam->set_direction(Vec3f(0.0,-0.0,1.0));
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
  light->set_direction(Vec3f(0.0,1.0,-1.0));
  light->setConstantUniforms(GL_TRUE);

  colMap_ = TextureLoader::load("res/textures/relief/color2.jpg");
  norMap_ = TextureLoader::load("res/textures/relief/normal2.png");
  heightMap_ = TextureLoader::load("res/textures/relief/height2.png");;

  setMode(NM_MODE_TESSELATION);
  ref_ptr<NMKeyEventHandler> keyHandler =
      ref_ptr<NMKeyEventHandler>::manage(new NMKeyEventHandler);
  application->connect(OGLEApplication::KEY_EVENT,
      ref_ptr<EventCallable>::cast(keyHandler));

  renderTree->setShowFPS();
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);
  return application->mainLoop();
}
