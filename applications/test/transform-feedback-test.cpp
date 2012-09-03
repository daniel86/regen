
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/states/debug-normal.h>

#include <applications/glut-render-tree.h>

class PickEventHandler : public EventCallable
{
public:
  PickEventHandler(
      MeshState *pickable,
      Material *mat)
  : EventCallable(),
    pickable_(pickable),
    mat_(mat),
    isPicked_(GL_FALSE)
  {}
  virtual void call(EventObject *evObject, void *data)
  {
    Picker::PickEvent *ev = (Picker::PickEvent*)data;

    if(isPicked_)
    {
      mat_->set_chrome();
    }

    if(ev->state == pickable_)
    {
      mat_->set_gold();
      isPicked_ = GL_TRUE;
    }
    else
    {
      isPicked_ = GL_FALSE;
    }
  }
  MeshState *pickable_;
  Material *mat_;
  GLboolean isPicked_;
};

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Transform Feedback");

  ref_ptr<FBOState> fboState = application->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      // with sky box there is no need to clear the color buffer
      GL_FALSE,
      Vec4f(0.0f)
  );

  ref_ptr<Light> &light = application->setLight();
  light->setConstantUniforms(GL_TRUE);

  ref_ptr<Picker> picker = application->usePicking();

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    ref_ptr<MeshState> sphereState =
        ref_ptr<MeshState>::manage(new UnitSphere(sphereConfig));

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.5f, 0.0f, 0.0f), 0.0f);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<ShaderInput> posAtt_ = ref_ptr<ShaderInput>::manage(
        new ShaderInput4f( "Position" ));
    sphereState->setTransformFeedbackAttribute(posAtt_);

    ref_ptr<ShaderInput> norAtt_ = ref_ptr<ShaderInput>::manage(
        new ShaderInput3f( ATTRIBUTE_NAME_NOR ));
    sphereState->setTransformFeedbackAttribute(norAtt_);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading(Material::PHONG_SHADING);
    material->set_chrome();

    ref_ptr<PickEventHandler> pickHandler = ref_ptr<PickEventHandler>::manage(
        new PickEventHandler(sphereState.get(), material.get()));
    picker->connect(Picker::PICK_EVENT, ref_ptr<EventCallable>::cast(pickHandler));

    ref_ptr<StateNode> meshNode = application->addMesh(sphereState, modelMat, material);


    ref_ptr<StateNode> &tfParent = application->perspectivePass();
    map< string, ref_ptr<ShaderInput> > tfInputs =
        application->renderTree()->collectParentInputs(*tfParent.get());

    ref_ptr<TFMeshState> tfState =
        ref_ptr<TFMeshState>::manage(new TFMeshState(sphereState));
    tfState->joinStates(ref_ptr<State>::manage(
        new DebugNormal(tfInputs, GS_INPUT_TRIANGLES, 0.1)));

    ref_ptr<StateNode> tfNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(tfState)));
    application->renderTree()->addChild(tfParent, tfNode);
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  application->addSkyBox("res/textures/cube-stormydays.jpg");
  application->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  application->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  application->mainLoop();
  return 0;
}
