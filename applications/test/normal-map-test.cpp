

#include "factory.h"

#define USE_HUD

static const string transferTBNNormal =
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
    "#endif";

class NormalMapLoader : public EventCallable, public Animation
{
public:
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

  NormalMapLoader(OGLEFltkApplication *app, const ref_ptr<StateNode> &root)
  : EventCallable(), Animation(), app_(app)
  {
    rotateEnabled_ = GL_TRUE;
    rotation_ = Mat4f::identity();

    Box::Config meshCfg;
    meshCfg.texcoMode = Box::TEXCO_MODE_UV;
    meshCfg.isNormalRequired = GL_TRUE;
    meshCfg.isTangentRequired = GL_TRUE;
    meshCfg.posScale = Vec3f(0.25f);
    meshCfg.texcoScale = Vec2f(1.0f);
    mesh_ = ref_ptr<MeshState>::manage(new Box(meshCfg));

    modelMat_ = ref_ptr<ModelTransformation>::manage(new ModelTransformation);
    modelMat_->translate(Vec3f(0.0f,-0.75f,0.0f), 0.0f);
    mesh_->joinStates(ref_ptr<State>::cast(modelMat_));

    material_ = ref_ptr<Material>::manage(new Material);
    material_->set_chrome();
    mesh_->joinStates(ref_ptr<State>::cast(material_));

    shaderState_ = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh_->joinStates(ref_ptr<State>::cast(shaderState_));

    node_ = ref_ptr<StateNode>::manage(new StateNode(ref_ptr<State>::cast(mesh_)));
    root->addChild(node_);

    colMap_ = TextureLoader::load("res/textures/relief/color2.jpg");
    norMap_ = TextureLoader::load("res/textures/relief/normal2.png");
    heightMap_ = TextureLoader::load("res/textures/relief/height2.png");

    setMode(NM_MODE_TESSELATION);
    createShader();
  }

  void createShader()
  {
    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(node_.get());
    shaderState_->createShader(shaderConfigurer.cfg(), "mesh");
  }

  void setMode(NormalMapMode mode)
  {
    mode_ = mode;

    mesh_->set_primitive(GL_TRIANGLES);
    if(tessState_.get())
      modelMat_->disjoinStates(ref_ptr<State>::cast(tessState_));
    if(colorMapState_.get())
      modelMat_->disjoinStates(ref_ptr<State>::cast(colorMapState_));
    if(normalMapState_.get())
      modelMat_->disjoinStates(ref_ptr<State>::cast(normalMapState_));
    if(heightMapState_.get())
      modelMat_->disjoinStates(ref_ptr<State>::cast(heightMapState_));

    colorMapState_ = ref_ptr<TextureState>::manage(new TextureState(colMap_));
    colorMapState_->set_name("colorTexture");
    colorMapState_->set_blendMode(BLEND_MODE_SRC);
    colorMapState_->setMapTo(MAP_TO_COLOR);
    switch(mode) {
    case NM_MODE_STEEP_PARALLAX_MAPPING:
      colorMapState_->set_mappingFunction(steepParallaxMapping, "texco_parallax");
      break;
    case NM_MODE_PARALLAX_MAPPING:
      colorMapState_->set_mappingFunction(parallaxMapping, "texco_parallax");
      break;
    case NM_MODE_RELIEF_MAPPING:
      colorMapState_->set_mappingFunction(reliefMapping, "texco_relief");
      break;
    default:
      break;
    }
    modelMat_->joinStates(ref_ptr<State>::cast(colorMapState_));

    if(mode != NM_MODE_NONE) {
      normalMapState_ = ref_ptr<TextureState>::manage(new TextureState(norMap_));
      normalMapState_->set_name("normalTexture");
      normalMapState_->set_blendMode(BLEND_MODE_SRC);
      normalMapState_->setMapTo(MAP_TO_NORMAL);
      normalMapState_->set_transferFunction(transferTBNNormal, "transferTBNNormal");
      if(mode == NM_MODE_PARALLAX_MAPPING) {
        normalMapState_->set_mappingFunction(parallaxMapping, "texco_parallax");
      }
      if(mode == NM_MODE_STEEP_PARALLAX_MAPPING) {
        normalMapState_->set_mappingFunction(steepParallaxMapping, "texco_parallax");
      }
      if(mode == NM_MODE_RELIEF_MAPPING) {
        normalMapState_->set_mappingFunction(reliefMapping, "texco_relief");
      }
      modelMat_->joinStates(ref_ptr<State>::cast(normalMapState_));
    }

    if(mode > NM_MODE_NORMAL_MAPPING) {
      heightMapState_ = ref_ptr<TextureState>::manage(new TextureState(heightMap_));
      heightMapState_->set_name("heightTexture");
      if(mode == NM_MODE_TESSELATION) {
        heightMapState_->set_blendMode(BLEND_MODE_ADD);
        heightMapState_->setMapTo(MAP_TO_HEIGHT);
        heightMapState_->set_transferFunction(transferBrickHeight, "transferBrickHeight");
      }
      modelMat_->joinStates(ref_ptr<State>::cast(heightMapState_));
    }

    switch(mode) {
    case NM_MODE_NONE:
      app_->set_windowTitle("Color map only");
      break;
    case NM_MODE_NORMAL_MAPPING:
      app_->set_windowTitle("Normal Mapping");
      break;
    case NM_MODE_STEEP_PARALLAX_MAPPING:
      app_->set_windowTitle("Steep Parallax Mapping");
      break;
    case NM_MODE_PARALLAX_MAPPING:
      app_->set_windowTitle("Parallax Mapping");
      break;
    case NM_MODE_RELIEF_MAPPING:
      app_->set_windowTitle("Relief Mapping");
      break;
    case NM_MODE_TESSELATION:
      app_->set_windowTitle("Tesselation");
      break;
    case NM_MODE_LAST:
      app_->set_windowTitle("XXX");
      break;
    }

    if(mode == NM_MODE_TESSELATION) {
      tessState_ = ref_ptr<TesselationState>::manage(new TesselationState(3));
      tessState_->set_lodMetric(TesselationState::CAMERA_DISTANCE_INVERSE);
      tessState_->lodFactor()->setVertex1f(0,1.0f);
      mesh_->set_primitive(GL_PATCHES);
      modelMat_->joinStates(ref_ptr<State>::cast(tessState_));
    }
  }
  void nextMode()
  {
    GLint mode = (GLint)mode_+1;
    if(mode == NM_MODE_LAST) { mode = 0; }
    setMode((NormalMapMode)mode);
  }

  virtual void call(EventObject *ev, void *data)
  {
    OGLEApplication::KeyEvent *keyEv = (OGLEApplication::KeyEvent*)data;
    if(!keyEv->isUp) { return; }

    if(keyEv->key == ' ') {
      nextMode();
      createShader();
    }
    if(keyEv->key == 'm') {
      if(material_->fillMode() == GL_LINE) {
        material_->set_fillMode(GL_FILL);
      } else {
        material_->set_fillMode(GL_LINE);
      }
    }
    if(keyEv->key == 'r') {
      rotateEnabled_ = !rotateEnabled_;
    }
  }

  virtual void animate(GLdouble dt)
  {
    if(rotateEnabled_) {
      rotation_ = rotation_ * Mat4f::rotationMatrix(0.000135*dt, 0.000234*dt, 0.0);
    }
  }
  virtual void glAnimate(GLdouble dt)
  {
    modelMat_->set_modelMat(rotation_, dt);
  }
  virtual GLboolean useGLAnimation() const
  {
    return GL_TRUE;
  }
  virtual GLboolean useAnimation() const
  {
    return GL_TRUE;
  }

protected:
  OGLEFltkApplication *app_;

  ref_ptr<MeshState> mesh_;
  ref_ptr<StateNode> node_;
  ref_ptr<ShaderState> shaderState_;
  ref_ptr<ModelTransformation> modelMat_;
  ref_ptr<Material> material_;

  ref_ptr<TesselationState> tessState_;
  ref_ptr<TextureState> colorMapState_;
  ref_ptr<TextureState> normalMapState_;
  ref_ptr<TextureState> heightMapState_;

  ref_ptr<Texture> colMap_;
  ref_ptr<Texture> norMap_;
  ref_ptr<Texture> heightMap_;

  GLboolean rotateEnabled_;
  Mat4f rotation_;
  NormalMapMode mode_;
};

int main(int argc, char** argv)
{
  ref_ptr<OGLEFltkApplication> app = initApplication(argc,argv,"Normal Mapping");

  // create a root node for everything that needs camera as input
  ref_ptr<PerspectiveCamera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 0.2f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 2.0f );
  manipulator->set_degree( 0.0f );
  manipulator->setStepLength( M_PI*0.0 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  ref_ptr<Frustum> frustum = ref_ptr<Frustum>::manage(new Frustum);
  frustum->setProjection(cam->fov(), cam->aspect(), cam->near(), cam->far());

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get());
  ref_ptr<StateNode> gBufferParent = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gBufferState)));
  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage( new StateNode);
  sceneRoot->addChild(gBufferParent);
  gBufferParent->addChild(gBufferNode);

  ref_ptr<Texture> gDiffuseTexture = gBufferState->fbo()->colorBuffer()[0];
  ref_ptr<Texture> gDepthTexture = gBufferState->fbo()->depthTexture();
  sceneRoot->addChild(gBufferNode);

  ref_ptr<NormalMapLoader> nmLoader = ref_ptr<NormalMapLoader>::manage(
      new NormalMapLoader(app.get(), gBufferNode));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(nmLoader));
  app->connect(OGLEApplication::KEY_EVENT, ref_ptr<EventCallable>::cast(nmLoader));

  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gBufferState->fbo(), sceneRoot);

  ref_ptr<SpotLight> spotLight = createSpotLight(app.get());
  spotLight->set_position(Vec3f(0.0f,2.0f,0.0f));
  spotLight->set_spotDirection(Vec3f(0.0f,-1.0f,0.0f));
  spotLight->set_diffuse(Vec3f(0.5f,0.5f,0.5f));
  spotLight->set_specular(Vec3f(0.0f));
  spotLight->set_innerRadius(1.0);
  spotLight->set_outerRadius(4.0);
  spotLight->coneAngle()->getVertex2f(0) = Vec2f(0.98, 0.9);
  //ref_ptr<SpotShadowMap> spotShadow = createSpotShadow(app.get(), spotLight, cam);
  //spotShadow->addCaster(gBufferNode);
  //deferredShading->addLight(spotLight, spotShadow);
  deferredShading->addLight(ref_ptr<Light>::cast(spotLight));

  ref_ptr<StateNode> postPassNode = createPostPassNode(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(postPassNode);

  ref_ptr<VolumetricFog> volumeFog = createVolumeFog(app.get(), gDepthTexture, postPassNode);
  ref_ptr<ShaderInput1f> spotExposure =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogExposure"));
  ref_ptr<ShaderInput2f> spotRadiusScale =
      ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogRadiusScale"));
  ref_ptr<ShaderInput2f> spotConeScale =
      ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogConeScale"));
  spotExposure->setUniformData(0.45);
  spotRadiusScale->setUniformData(Vec2f(1.0));
  spotConeScale->setUniformData(Vec2f(1.0));
  app->addShaderInput(spotExposure, 0.0, 10.0, 2);
  app->addShaderInput(spotRadiusScale, 0.0, 10.0, 2);
  app->addShaderInput(spotConeScale, 0.0, 10.0, 2);
  volumeFog->addLight(spotLight,
      spotExposure, spotRadiusScale, spotConeScale);

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gBufferState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), gBufferState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
