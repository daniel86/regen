
#include <regen/utility/filesystem.h>
#include <regen/bullet/bullet-physics.h>
#include "../test/factory.h"
using namespace regen;

#define USE_HUD
#define USE_FXAA
#define USE_SPHERE_MESH

#define RANDOM (rand()%100)/100.0f
#define SPHERE_RADI_COUNT 10

void createWall(
    QtApplication *app,
    const ref_ptr<StateNode> &root,
    const GLfloat &height,
    const Vec3f &posScale)
{
  Rectangle::Config meshCfg;
  meshCfg.levelOfDetail = 0;
  meshCfg.isTexcoRequired = GL_FALSE;
  meshCfg.isNormalRequired = GL_TRUE;
  meshCfg.isTangentRequired = GL_FALSE;
  meshCfg.centerAtOrigin = GL_TRUE;
  meshCfg.rotation = Vec3f(0.0f*M_PI, 0.0f*M_PI, 1.0f*M_PI);
  meshCfg.posScale = posScale;
  ref_ptr<Mesh> floor = ref_ptr<Rectangle>::alloc(meshCfg);

  ref_ptr<Material> material = ref_ptr<Material>::alloc();
  material->set_ruby();
  material->diffuse()->setUniformData(Vec3f(0.8f));
  material->specular()->setUniformData(Vec3f(0.0f));
  material->setConstantUniforms(GL_TRUE);
  floor->joinStates(material);

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  floor->joinStates(shaderState);

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(floor);
  root->addChild(meshNode);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderState->createShader(shaderConfigurer.cfg(), "regen.meshes.mesh");
  floor->initializeResources(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());
}

class BulletSpheres {
public:

  class KeyHandler : public EventHandler
  {
  public:
    BulletSpheres *spheres_;
    KeyHandler(BulletSpheres *spheres) : spheres_(spheres) {}

    void call(EventObject *emitter, EventData *data)
    {
      if(data->eventID == Application::KEY_EVENT) {
        Application::KeyEvent *ev = (Application::KeyEvent*)data;
        if(!ev->isUp) return;

        if(ev->key==' ') {
          spheres_->_spawnSphere();
        }
      }
    }
  };

  BulletSpheres(int argc, char** argv) {
    Vec3f dir(-4.0f,0.0f,-1.0f);
    dir.normalize();

    app_ = initApplication(argc,argv);
    app_->connect(Application::KEY_EVENT, ref_ptr<KeyHandler>::alloc(this));

    physics_ = ref_ptr<BulletPhysics>::alloc();

    cam_ = createPerspectiveCamera(app_.get());
    cam_->position()->setVertex(0, Vec3f(16.0f,0.5f,7.0f));
    cam_->direction()->setVertex(0, dir);

    manipulator_ = createLookAtCameraManipulator(app_.get(), cam_);
    manipulator_->set_lookAt(Vec3f(0.0f,2.5f,0.0f));
    manipulator_->set_height(1.75f,0.0);
    manipulator_->set_radius(10.0f,0.0);
    manipulator_->set_degree(2.2,0.0);
    manipulator_->setStepLength(0.001f,0.0);

    createRenderTree();
  }

  int mainLoop() {
    return app_->mainLoop();
  }

  void createRenderTree() {
    // Create scene root node (camera is activated at root node)
    rootNode_ = ref_ptr<StateNode>::alloc(cam_);
    app_->renderTree()->addChild(rootNode_);

    // Create a geometry buffer render target for unshaded objects
    // and add meshes to it.
    _createGeometryPass();
    // Compute shading after geometry was updated in the G-Buffer
    _createShadingPass();
    // Render the background, add sun light
    _createBackgroundPass();
    // FXAA pass
    _createPostPasses();
    // Default HUD (FPS+Logo)
    _createHUD();

    // blit result to screen
    setBlitToScreen(app_.get(),
        gTargetState_->fbo(),
        gDiffuseTexture_,
        GL_COLOR_ATTACHMENT0);

    for(int i=0; i<20; ++i) _spawnSphere();
  }

  void _createGeometryPass() {
    gTargetState_ = createGBuffer(app_.get());
    gDiffuseTexture_ = gTargetState_->fbo()->colorTextures()[0];
    gDepthTexture_   = gTargetState_->fbo()->depthTexture();

    ref_ptr<StateNode> gTargetNode = ref_ptr<StateNode>::alloc(gTargetState_);
    gBufferNode_ = ref_ptr<StateNode>::alloc();

    rootNode_->addChild(gTargetNode);
    gTargetNode->addChild(gBufferNode_);

    {
      // add bullet floor
      GLfloat floorHeight = 0.0f;
      physics_->addObject(PhysicalObject::createInfiniteWall(floorHeight));
      physics_->addWall(5.0f, 5.0f);
      // add graphics floor
      createWall(app_.get(), gBufferNode_, floorHeight, Vec3f(5.0f));
    }

    for(int i=0; i<SPHERE_RADI_COUNT; ++i) {
      sphereRadi_[i] = 0.15f + ((GLfloat)i)*0.01f;

#ifdef USE_SPHERE_MESH
      Sphere::Config sphereConfig;
      sphereConfig.posScale = Vec3f(2.0*sphereRadi_[i]);
      sphereConfig.texcoMode = Sphere::TEXCO_MODE_NONE;
      sphereMeshes_[i] = ref_ptr<Sphere>::alloc(sphereConfig);
#else
      SphereSprite::Config sphereConfig;
      GLfloat radi[] = { 2.0f*sphereRadi_[i] };
      Vec3f pos[] = { Vec3f(0.0f) };
      sphereConfig.radius = radi;
      sphereConfig.position = pos;
      sphereConfig.sphereCount = 1;
      sphereMeshes_[i] = ref_ptr<SphereSprite>::alloc(sphereConfig);
#endif
    }

    sphereShaderState_ = ref_ptr<ShaderState>::alloc();
    sphereNode_ = ref_ptr<StateNode>::alloc(sphereShaderState_);
    gBufferNode_->addChild(sphereNode_);

    sphereNodeConfig_.addNode(sphereNode_.get());
    sphereNodeConfig_.define("HAS_modelMatrix", "TRUE");
    sphereNodeConfig_.define("HAS_nor", "TRUE");
    sphereNodeConfig_.define("HAS_MATERIAL", "TRUE");
    sphereNodeConfig_.define("SHADING", "deferred");

#ifdef USE_SPHERE_MESH
    sphereShaderState_->createShader(sphereNodeConfig_.cfg(), "regen.meshes.mesh");
#else
    sphereShaderState_->createShader(sphereNodeConfig_.cfg(), "regen.meshes.sprite-sphere");
#endif
  }

  void _spawnSphere() {
    Vec3f spherePosition(
        RANDOM*1.0f, 3.0f + RANDOM*3.0f, RANDOM*1.0f);
    GLint index = (int)(RANDOM*SPHERE_RADI_COUNT);
    GLfloat sphereRadius = sphereRadi_[index];
    ref_ptr<Mesh> meshData = sphereMeshes_[index];

    Sphere::Config sphereConfig;
    sphereConfig.posScale = Vec3f(2.0*sphereRadius);
    sphereConfig.texcoMode = Sphere::TEXCO_MODE_NONE;
#ifdef USE_SPHERE_MESH
    ref_ptr<Mesh> mesh = ref_ptr<Mesh>::alloc(GL_TRIANGLES, meshData->inputContainer());
#else
    ref_ptr<Mesh> mesh = ref_ptr<Mesh>::alloc(GL_POINTS, meshData->inputContainer());
#endif

    ref_ptr<Material> material = ref_ptr<Material>::alloc();
    material->set_pewter();
    material->diffuse()->setUniformData(Vec3f(RANDOM,RANDOM,RANDOM));
    mesh->joinStates(material);

    ref_ptr<ModelTransformation> modelMat = ref_ptr<ModelTransformation>::alloc();
    modelMat->translate(spherePosition, 0.0f);
    mesh->joinStates(modelMat);

    mesh->initializeResources(RenderState::get(),
        sphereNodeConfig_.cfg(), sphereShaderState_->shader());

    // create a mesh node and add it to the render tree
    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::alloc(mesh);
    sphereNode_->addChild(meshNode);

    // Create a physical sphere
    GLfloat sphereMass = 1.0f;
    // Synchronize ModelTransformation and physics simulation.
    ref_ptr<ModelMatrixMotion> motion = ref_ptr<ModelMatrixMotion>::alloc(btTransform(
        btQuaternion(0,0,0,1),
        btVector3(spherePosition.x,spherePosition.y,spherePosition.z)),
        modelMat);
    ref_ptr<PhysicalObject> physicalObject = PhysicalObject::createSphere(
        motion,
        sphereRadius,
        sphereMass);
    physics_->addObject(physicalObject);
  }

  void _createShadingPass() {
    const GLboolean useAmbientLight = GL_TRUE;

    deferredShading_ = createShadingPass(
        app_.get(),
        gTargetState_->fbo(),
        gBufferNode_,
        ShadowMap::FILTERING_PCF_GAUSSIAN,
        useAmbientLight);
    deferredShading_->ambientLight()->setVertex(0,Vec3f(0.2f));
    deferredShading_->dirShadowState()->setShadowLayer(3);
  }

  void _createBackgroundPass() {
    // create root node for background rendering, draw ontop gDiffuseTexture
    ref_ptr<StateNode> backgroundNode = createBackground(
        app_.get(),
        gTargetState_->fbo(),
        gDiffuseTexture_,
        GL_COLOR_ATTACHMENT0);
    rootNode_->addChild(backgroundNode);
    // add a sky box
    sky_ = createSky(app_.get(), backgroundNode);
    sky_->sun()->specular()->setUniformData(Vec3f(0.4f));

#if 1
    // create sun shadow
    {
      ShadowMap::Config sunShadowCfg;
      sunShadowCfg.size = 2048;
      sunShadowCfg.depthFormat = GL_DEPTH_COMPONENT24;
      sunShadowCfg.depthType = GL_FLOAT;
      sunShadowCfg.numLayer = 3;
      sunShadowCfg.splitWeight = 0.8;

      ref_ptr<ShadowMap> sunShadow = createShadow(
          app_.get(), sky_->sun(), cam_, sunShadowCfg);
      // all meshes added to GBuffer node casting shadows
      sunShadow->addCaster(gBufferNode_);
      deferredShading_->addLight(sky_->sun(), sunShadow);
    }
#else
    deferredShading_->addLight(sky_->sun());
#endif
  }

  void _createPostPasses() {
    ref_ptr<StateNode> postPassNode = createPostPassNode(
        app_.get(),
        gTargetState_->fbo(),
        gDiffuseTexture_,
        GL_COLOR_ATTACHMENT0);
    rootNode_->addChild(postPassNode);

  #ifdef USE_FXAA
    ref_ptr<FullscreenPass> aa = createAAState(
        app_.get(),
        gDiffuseTexture_,
        postPassNode);
    aa->joinStatesFront(ref_ptr<DrawBufferUpdate>::alloc(
        gTargetState_->fbo(),
        gDiffuseTexture_,
        GL_COLOR_ATTACHMENT0));
  #endif
  }

  void _createHUD() {
    // create HUD with FPS text, draw ontop gDiffuseTexture
    ref_ptr<StateNode> guiNode = createHUD(
        app_.get(),
        gTargetState_->fbo(),
        gDiffuseTexture_,
        GL_COLOR_ATTACHMENT0);
    app_->renderTree()->addChild(guiNode);
    createLogoWidget(app_.get(), guiNode);
    createFPSWidget(app_.get(), guiNode);
  }

protected:
  ref_ptr<Mesh> sphereMeshes_[SPHERE_RADI_COUNT];
  GLfloat sphereRadi_[SPHERE_RADI_COUNT];

  ref_ptr<QtApplication> app_;
  ref_ptr<BulletPhysics> physics_;
  // camera provides view and projection matrix
  ref_ptr<Camera> cam_;
  // camera animator
  ref_ptr<LookAtCameraManipulator> manipulator_;

  ref_ptr<FBOState> gTargetState_;
  ref_ptr<Texture> gDiffuseTexture_;
  ref_ptr<Texture> gDepthTexture_;
  ref_ptr<DeferredShading> deferredShading_;
  ref_ptr<SkyScattering> sky_;

  ref_ptr<StateNode> rootNode_;
  ref_ptr<StateNode> gBufferNode_;
  ref_ptr<StateNode> sphereNode_;
  StateConfigurer sphereNodeConfig_;
  ref_ptr<ShaderState> sphereShaderState_;
};

int main(int argc, char** argv)
{
  BulletSpheres spheres(argc,argv);
  return spheres.mainLoop();
}
