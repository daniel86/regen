
#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/textures/video-texture.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/states/assimp-importer.h>
#include <ogle/shadows/directional-shadow-map.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

class AnimStoppedHandler : public EventCallable
{
public:
  map< string, Vec2d > animationRanges_;
  AnimStoppedHandler(const map< string, Vec2d > &animationRanges)
  : EventCallable(),
    animationRanges_(animationRanges)
  {
  }
  void call(EventObject *ev, void *data)
  {
    NodeAnimation *anim = (NodeAnimation*)ev;

    GLint i = rand()%animationRanges_.size();
    GLint index = 0;
    for(map< string, Vec2d >::iterator
        it=animationRanges_.begin(); it!=animationRanges_.end(); ++it)
    {
      if(index==i) {
        anim->setAnimationIndexActive(0,
            it->second + Vec2d(-1.0, -1.0) );
        break;
      }
      ++index;
    }
  }
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;
  const GLuint shadowMapSize = 512;
  const GLenum internalFormat = GL_DEPTH_COMPONENT16;
  const GLenum pixelType = GL_BYTE;
  const GLfloat shadowSplitWeight = 0.7;
  ShadowMap::FilterMode sunShadowFilter = ShadowMap::SINGLE;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("Instancing Test");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  camManipulator->setStepLength(0.0f, 0.0f);
  camManipulator->set_degree(M_PI,0.0f);
  camManipulator->set_height(30.0f, 0.0f);
  camManipulator->set_radius(40.0f, 0.0f);

  ref_ptr<PerspectiveCamera> &sceneCamera = renderTree->perspectiveCamera();
  ref_ptr<Frustum> sceneFrustum = ref_ptr<Frustum>::manage(new Frustum);
  sceneFrustum->setProjection(
      sceneCamera->fovUniform()->getVertex1f(0),
      sceneCamera->aspect(),
      sceneCamera->nearUniform()->getVertex1f(0),
      sceneCamera->farUniform()->getVertex1f(0));

  ref_ptr<DirectionalLight> sunLight =
      ref_ptr<DirectionalLight>::manage(new DirectionalLight);
  sunLight->set_direction(Vec3f(1.0f,1.0f,-1.0f));
  sunLight->set_ambient(Vec3f(0.15f));
  sunLight->set_diffuse(Vec3f(0.35f));
  renderTree->setLight(ref_ptr<Light>::cast(sunLight));
  // add shadow maps to the sun light
  ref_ptr<DirectionalShadowMap> sunShadow = ref_ptr<DirectionalShadowMap>::manage(
      new DirectionalShadowMap(sunLight, sceneFrustum, sceneCamera,
          shadowMapSize, shadowSplitWeight, internalFormat, pixelType));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(sunShadow));
  sunShadow->set_filteringMode(sunShadowFilter);

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.7f,0.6f,0.5f,0.0f)
  );

  srand(time(0));

  GLuint numInstancesX = 10;
  GLuint numInstancesY = 10;
  GLuint numInstances = numInstancesX*numInstancesY;
  float instanceDistance = 8.0;
  int numInstancedAnimations = 50;
  float f_numInstancedAnimations = (float)numInstancedAnimations;

  Mat4f* instancedModelMat = new Mat4f[numInstances];
  float bufX = -0.5f*numInstancesX*instanceDistance -0.5f;
  float bufY = -0.5f*numInstancesY*instanceDistance -0.5f;
  Vec3f translation = (Vec3f) { bufX, 0.0f, bufY };
  unsigned int i = 0;
  for(unsigned int x=0; x<numInstancesX; ++x) {
    translation.x += instanceDistance;
    for(unsigned int y=0; y<numInstancesY; ++y) {
      translation.z += instanceDistance;
#define RANDOM (rand()%100)/100.0f
      instancedModelMat[i] = transpose(transformMatrix(
          //Vec3f(0.0f , 2.0f*RANDOM*M_PI, 0.0f),
          Vec3f(0.0f , M_PI, 0.0f),
          translation + Vec3f(1.5f*(0.5f-RANDOM),0.0f,1.25f*(0.5f-RANDOM)),
          Vec3f(1.0f,1.0f,1.0f)
#undef RANDOM
      ));
      i += 1;
    }
    translation.z = bufY;
  }
  // defines offset to matrix tbo for each instance
  int *boneOffset = new int[numInstances];
  for(unsigned int x=0; x<numInstances; ++x) {
#define RANDOM (rand()%100)/100.0f
    boneOffset[x] = (int)(f_numInstancedAnimations*RANDOM);
#undef RANDOM
  }

  // mapping from different types of animations
  // to matching ticks
  map< string, Vec2d > animationRanges;
  animationRanges["none"] = Vec2d( -1.0, -1.0 );
  animationRanges["complete"] = Vec2d( 0.0, 361.0 );
  animationRanges["run"] = Vec2d( 16.0, 26.0 );
  animationRanges["jump"] = Vec2d( 28.0, 40.0 );
  animationRanges["jumpSpot"] = Vec2d( 42.0, 54.0 );
  animationRanges["crouch"] = Vec2d( 56.0, 59.0 );
  animationRanges["crouchLoop"] = Vec2d( 60.0, 69.0 );
  animationRanges["getUp"] = Vec2d( 70.0, 74.0 );
  animationRanges["battleIdle1"] = Vec2d( 75.0, 88.0 );
  animationRanges["battleIdle2"] = Vec2d( 90.0, 110.0 );
  animationRanges["attack1"] = Vec2d( 112.0, 126.0 );
  animationRanges["attack2"] = Vec2d( 128.0, 142.0 );
  animationRanges["attack3"] = Vec2d( 144.0, 160.0 );
  animationRanges["attack4"] = Vec2d( 162.0, 180.0 );
  animationRanges["attack5"] = Vec2d( 182.0, 192.0 );
  animationRanges["block"] = Vec2d( 194.0, 210.0 );
  animationRanges["dieFwd"] = Vec2d( 212.0, 227.0 );
  animationRanges["dieBack"] = Vec2d( 230.0, 251.0 );
  animationRanges["yes"] = Vec2d( 253.0, 272.0 );
  animationRanges["no"] = Vec2d( 274.0, 290.0 );
  animationRanges["idle1"] = Vec2d( 292.0, 325.0 );
  animationRanges["idle2"] = Vec2d( 327.0, 360.0 );
  bool forceChannelStates=true;
  AnimationBehaviour forcedPostState = ANIM_BEHAVIOR_LINEAR;
  AnimationBehaviour forcedPreState = ANIM_BEHAVIOR_LINEAR;
  double defaultTicksPerSecond=20.0;

  string modelPath = "res/models/psionic/dwarf/x";
  string modelName = "dwarf2.x";
  AssimpImporter importer(modelPath + "/" + modelName, modelPath);
  list< ref_ptr<MeshState> > meshes = importer.loadMeshes();

  ref_ptr<NodeAnimation> boneAnim = importer.loadNodeAnimation(
      forceChannelStates,
      forcedPostState,
      forcedPreState,
      defaultTicksPerSecond);
  boneAnim->set_timeFactor(1.0);
  list< ref_ptr<NodeAnimation> > instancedAnims;
  instancedAnims.push_back(boneAnim);
  for(int i=1; i<numInstancedAnimations; ++i) {
    instancedAnims.push_back(boneAnim->copy());
  }

  for(list< ref_ptr<MeshState> >::iterator
      it=meshes.begin(); it!=meshes.end(); ++it)
  {
    ref_ptr<MeshState> &mesh = *it;

    ref_ptr<ModelTransformationState> modelMat =
        ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
    modelMat->modelMat()->setInstanceData(numInstances, 1, (byte*)instancedModelMat);

    ref_ptr<Material> material = importer.getMeshMaterial(mesh.get());
    material->setConstantUniforms(GL_TRUE);

    list< ref_ptr<AnimationNode> > bonNodes;
    GLuint boneCount = 0;
    for(list< ref_ptr<NodeAnimation> >::iterator
        it=instancedAnims.begin(); it!=instancedAnims.end(); ++it)
    {
      list< ref_ptr<AnimationNode> > ibonNodes =
          importer.loadMeshBones(mesh.get(), it->get());
      boneCount = ibonNodes.size();
      bonNodes.insert(bonNodes.end(), ibonNodes.begin(), ibonNodes.end() );
    }
    GLuint numBoneWeights = importer.numBoneWeights(mesh.get());
    if(bonNodes.size()==0) {
      WARN_LOG("No bones state!");
    }
    else {
      ref_ptr<BonesState> bonesState = ref_ptr<BonesState>::manage(
          new BonesState(bonNodes, numBoneWeights));
      modelMat->joinStates(ref_ptr<State>::cast(bonesState));
      AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(bonesState));

      float *boneOffset_ = new float[numInstances];
      for(unsigned int x=0; x<numInstances; ++x) {
        boneOffset_[x] = (float) boneCount*boneOffset[x];
      }
      ref_ptr<ShaderInput1f> u_boneOffset = ref_ptr<ShaderInput1f>::manage(
          new ShaderInput1f("boneOffset"));
      u_boneOffset->setInstanceData(numInstances, 1, (byte*)boneOffset_);
      delete []boneOffset_;
      modelMat->joinShaderInput(ref_ptr<ShaderInput>::cast(u_boneOffset));
    }

    renderTree->addMesh(mesh, modelMat, material);
  }
  {
    Rectangle::Config quadConfig;
    quadConfig.levelOfDetail = 0;
    quadConfig.isTexcoRequired = GL_TRUE;
    quadConfig.isNormalRequired = GL_TRUE;
    quadConfig.centerAtOrigin = GL_TRUE;
    quadConfig.rotation = Vec3f(0.0*M_PI, 0.0*M_PI, 1.0*M_PI);
    quadConfig.posScale = Vec3f(100.0f);
    quadConfig.texcoScale = Vec2f(5.0f);
    ref_ptr<MeshState> quad =
        ref_ptr<MeshState>::manage(new Rectangle(quadConfig));

    ref_ptr<ModelTransformationState> modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_ambient(Vec3f(0.3f));
    material->set_diffuse(Vec3f(0.7f));
    material->setConstantUniforms(GL_TRUE);

    renderTree->addMesh(quad, modelMat, material);
  }

  for(list< ref_ptr<NodeAnimation> >::iterator
      it=instancedAnims.begin(); it!=instancedAnims.end(); ++it)
  {
    ref_ptr<NodeAnimation> &anim = *it;
    ref_ptr<EventCallable> animStopped = ref_ptr<EventCallable>::manage(
        new AnimStoppedHandler(animationRanges) );
    anim->connect( NodeAnimation::ANIMATION_STOPPED, animStopped );
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(anim));
    animStopped->call(anim.get(), NULL);
  }

  sunShadow->addCaster(renderTree->perspectivePass());

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  return application->mainLoop();
}
