
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/states/debug-normal.h>

#include <applications/glut-render-tree.h>

int main(int argc, char** argv)
{
  GlutRenderTree *application = new GlutRenderTree(argc, argv, "Transform feedback test");

  application->setClearScreenColor(Vec4f(0.10045f, 0.0056f, 0.012f, 1.0f));
  application->globalStates()->state()->addEnabler(
      ref_ptr<Callable>::manage(new ClearDepthState));

  application->setLight();

  ref_ptr<ModelTransformationState> modelMat;

  {
    UnitSphere::Config sphereConfig;
    sphereConfig.texcoMode = UnitSphere::TEXCO_MODE_NONE;
    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->translate(Vec3f(0.5f, 0.0f, 0.0f), 0.0f);
    ref_ptr<AttributeState> sphereState =
        ref_ptr<AttributeState>::manage(new UnitSphere(sphereConfig));

    ref_ptr<VertexAttribute> posAtt_ = ref_ptr<VertexAttribute>::manage(
        new VertexAttributefv( "Position", 4 ));
    sphereState->setTransformFeedbackAttribute(posAtt_);

    //ref_ptr<VertexAttribute> norAtt_ = ref_ptr<VertexAttribute>::manage(
    //    new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
    //sphereState->setTransformFeedbackAttribute(norAtt_);

    ref_ptr<StateNode> meshNode = application->addMesh(sphereState, modelMat);

    /*
    ref_ptr<TFAttributeState> tfState =
        ref_ptr<TFAttributeState>::manage(new TFAttributeState(sphereState));
    tfState->joinStates(ref_ptr<State>::manage(
        new DebugNormal(GS_INPUT_TRIANGLES, 0.1)));
    ref_ptr<StateNode> tfNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(tfState)));
    application->renderTree()->addChild(meshNode, tfNode);
    */
  }

  //application->setShowFPS();

  application->mainLoop();
  return 0;
}
