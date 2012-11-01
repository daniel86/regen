
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/models/quad.h>
#include <ogle/textures/video-texture.h>
#include <ogle/animations/animation-manager.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

class PickRotation : public Animation
{
public:
  PickRotation(ShaderInputMat4 *modelMat)
  : Animation(),
    modelMat_(modelMat),
    instanceIndex_(0)
  {
  }
  void set_instanceIndex(GLint instanceIndex)
  {
    instanceIndex_ = instanceIndex;
  }
  virtual void animate(GLdouble dt)
  {
  }
  virtual void updateGraphics(GLdouble dt)
  {
    // update model matrix
    Mat4f &modelMat = modelMat_->getVertex16f(instanceIndex_);
    modelMat = xyzRotationMatrix(0.0, dt*0.01, 0.0) * modelMat;
    // copy single matrix to VBO
    glBindBuffer(GL_ARRAY_BUFFER, modelMat_->buffer());
    glBufferSubData(GL_ARRAY_BUFFER,
        modelMat_->offset() + instanceIndex_*modelMat_->elementSize(),
        modelMat_->elementSize(),
        &modelMat);
  }
private:
  ShaderInputMat4 *modelMat_;
  GLint instanceIndex_;
};
class PickEventHandler : public EventCallable
{
public:
  PickEventHandler(
      MeshState *pickable,
      PickRotation *anim)
  : EventCallable(),
    pickable_(pickable),
    anim_(anim)
  {}
  virtual void call(EventObject *evObject, void *data)
  {
    Picker::PickEvent *ev = (Picker::PickEvent*)data;
    if(ev->state == pickable_)
    {
      anim_->set_instanceIndex(ev->instanceId);
    }
  }
  MeshState *pickable_;
  PickRotation *anim_;
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("Instancing Test");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_TRUE,
      Vec4f(0.5f,0.5f,0.5f,1.0f)
  );

  ref_ptr<Light> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

  camManipulator->setStepLength(0.005f, 0.0f);
  camManipulator->set_height(20.0f, 0.0f);
  camManipulator->set_radius(30.0f, 0.0f);

  ref_ptr<Picker> picker = renderTree->usePicking();

  {
    GLuint numInstancesX = 30;
    GLuint numInstancesY = 30;
    GLuint numInstances = numInstancesX*numInstancesY;
    float instanceDistance = 2.25;

    UnitCube::Config cubeConfig;
    cubeConfig.texcoMode = UnitCube::TEXCO_MODE_NONE;
    ref_ptr<MeshState> cube = ref_ptr<MeshState>::manage(new UnitCube(cubeConfig));

    ref_ptr<ModelTransformationState> modelMat =
        ref_ptr<ModelTransformationState>::manage(new ModelTransformationState);
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
            Vec3f(0.0f , 2.0f*RANDOM*M_PI, 0.0f),
            translation + Vec3f(1.5f*(0.5f-RANDOM),0.0f,1.25f*(0.5f-RANDOM)),
            Vec3f(1.0f,1.0f,1.0f)
#undef RANDOM
        ));
        i += 1;
      }
      translation.z = bufY;
    }
    modelMat->modelMat()->setInstanceData(
        numInstances, 1, (byte*)instancedModelMat);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_pewter();
    material->set_shading(Material::GOURAD_SHADING);
    material->setConstantUniforms(GL_TRUE);

    Vec4f *diffuse = new Vec4f[numInstances];
    for(unsigned int x=0; x<numInstances; ++x) {
#define RANDOM (rand()%100)/100.0f
      diffuse[x] = Vec4f(RANDOM,RANDOM,RANDOM,0.0f);
#undef RANDOM
    }
    material->set_diffuse( numInstances, 1, diffuse );

    ref_ptr<PickRotation> pickAnim = ref_ptr<PickRotation>::manage(
        new PickRotation(modelMat->modelMat()));
    ref_ptr<PickEventHandler> pickHandler = ref_ptr<PickEventHandler>::manage(
        new PickEventHandler(cube.get(), pickAnim.get()));
    picker->connect(Picker::PICK_EVENT, ref_ptr<EventCallable>::cast(pickHandler));
    AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(pickAnim));

    renderTree->addMesh(cube, modelMat, material);
  }

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
