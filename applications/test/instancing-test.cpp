
#include <regen/utility/filesystem.h>
#include "factory.h"
using namespace regen;

#define USE_SKY
#define USE_HUD
//#define USE_PICKING
#define USE_AMBIENT_OCCLUSION
#define USE_ANIMATION

// Loads Meshes from File using Assimp. Optionally Bone animations are loaded.
list<MeshData> createAssimpMeshInstanced(
    Application *app,
    const ref_ptr<StateNode> &root,
    const string &modelFile,
    const string &texturePath,
    const Mat4f &meshRotation,
    const Vec3f &meshTranslation,
    const BoneAnimRange *animRanges=NULL,
    GLuint numAnimationRanges=0,
    GLdouble ticksPerSecond=20.0)
{
#define RANDOM (rand()%100)/100.0f

  const GLuint numInstancesX = 10;
  const GLuint numInstancesY = 10;
  const GLuint numInstances = numInstancesX*numInstancesY;
  // two instances play the same animation
  const GLint numInstancedAnimations = numInstances/2;

  // import file
  AssimpImporter importer(modelFile, texturePath);

  ref_ptr<ModelTransformation> modelMat =
      createInstancedModelMat(numInstancesX, numInstancesY, 8.0);
  // defines offset to matrix tbo for each instance
  GLint *boneOffset = new int[numInstances];
  for(GLuint i=0; i<numInstances; ++i) boneOffset[i] = numInstancedAnimations*RANDOM;

  // load meshes
  list< ref_ptr<Mesh> > meshes;
  importer.loadMeshes(Mat4f::identity(), VertexBufferObject::USAGE_DYNAMIC, meshes);
  // load node animations, copy the animation for each different animation that
  // should be played by different instances
  list<NodeAnimation*> instanceAnimations;
  NodeAnimation *boneAnim = importer.loadNodeAnimation(
      GL_TRUE,
      NodeAnimation::BEHAVIOR_LINEAR,
      NodeAnimation::BEHAVIOR_LINEAR,
      ticksPerSecond);
  instanceAnimations.push_back(boneAnim);
  boneAnim->stopAnimation();
  for(GLint i=1; i<numInstancedAnimations; ++i) {
    NodeAnimation *anim = boneAnim->copy();
    anim->stopAnimation();
    instanceAnimations.push_back(anim);
  }

  list<MeshData> ret;

  for(list< ref_ptr<Mesh> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<Mesh> &mesh = *it;

    mesh->joinStates(importer.getMeshMaterial(mesh.get()));
    mesh->joinStates(modelMat);

#ifdef USE_ANIMATION
    if(boneAnim) {
      list< ref_ptr<AnimationNode> > meshBones;
      GLuint boneCount = 0;
      for(list<NodeAnimation*>::iterator
          it=instanceAnimations.begin(); it!=instanceAnimations.end(); ++it)
      {
        list< ref_ptr<AnimationNode> > ibonNodes = importer.loadMeshBones(mesh.get(), *it);
        boneCount = ibonNodes.size();
        meshBones.insert(meshBones.end(), ibonNodes.begin(), ibonNodes.end() );
      }
      GLuint numBoneWeights = importer.numBoneWeights(mesh.get());

      ref_ptr<Bones> bonesState = ref_ptr<Bones>::manage(
          new Bones(meshBones, numBoneWeights));
      mesh->joinStates(bonesState);

      // defines offset to matrix tbo for each instance
      ref_ptr<ShaderInput1f> u_boneOffset =
          ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("boneOffset"));
      u_boneOffset->setInstanceData(numInstances, 1, NULL);
      GLfloat *boneOffset_ = (GLfloat*)u_boneOffset->dataPtr();
      for(GLuint i=0; i<numInstances; ++i) boneOffset_[i] = boneCount*boneOffset[i];
      mesh->setInput(u_boneOffset);
    }
#endif

    ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
    mesh->joinStates(shaderState);

    ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(new StateNode(mesh));
    root->addChild(meshNode);

    StateConfigurer shaderConfigurer;
    shaderConfigurer.addNode(meshNode.get());
    shaderState->createShader(shaderConfigurer.cfg(), "mesh");
    mesh->updateVAO(RenderState::get(), shaderConfigurer.cfg(), shaderState->shader());

    MeshData d;
    d.mesh_ = mesh;
    d.shader_ = shaderState;
    d.node_ = meshNode;
    ret.push_back(d);
  }

  for(list<NodeAnimation*>::iterator
      it=instanceAnimations.begin(); it!=instanceAnimations.end(); ++it)
  {
    ref_ptr<NodeAnimation> anim = ref_ptr<NodeAnimation>::manage(*it);
    ref_ptr<EventHandler> animStopped = ref_ptr<EventHandler>::manage(
        new AnimationRangeUpdater(anim, animRanges,numAnimationRanges));
    anim->connect(NodeAnimation::ANIMATION_STOPPED, animStopped);
    anim->startAnimation();

    EventData evData;
    evData.eventID = NodeAnimation::ANIMATION_STOPPED;
    animStopped->call(anim.get(), &evData);
  }

  return ret;
#undef RANDOM
}

int main(int argc, char** argv)
{
  static const string assimpMeshFile = filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/models/psionic/dwarf/x/dwarf.x");
  static const string assimpMeshTexturesPath = filesystemPath(
      REGEN_SOURCE_DIR, "applications/res/models/psionic/dwarf/x");
  static const BoneAnimRange animRanges[] = {
      BoneAnimRange("none",        Vec2d(  -1.0,  -1.0 )),
      BoneAnimRange("complete",    Vec2d(   0.0, 361.0 )),
      BoneAnimRange("run",         Vec2d(  16.0,  26.0 )),
      BoneAnimRange("jump",        Vec2d(  28.0,  40.0 )),
      BoneAnimRange("jumpSpot",    Vec2d(  42.0,  54.0 )),
      BoneAnimRange("crouch",      Vec2d(  56.0,  59.0 )),
      BoneAnimRange("crouchLoop",  Vec2d(  60.0,  69.0 )),
      BoneAnimRange("getUp",       Vec2d(  70.0,  74.0 )),
      BoneAnimRange("battleIdle1", Vec2d(  75.0,  88.0 )),
      BoneAnimRange("battleIdle2", Vec2d(  90.0, 110.0 )),
      BoneAnimRange("attack1",     Vec2d( 112.0, 126.0 )),
      BoneAnimRange("attack2",     Vec2d( 128.0, 142.0 )),
      BoneAnimRange("attack3",     Vec2d( 144.0, 160.0 )),
      BoneAnimRange("attack4",     Vec2d( 162.0, 180.0 )),
      BoneAnimRange("attack5",     Vec2d( 182.0, 192.0 )),
      BoneAnimRange("block",       Vec2d( 194.0, 210.0 )),
      BoneAnimRange("dieFwd",      Vec2d( 212.0, 227.0 )),
      BoneAnimRange("dieBack",     Vec2d( 230.0, 251.0 )),
      BoneAnimRange("yes",         Vec2d( 253.0, 272.0 )),
      BoneAnimRange("no",          Vec2d( 274.0, 290.0 )),
      BoneAnimRange("idle1",       Vec2d( 292.0, 325.0 )),
      BoneAnimRange("idle2",       Vec2d( 327.0, 360.0 ))
  };

  ref_ptr<QtApplication> app = initApplication(argc,argv);

  // create a root node for everything that needs camera as input
  ref_ptr<Camera> cam = createPerspectiveCamera(app.get());
  cam->position()->setVertex3f(0, Vec3f(0.0f,3.0f,-42.0f));
  cam->direction()->setVertex3f(0, Vec3f(0.0f,0.0f,1.0f));
  ref_ptr<EgoCameraManipulator> manipulator = createEgoCameraManipulator(app.get(), cam);

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(new StateNode(cam));
  app->renderTree()->addChild(sceneRoot);

  ref_ptr<FBOState> gTargetState = createGBuffer(app.get());
  ref_ptr<StateNode> gTargetNode = ref_ptr<StateNode>::manage(new StateNode(gTargetState));
  sceneRoot->addChild(gTargetNode);
  ref_ptr<Texture> gDiffuseTexture = gTargetState->fbo()->colorBuffer()[0];
  ref_ptr<Texture> gSpecularTexture = gTargetState->fbo()->colorBuffer()[2];
  ref_ptr<Texture> gNorWorldTexture = gTargetState->fbo()->colorBuffer()[3];
  ref_ptr<Texture> gDepthTexture = gTargetState->fbo()->depthTexture();

  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage(new StateNode);
  gTargetNode->addChild(gBufferNode);
  list<MeshData> dwarf = createAssimpMeshInstanced(
        app.get(), gBufferNode
      , assimpMeshFile
      , assimpMeshTexturesPath
      , Mat4f::rotationMatrix(0.0f,M_PI,0.0f)
      , Vec3f(0.0f,-2.0f,0.0f)
      , animRanges, sizeof(animRanges)/sizeof(BoneAnimRange)
  );
  MeshData floor = createFloorMesh(app.get(), gBufferNode, 0.0f, Vec3f(100.0f), Vec2f(20.0f));

  const GLboolean useAmbientLight = GL_TRUE;
  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gTargetState->fbo(), sceneRoot,
      ShadowMap::FILTERING_NONE,
      useAmbientLight);
  deferredShading->dirShadowState()->setShadowLayer(3);
  deferredShading->ambientLight()->setVertex3f(0,Vec3f(0.2f));

#ifdef USE_AMBIENT_OCCLUSION
  deferredShading->setUseAmbientOcclusion();
  const ref_ptr<AmbientOcclusion> ao = deferredShading->ambientOcclusion();

  ao->aoAttenuation()->setVertex2f(0, Vec2f(0.1,0.2));
  ao->aoBias()->setVertex1f(0, 0.28);
  ao->aoSamplingRadius()->setVertex1f(0, 50.0);
  ao->blurSigma()->setVertex1f(0, 3.0);
  ao->blurNumPixels()->setVertex1i(0, 3);

  app->addShaderInput("Light.AmbientOcclusion",
      ao->aoAttenuation(),
      Vec4f(0.0f), Vec4f(9.0f), Vec4i(2),
      "similar to how lights are attenuated.");
  app->addShaderInput("Light.AmbientOcclusion",
      ao->aoBias(),
      Vec4f(0.0f), Vec4f(1.0f), Vec4i(2),
      "");
  app->addShaderInput("Light.AmbientOcclusion",
      ao->aoSamplingRadius(),
      Vec4f(0.0f), Vec4f(99.0f), Vec4i(2),
      "");
  app->addShaderInput("Light.AmbientOcclusion",
      ao->blurSigma(),
      Vec4f(0.0f), Vec4f(99.0f), Vec4i(2),
      "");
  app->addShaderInput("Light.AmbientOcclusion",
      ao->blurNumPixels(),
      Vec4f(0.0f), Vec4f(99.0f), Vec4i(0),
      "width/height of blur kernel.");
#endif

  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);

#ifdef USE_SKY
  // add a sky box
  ref_ptr<SkyScattering> sky = createSky(app.get(), backgroundNode);
  //sky->setMars();
  sky->setEarth();
  ShadowMap::Config sunShadowCfg; {
    sunShadowCfg.size = 1024;
    sunShadowCfg.depthFormat = GL_DEPTH_COMPONENT24; // GL_DEPTH_COMPONENT16
    sunShadowCfg.depthType = GL_FLOAT; // GL_UNSIGNED_BYTE
    sunShadowCfg.numLayer = 3;
    sunShadowCfg.splitWeight = 0.5;
  }
  ref_ptr<ShadowMap> sunShadow = createShadow(
      app.get(), sky->sun(), cam, sunShadowCfg);
  sunShadow->addCaster(gBufferNode);
  deferredShading->addLight(sky->sun(), sunShadow);

  ref_ptr<DistanceFog> dfog = createDistanceFog(app.get(), Vec3f(1.0f),
      sky->cubeMap(), gDepthTexture, backgroundNode);
#endif

  ref_ptr<StateNode> postPassNode = createPostPassNode(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(postPassNode);

  //ref_ptr<SkyLightShaft> sunRay = createSkyLightShaft(
  //    app.get(), sky->sun(), gDiffuseTexture, gDepthTexture, postPassNode);
  //sunRay->joinStatesFront(ref_ptr<State>::manage(new DrawBufferTex(
  //    gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));

  ref_ptr<FilterSequence> blur = createBlurState(
      app.get(), gDiffuseTexture, backgroundNode, 4, 2.0);
  blur->joinStatesFront(ref_ptr<State>::manage(new TexturePingPong(gDiffuseTexture)));
  ref_ptr<Texture> blurTexture = blur->output();

  ref_ptr<DepthOfField> dof =
      createDoFState(app.get(), gDiffuseTexture, blurTexture, gDepthTexture, backgroundNode);
  dof->joinStatesFront(ref_ptr<State>::manage(new DrawBufferUpdate(
      gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0)));

  ref_ptr<FullscreenPass> aa = createAAState(
      app.get(), gDiffuseTexture, postPassNode);
  aa->joinStatesFront(ref_ptr<State>::manage(new DrawBufferUpdate(
      gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0)));

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createLogoWidget(app.get(), guiNode);
  createFPSWidget(app.get(), guiNode);
#ifdef USE_AMBIENT_OCCLUSION
  //createTextureWidget(app.get(), guiNode,
  //    ao->aoTexture(), Vec2ui(50u,50u), 200.0f);
#endif
#endif

#ifdef USE_PICKING
  PickingGeom *picker = createPicker(app.get(), gDepthTexture);
  picker->add(floor.mesh_, floor.node_, floor.shader_->shader());
  for(list<MeshData>::iterator it=dwarf.begin(); it!=dwarf.end(); ++it) {
    picker->add(it->mesh_, it->node_, it->shader_->shader());
  }
#endif

  setBlitToScreen(app.get(), gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
