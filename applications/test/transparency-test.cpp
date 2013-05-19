
#include <regen/utility/filesystem.h>
#include "factory.h"
using namespace regen;

#define USE_SPOT_LIGHT
#define USE_HUD
#define USE_FXAA
#define USE_PARTICLE_FOG
#define USE_FOG_BLUR
#define USE_SHADOW
#define USE_VENUS
#define USE_PLATFORM

PickingGeom *picker;

#ifdef USE_PLATFORM
void createBox(QtApplication *app,
    const ref_ptr<StateNode> &root,
    const Vec3f &position,
    const Vec3f &scale,
    const Mat4f &rotation)
{
  Box::Config cfg;
  cfg.isNormalRequired = GL_TRUE;
  cfg.posScale = scale;
  ref_ptr<Mesh> mesh = ref_ptr<Box>::alloc(cfg);

  ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
  modelMat->set_modelMat(rotation,0.0f);
  modelMat->translate(position, 0.0f);
  mesh->joinStates(modelMat);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  material->set_silver();
  mesh->joinStates(material);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  mesh->joinStates(shaderState);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "regen.meshes.mesh");
  mesh->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());
#ifdef USE_PICKING
  picker->add(mesh, meshNode, shaderState->shader());
#endif
}
#endif

int main(int argc, char** argv)
{
  const TBuffer::Mode alphaMode = TBuffer::MODE_BACK_TO_FRONT;
  const Vec3f centerTranslations[3] = {
      Vec3f(-0.35f, 0.0f,-1.365) ,
      Vec3f(  2.0f, 0.0f, 0.0f) ,
      Vec3f(-0.35f, 0.0f, 1.365)
  };
#ifdef USE_VENUS
  const string assimpMeshFile = filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/models/venusm.obj");
  const string assimpMeshTexturesPath = filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/models/venusm.obj");
  const Mat4f venusRotations[3] = {
      Mat4f::rotationMatrix(0.0f,0.3f*M_PI,0.0f),
      Mat4f::rotationMatrix(0.0f,0.5f*M_PI,0.0f),
      Mat4f::rotationMatrix(0.0f,1.6f*M_PI,0.0f)
  };
  const Vec3f venusTranslations[3] = {
      centerTranslations[0] + Vec3f(0.2,0.0,0.2),
      centerTranslations[1] + Vec3f(0.3,0.0,0.0),
      centerTranslations[2] + Vec3f(-0.2,0.0,0.2)
  };
  const GLfloat venusAlpha[3] = { 0.99, 0.9, 0.85 };
#endif
#ifdef USE_PLATFORM
  const Mat4f platformRotations[3] = {
      Mat4f::rotationMatrix(0.0f,(30.0/360.0)*2.0*M_PI,0.0f),
      Mat4f::rotationMatrix(0.0f,0.0f*M_PI,0.0f),
      Mat4f::rotationMatrix(0.0f,2.0*M_PI-(30.0/360.0)*2.0*M_PI,0.0f)
  };
  const Vec3f platformTranslations[3] = {
      centerTranslations[0] + Vec3f(0.0,-100.6,0.0),
      centerTranslations[1] + Vec3f(0.0,-100.6,0.0),
      centerTranslations[2] + Vec3f(0.0,-100.6,0.0)
  };
#endif

  ref_ptr<QtApplication> app = initApplication(argc,argv);

  // create a root node for everything that needs camera as input
  ref_ptr<Camera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 3.0f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 9.0f );
  manipulator->set_degree( M_PI*0.1 );
  manipulator->setStepLength( M_PI*0.001 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::alloc(cam);
  app->renderTree()->addChild(sceneRoot);

  ref_ptr<Light> spotLight = createSpotLight(app.get());
  spotLight->specular()->setVertex(0,Vec3f(0.0));
  spotLight->diffuse()->setVertex(0,Vec3f(0.58,0.58,0.28));
  spotLight->position()->setVertex(0,Vec3f(5.0,6.0,0.0));
  spotLight->direction()->setVertex(0,Vec3f(-0.5,-0.6,0.0));
  spotLight->radius()->setVertex(0,Vec2f(9.0,14.0));
  spotLight->coneAngle()->setVertex(0, Vec2f(0.9,0.8));
#ifdef USE_SHADOW
  ShadowMap::Config spotShadowCfg; {
    spotShadowCfg.size = 512;
    spotShadowCfg.depthFormat = GL_DEPTH_COMPONENT24;
    spotShadowCfg.depthType = GL_UNSIGNED_BYTE;
  }
  ref_ptr<ShadowMap> spotShadow = createShadow(app.get(), spotLight, cam, spotShadowCfg);
  ShadowMap::FilterMode spotShadowFilter = ShadowMap::FILTERING_VSM;
  if(ShadowMap::useShadowMoments(spotShadowFilter)) {
    spotShadow->setComputeMoments();
    spotShadow->setCullFrontFaces(GL_FALSE);
    spotShadow->createBlurFilter(4, 2.0, GL_FALSE);

    app->addShaderInput("Shadow.Shadow0.Blur",
        spotShadow->momentsBlurSigma(),
        Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
        "Number of samples for moment blur.");
    app->addShaderInput("Shadow.Shadow0.Blur",
        spotShadow->momentsBlurSize(),
        Vec4f(0.0f), Vec4f(50.0f), Vec4i(2),
        "Blur sigma for moments blur.");
  }
#endif

  ref_ptr<FBOState> gTargetState = createGBuffer(app.get());
  ref_ptr<StateNode> gTargetNode = ref_ptr<StateNode>::alloc(gTargetState);
  sceneRoot->addChild(gTargetNode);
  ref_ptr<Texture> gDiffuseTexture = gTargetState->fbo()->colorTextures()[0];
  ref_ptr<Texture> gDepthTexture = gTargetState->fbo()->depthTexture();

  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::alloc();
  gTargetNode->addChild(gBufferNode);
#ifdef USE_SHADOW
  spotShadow->addCaster(gBufferNode);
#endif
#ifdef USE_PLATFORM
  createBox(app.get(), gBufferNode,
      platformTranslations[0], Vec3f(1.0f, 100.0f, 1.0f), platformRotations[0]);
  createBox(app.get(), gBufferNode,
      platformTranslations[1], Vec3f(1.0f, 100.0f, 1.0f), platformRotations[1]);
  createBox(app.get(), gBufferNode,
      platformTranslations[2], Vec3f(1.0f, 100.0f, 1.0f), platformRotations[2]);
#endif

  ref_ptr<TBuffer> tTargetState = createTBuffer(app.get(), cam, gDepthTexture, alphaMode);
  ref_ptr<StateNode> tTargetNode = ref_ptr<StateNode>::alloc(tTargetState);
  sceneRoot->addChild(tTargetNode);

  ref_ptr<StateNode> tBufferNode = ref_ptr<StateNode>::alloc();
  switch(alphaMode) {
  case TBuffer::MODE_BACK_TO_FRONT:
    tTargetState->joinStatesFront(ref_ptr<SortByModelMatrix>::alloc(tBufferNode, cam, GL_FALSE));
    break;
  case TBuffer::MODE_FRONT_TO_BACK:
    tTargetState->joinStatesFront(ref_ptr<SortByModelMatrix>::alloc(tBufferNode, cam, GL_TRUE));
    break;
  default:
    break;
  }
  // TBuffer uses direct lighting
  tTargetNode->addChild(tBufferNode);
#ifdef USE_SHADOW
  tTargetState->addLight(spotLight, spotShadow, spotShadowFilter);
  spotShadow->addCaster(tBufferNode);
#else
  tTargetState->addLight(spotLight);
#endif
#ifdef USE_VENUS
  Mat4f scaleModel = Mat4f::identity();
  scaleModel.scale(Vec3f(0.0008f));
  for(int i=0; i<3; ++i)
  {
    list<MeshData> imported = createAssimpMesh(
              app.get(), tBufferNode
            , assimpMeshFile
            , assimpMeshTexturesPath
            , venusRotations[i]
            , venusTranslations[i]
            , scaleModel
            , NULL, 0, 20.0
            , "regen.meshes.transparent-mesh"
        );
    MeshData &venusMesh = *imported.begin();
    if(i==0)      venusMesh.material_->set_jade();
    else if(i==1) venusMesh.material_->set_ruby();
    else if(i==2) venusMesh.material_->set_copper();
    venusMesh.material_->alpha()->setUniformData(venusAlpha[i]);
    //venusMesh.material_->set_twoSided(GL_TRUE);
  }
#endif

#ifdef USE_SHADOW
  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gTargetState->fbo(), sceneRoot, spotShadowFilter);
  deferredShading->addLight(spotLight, spotShadow);
  {
    const ref_ptr<FilterSequence> &momentsFilter = spotShadow->momentsFilter();
    StateConfigurer _cfg;
    _cfg.addNode(sceneRoot.get());
    _cfg.addState(momentsFilter.get());
    momentsFilter->createShader(_cfg.cfg());
  }
#else
  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gTargetState->fbo(), sceneRoot);
  deferredShading->addLight(spotLight);
#endif

  deferredShading->ambientLight()->setVertex(0,Vec3f(0.2f));
  tTargetState->ambientLight()->setVertex(0,Vec3f(0.2f));

  ref_ptr<FBOState> postPassState = ref_ptr<FBOState>::alloc(gTargetState->fbo());
  ref_ptr<StateNode> postPassNode = ref_ptr<StateNode>::alloc(postPassState);
  sceneRoot->addChild(postPassNode);

  // Combine TBuffer and shaded GBuffer
  ref_ptr<FullscreenPass> resolveAlpha = ref_ptr<FullscreenPass>::alloc("regen.utility.sampling");
  {
    resolveAlpha->shaderDefine("IS_2D_TEXTURE","TRUE");
    resolveAlpha->joinStatesFront(ref_ptr<TextureState>::alloc(tTargetState->colorTexture(), "in_inputTexture"));
    resolveAlpha->joinStatesFront(ref_ptr<BlendState>::alloc(BLEND_MODE_ALPHA));
    resolveAlpha->joinStatesFront(ref_ptr<DrawBufferOntop>::alloc(
        gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0));
    ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
    depth->set_useDepthTest(GL_FALSE);
    depth->set_useDepthWrite(GL_FALSE);
    resolveAlpha->joinStatesFront(depth);

    ref_ptr<StateNode> n = ref_ptr<StateNode>::alloc(resolveAlpha);
    postPassNode->addChild(n);

    StateConfigurer shaderConfigurer;
    shaderConfigurer.addNode(n.get());
    resolveAlpha->createShader(shaderConfigurer.cfg());
  }

#ifdef USE_PARTICLE_FOG
  ref_ptr<FBO> particleFBO = ref_ptr<FBO>::alloc(256, 256, 1);
  ref_ptr<Texture> particleTex = particleFBO->addTexture(
      1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA16F, GL_FLOAT);

  ref_ptr<FBOState> particleBuffer = ref_ptr<FBOState>::alloc(particleFBO);
  ClearColorState::Data clearData;
  clearData.clearColor = Vec4f(0.0f);
  clearData.colorBuffers.buffers_.push_back(GL_COLOR_ATTACHMENT0);
  particleBuffer->setClearColor(clearData);
  particleBuffer->addDrawBuffer(GL_COLOR_ATTACHMENT0);

  ref_ptr<DirectShading> directShading = ref_ptr<DirectShading>::alloc();
  directShading->addLight(spotLight);
  particleBuffer->joinStates(directShading);

  ref_ptr<StateNode> directShadingNode = ref_ptr<StateNode>::alloc(particleBuffer);
  postPassNode->addChild(directShadingNode);

  ref_ptr<ParticleSnow> fogParticles = createParticleFog(
      app.get(), gDepthTexture, directShadingNode, 500);
#ifdef USE_FOG_BLUR
  ref_ptr<ShaderInput1i> blurSize = ref_ptr<ShaderInput1i>::alloc("numBlurPixels");
  blurSize->setUniformData(9);
  ref_ptr<ShaderInput1f> blurSigma = ref_ptr<ShaderInput1f>::alloc("blurSigma");
  blurSigma->setUniformData(2.3);
  app->addShaderInput("Particles.Fog.Draw",
      blurSize,
      Vec4f(0.0f), Vec4f(100.0f), Vec4i(0),
      "Width and height of blur kernel.");
  app->addShaderInput("Particles.Fog.Draw",
      blurSigma,
      Vec4f(0.0f), Vec4f(50.0f), Vec4i(2),
      "Blur sigma.");

  ref_ptr<FilterSequence> filter = ref_ptr<FilterSequence>::alloc(particleTex);
  filter->joinShaderInput(blurSize);
  filter->joinShaderInput(blurSigma);
  filter->addFilter(ref_ptr<Filter>::alloc("regen.post-passes.blur.horizontal"));
  filter->addFilter(ref_ptr<Filter>::alloc("regen.post-passes.blur.vertical"));

  ref_ptr<StateNode> blurNode = ref_ptr<StateNode>::alloc(filter);
  postPassNode->addChild(blurNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(blurNode.get());
  filter->createShader(shaderConfigurer.cfg());
#endif

  // Combine FOG with rest of the scene
  ref_ptr<FullscreenPass> combineParticles = ref_ptr<FullscreenPass>::alloc("regen.utility.sampling");
  {
    combineParticles->shaderDefine("IS_2D_TEXTURE","TRUE");
#ifdef USE_FOG_BLUR
    combineParticles->joinStatesFront(ref_ptr<TextureState>::alloc(filter->output(), "in_inputTexture"));
#else
    combineParticles->joinStatesFront(ref_ptr<State>::manage(
        new TextureState(particleTex, "in_inputTexture")));
#endif
    combineParticles->joinStatesFront(ref_ptr<BlendState>::alloc(BLEND_MODE_ADD));
    combineParticles->joinStatesFront(ref_ptr<DrawBufferOntop>::alloc(
        gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0));
    ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
    depth->set_useDepthTest(GL_FALSE);
    depth->set_useDepthWrite(GL_FALSE);
    combineParticles->joinStatesFront(depth);

    ref_ptr<StateNode> n = ref_ptr<StateNode>::alloc(combineParticles);
    postPassNode->addChild(n);

    StateConfigurer shaderConfigurer;
    shaderConfigurer.addNode(n.get());
    combineParticles->createShader(shaderConfigurer.cfg());
  }
#endif

#ifdef USE_FXAA
  ref_ptr<FullscreenPass> aa = createAAState(
      app.get(), gDiffuseTexture, postPassNode);
  aa->joinStatesFront(ref_ptr<DrawBufferUpdate>::alloc(
      gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0));
#endif

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createLogoWidget(app.get(), guiNode);
  createFPSWidget(app.get(), guiNode);
  //createTextureWidget(app.get(), guiNode,
  //    spotShadow->shadowMomentsUnfiltered(), Vec2ui(50u,0u), 200.0f);
  //createTextureWidget(app.get(), guiNode,
  //    spotShadow->shadowMoments(), Vec2ui(450u,0u), 200.0f);
#endif

  setBlitToScreen(app.get(), gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  //setBlitToScreen(app.get(), tBufferState->fboState()->fbo(), GL_COLOR_ATTACHMENT0);
  //setBlitToScreen(app.get(), particleFBO, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
