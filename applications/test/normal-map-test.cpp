

#include "factory.h"
using namespace ogle;

#define USE_HUD

static const string transferTBNNormal =
    "void transferTBNNormal(inout vec4 texel) {\n"
    "#if SHADER_STAGE==fs\n"
    "    mat3 tbn = mat3(in_tangent,in_binormal,in_norWorld);"
    "    texel.xyz = normalize( tbn * ( texel.xyz*2.0 - vec3(1.0) ) );\n"
    "#endif\n"
    "}";
const string transferBrickHeight =
    "void transferBrickHeight(inout vec4 texel) { texel.x = texel.x*0.05 - 0.05; }";

class NormalMapLoader : public EventHandler, public Animation
{
public:
  enum NormalMapMode
  {
    NM_MODE_NONE,
    NM_MODE_NORMAL_MAPPING,
    NM_MODE_PARALLAX_MAPPING,
    NM_MODE_PARALLAX_OCCLUSION_MAPPING,
    NM_MODE_RELIEF_MAPPING,
    NM_MODE_TESSELATION,
    NM_MODE_LAST
  };

  NormalMapLoader(FltkApplication *app, const ref_ptr<StateNode> &root)
  : EventHandler(), Animation(), app_(app)
  {
    rotateEnabled_ = GL_TRUE;
    rotation_ = Mat4f::identity();

    Box::Config meshCfg;
    meshCfg.texcoMode = Box::TEXCO_MODE_UV;
    meshCfg.isNormalRequired = GL_TRUE;
    meshCfg.isTangentRequired = GL_TRUE;
    meshCfg.posScale = Vec3f(0.25f);
    meshCfg.texcoScale = Vec2f(1.0f);
    mesh_ = ref_ptr<Mesh>::manage(new Box(meshCfg));

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

    setMode(NM_MODE_PARALLAX_MAPPING);
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

    colorMapState_ = ref_ptr<TextureState>::manage(new TextureState(colMap_, "colorTexture"));
    colorMapState_->set_blendMode(BLEND_MODE_SRC);
    colorMapState_->set_mapTo(MAP_TO_COLOR);
    switch(mode) {
    case NM_MODE_PARALLAX_OCCLUSION_MAPPING:
      colorMapState_->set_texcoTransfer(TRANSFER_TEXCO_PARALLAX_OCC);
      break;
    case NM_MODE_PARALLAX_MAPPING:
      colorMapState_->set_texcoTransfer(TRANSFER_TEXCO_PARALLAX);
      break;
    case NM_MODE_RELIEF_MAPPING:
      colorMapState_->set_texcoTransfer(TRANSFER_TEXCO_RELIEF);
      break;
    default:
      break;
    }
    modelMat_->joinStates(ref_ptr<State>::cast(colorMapState_));

    if(mode != NM_MODE_NONE) {
      normalMapState_ = ref_ptr<TextureState>::manage(new TextureState(norMap_, "normalTexture"));
      normalMapState_->set_blendMode(BLEND_MODE_SRC);
      normalMapState_->set_mapTo(MAP_TO_NORMAL);
      normalMapState_->set_texelTransferFunction(transferTBNNormal, "transferTBNNormal");
      if(mode == NM_MODE_PARALLAX_MAPPING) {
        normalMapState_->set_texcoTransfer(TRANSFER_TEXCO_PARALLAX);
      }
      if(mode == NM_MODE_PARALLAX_OCCLUSION_MAPPING) {
        normalMapState_->set_texcoTransfer(TRANSFER_TEXCO_PARALLAX_OCC);
      }
      if(mode == NM_MODE_RELIEF_MAPPING) {
        normalMapState_->set_texcoTransfer(TRANSFER_TEXCO_RELIEF);
      }
      modelMat_->joinStates(ref_ptr<State>::cast(normalMapState_));
    }

    if(mode > NM_MODE_NORMAL_MAPPING) {
      heightMapState_ = ref_ptr<TextureState>::manage(new TextureState(heightMap_, "heightTexture"));
      if(mode == NM_MODE_TESSELATION) {
        heightMapState_->set_blendMode(BLEND_MODE_ADD);
        heightMapState_->set_mapTo(MAP_TO_HEIGHT);
        heightMapState_->set_texelTransferFunction(transferBrickHeight, "transferBrickHeight");
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
    case NM_MODE_PARALLAX_OCCLUSION_MAPPING:
      app_->set_windowTitle("Parallax Occlusion Mapping");
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
  virtual void glAnimate(RenderState *rs, GLdouble dt)
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
  FltkApplication *app_;

  ref_ptr<Mesh> mesh_;
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
  ref_ptr<QtApplication> app = initApplication(argc,argv,"Normal Mapping");

  // create a root node for everything that needs camera as input
  ref_ptr<Camera> cam = createPerspectiveCamera(app.get());
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
  app->connect(OGLEApplication::KEY_EVENT, ref_ptr<EventHandler>::cast(nmLoader));

  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gBufferState->fbo(), sceneRoot);

  ref_ptr<SpotLight> spotLight = createSpotLight(app.get());
  spotLight->set_position(Vec3f(0.0f,2.0f,0.0f));
  spotLight->set_spotDirection(Vec3f(0.0f,-1.0f,0.0f));
  spotLight->set_diffuse(Vec3f(0.5f,0.5f,0.5f));
  spotLight->set_specular(Vec3f(0.0f));
  spotLight->set_innerRadius(1.0);
  spotLight->set_outerRadius(4.0);
  spotLight->coneAngle()->setVertex2f(0, Vec2f(0.98, 0.9));
  //ref_ptr<SpotShadowMap> spotShadow = createSpotShadow(app.get(), spotLight, cam);
  //spotShadow->addCaster(gBufferNode);
  //deferredShading->addLight(spotLight, spotShadow);
  deferredShading->addLight(ref_ptr<Light>::cast(spotLight));

#if 1
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
#endif

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
