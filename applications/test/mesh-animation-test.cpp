
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/animations/mesh-animation.h>
#include <ogle/states/assimp-importer.h>
#include <ogle/animations/animation-manager.h>

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
  AnimStoppedHandler()
  : EventCallable()
  {
  }
  void call(EventObject *ev, void *data)
  {
    MeshAnimation *meshAnim = (MeshAnimation*)ev;
    meshAnim->setTickRange(Vec2d(0.0,7.5));
  }
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("VBO Animation");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  renderTree->setLight();

  ref_ptr<ModelTransformationState> modelMat;

  {
    string modelPath = "res/models/apple.obj";
    string texturePath = "res/textures";

    AssimpImporter importer(modelPath, texturePath);

    aiMatrix4x4 transform, translate;
    aiMatrix4x4::Scaling(aiVector3D(0.02,0.02,0.02), transform);
    aiMatrix4x4::Translation(aiVector3D(-1.25f, -1.0f, 0.0f), translate);
    transform = translate * transform;

    list< ref_ptr<MeshState> > meshes = importer.loadMeshes(transform);

    Mat4f transformation = identity4f();
    ref_ptr<ModelTransformationState> modelMat;
    ref_ptr<Material> material;

    for(list< ref_ptr<MeshState> >::iterator
        it=meshes.begin(); it!=meshes.end(); ++it)
    {
      ref_ptr<MeshState> mesh = *it;

      material = importer.getMeshMaterial(mesh.get());
      material->set_shading(Material::PHONG_SHADING);
      material->set_specular( Vec4f(0.0f) );
      material->setConstantUniforms(GL_TRUE);

      modelMat = ref_ptr<ModelTransformationState>::manage(
          new ModelTransformationState);
      modelMat->set_modelMat(transformation, 0.0f);
      modelMat->translate(Vec3f(-1.25f, 0.0f, 0.0f), 0.0f);
      modelMat->setConstantUniforms(GL_TRUE);

      ref_ptr<StateNode> meshNode = renderTree->addMesh(mesh, modelMat, material);

      ref_ptr<MeshAnimation> meshAnim = ref_ptr<MeshAnimation>::manage(new MeshAnimation(mesh));
      ref_ptr<VertexInterpolator> interpolator = ref_ptr<VertexInterpolator>::manage(
          new OscillateVertexInterpolator);

      MeshKeyFrame sphereFrame;
      sphereFrame.timeInTicks = 2.5;
      sphereFrame.interpolator = interpolator;
      meshAnim->addSphereAttributes(sphereFrame, 0.5, 0.5);
      meshAnim->addFrame(sphereFrame);

      MeshKeyFrame boxFrame;
      boxFrame.timeInTicks = 2.5;
      boxFrame.interpolator = interpolator;
      meshAnim->addBoxAttributes(boxFrame, 1.0, 1.0, 1.0);
      meshAnim->addFrame(boxFrame);

      MeshKeyFrame modelFrame;
      modelFrame.timeInTicks = 2.5;
      modelFrame.interpolator = interpolator;
      meshAnim->addMeshAttribute(modelFrame, ATTRIBUTE_NAME_POS);
      meshAnim->addMeshAttribute(modelFrame, ATTRIBUTE_NAME_NOR);
      meshAnim->addFrame(modelFrame);

      AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(meshAnim));

      ref_ptr<EventCallable> animStopped = ref_ptr<EventCallable>::manage( new AnimStoppedHandler );
      meshAnim->connect( MeshAnimation::ANIMATION_STOPPED, animStopped );
      animStopped->call(meshAnim.get(), NULL);
    }
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  renderTree->addSkyBox("res/textures/cube-grimmnight.jpg");
  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
