
#include "factory.h"

#define FRAME_TIME 0.5
// #define DEBUG_NORMAL
#define USE_HUD
#define USE_SKY
#define USE_LIGHT_SHAFTS

class AnimStoppedHandler : public EventCallable
{
public:
  AnimStoppedHandler()
  : EventCallable()
  {
  }
  void call(EventObject *ev, void *data)
  {
    MeshAnimation *meshAnim = (MeshAnimation*)ev;
    meshAnim->setTickRange(Vec2d(0.0,3.0*FRAME_TIME));
  }
};

// Loads Meshes from File using Assimp. Optionally Bone animations are loaded.
list< ref_ptr<MeshState> > createAssimpMesh(
    OGLEApplication *app,
    const ref_ptr<StateNode> &root,
    const string &modelFile,
    const string &texturePath,
    const Mat4f &meshRotation,
    const Vec3f &meshTranslation,
    const aiMatrix4x4 &importTransformation=aiMatrix4x4())
{
  AssimpImporter importer(modelFile, texturePath);
  list< ref_ptr<MeshState> > meshes = importer.loadMeshes(importTransformation);

  for(list< ref_ptr<MeshState> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<MeshState> &mesh = *it;

    ref_ptr<Material> material = importer.getMeshMaterial(mesh.get());
    material->setConstantUniforms(GL_TRUE);
    mesh->joinStates(ref_ptr<State>::cast(material));

    ref_ptr<ModelTransformation> modelMat =
        ref_ptr<ModelTransformation>::manage(new ModelTransformation);
    modelMat->set_modelMat(meshRotation, 0.0f);
    modelMat->translate(meshTranslation, 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);
    mesh->joinStates(ref_ptr<State>::cast(modelMat));

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh->joinStates(ref_ptr<State>::cast(shaderState));

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(mesh)));
    root->addChild(meshNode);

    ShaderConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), "mesh");
  }

  return meshes;
}

void createMeshAnimation(
    OGLEFltkApplication *app,
    list< ref_ptr<MeshState> > &meshes)
{
  list<AnimInterpoation> interpolations;
  interpolations.push_back(AnimInterpoation("pos","interpolate_elastic"));
  interpolations.push_back(AnimInterpoation("nor","interpolate_elastic"));

  ref_ptr<ShaderInput1f> friction =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("friction"));
  friction->setUniformData(2.2f);
  app->addShaderInput(friction,0.0f,10.0f,4);

  ref_ptr<ShaderInput1f> frequency =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("frequency"));
  frequency->setUniformData(0.25f);
  app->addShaderInput(frequency,0.0f,10.0f,4);

  list< ref_ptr<MeshAnimation> > anims;
  for(list< ref_ptr<MeshState> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<MeshState> mesh = *it;
    ref_ptr<MeshAnimation> meshAnim =
        ref_ptr<MeshAnimation>::manage(new MeshAnimation(mesh, interpolations));
    meshAnim->interpolationShader()->setInput(ref_ptr<ShaderInput>::cast(friction));
    meshAnim->interpolationShader()->setInput(ref_ptr<ShaderInput>::cast(frequency));
    meshAnim->addSphereAttributes(0.5, 0.5, FRAME_TIME);
    meshAnim->addBoxAttributes(1.0, 1.0, 1.0, FRAME_TIME);
    meshAnim->addMeshFrame(FRAME_TIME);
    anims.push_back(meshAnim);

    ref_ptr<EventCallable> animStopped = ref_ptr<EventCallable>::manage( new AnimStoppedHandler );
    meshAnim->connect( MeshAnimation::ANIMATION_STOPPED, animStopped );
    animStopped->call(meshAnim.get(), NULL);
  }
  for(list< ref_ptr<MeshAnimation> >::iterator
      it=anims.begin(); it!=anims.end(); ++it)
  {
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(*it));
  }
}

int main(int argc, char** argv)
{
  static const string assimpMeshFile = "res/models/apple.obj";
  static const string assimpMeshTexturesPath = "res/textures";

  ref_ptr<OGLEFltkApplication> app = initApplication(argc,argv,"VBO Animation");

  // create a root node for everything that needs camera as input
  ref_ptr<PerspectiveCamera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 0.2f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 4.0f );
  manipulator->set_degree( M_PI*1.0 );
  manipulator->setStepLength( M_PI*0.0 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get());
  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gBufferState)));
  ref_ptr<Texture> gDiffuseTexture = gBufferState->fbo()->colorBuffer()[0];
  ref_ptr<Texture> gDepthTexture = gBufferState->fbo()->depthTexture();
  sceneRoot->addChild(gBufferNode);

  aiMatrix4x4 transform, translate;
  aiMatrix4x4::Scaling(aiVector3D(0.02,0.02,0.02), transform);
  aiMatrix4x4::Translation(aiVector3D(-1.25f, -1.0f, 0.0f), translate);
  transform = translate * transform;
  list< ref_ptr<MeshState> > meshes = createAssimpMesh(
        app.get(), gBufferNode
      , assimpMeshFile
      , assimpMeshTexturesPath
      , Mat4f::identity()
      , Vec3f(-0.5f,0.0f,0.0f)
      , transform
  );
  createMeshAnimation(app.get(), meshes);

  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gBufferState->fbo(), sceneRoot);

  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);
#ifdef USE_SKY
  // add a sky box
  ref_ptr<DynamicSky> sky = createSky(app.get(), backgroundNode);
  deferredShading->addLight(sky->sun());
#endif

#ifdef USE_LIGHT_SHAFTS
  ref_ptr<StateNode> postPassNode = createPostPassNode(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(postPassNode);
  ref_ptr<SkyLightShaft> sunRay = createSkyLightShaft(
      app.get(), sky->sun(), gDiffuseTexture, gDepthTexture, postPassNode);
  sunRay->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
      gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));
#endif

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), gBufferState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
