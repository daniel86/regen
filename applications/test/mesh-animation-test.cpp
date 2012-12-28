
#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/animations/mesh-animation.h>
#include <ogle/states/assimp-importer.h>
#include <ogle/states/shader-state.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/string-util.h>

#include <applications/application-config.h>
#include <applications/fltk-ogle-application.h>

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

#define FRAME_TIME 0.5
#define DEBUG_NORMAL

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

#ifdef DEBUG_NORMAL
/**
 * For child models only the normals are rendered.
 * Transform feedback is used so that the transformations
 * must not be done again.
 * You have to add 'Position' and 'nor' as transform
 * feedback attribute for children.
 */
class DebugNormal : public ShaderState
{
public:
  DebugNormal(
      map<string, ref_ptr<ShaderInput> > &inputs,
      GLfloat normalLength=0.1)
  : ShaderState()
  {
    map<string,string> shaderConfig;
    shaderConfig["GS_INPUT_PRIMITIVE"] = "triangles";
    shaderConfig["NORMAL_LENGTH"] = FORMAT_STRING(normalLength);
    // configuration using macros
    map<GLenum,string> shaderNames;
    shaderNames[GL_FRAGMENT_SHADER] = "debug-normal.fs";
    shaderNames[GL_VERTEX_SHADER]   = "debug-normal.vs";
    shaderNames[GL_GEOMETRY_SHADER] = "debug-normal.gs";

    map<string,string> functions;
    shader_ = Shader::create(shaderConfig, functions, inputs, shaderNames);
    if(shader_->compile() && shader_->link()) {
      shader_->setInputs(inputs);
    } else {
      shader_ = ref_ptr<Shader>();
    }
  }
  // override
  virtual void enable(RenderState *state) {
    glDepthFunc(GL_LEQUAL);
    ShaderState::enable(state);
  }
  virtual void disable(RenderState *state) {
    ShaderState::disable(state);
    glDepthFunc(GL_LESS);
  }
};
#endif

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
  application->set_windowTitle("VBO Animation");
  application->show();

  ref_ptr<TestCamManipulator> camManipulator = ref_ptr<TestCamManipulator>::manage(
      new TestCamManipulator(*application, renderTree->perspectiveCamera()));
  camManipulator->setStepLength(0.0f,0.0f);
  camManipulator->set_degree(100.0f,0.0f);
  camManipulator->set_height(1.0f,0.0f);
  camManipulator->set_radius(2.5f, 0.0f);
  AnimationManager::get().addAnimation(ref_ptr<Animation>::cast(camManipulator));

  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f,
      GL_RGBA,
      GL_DEPTH_COMPONENT24,
      GL_TRUE,
      GL_FALSE,
      Vec4f(0.0f)
  );

  renderTree->addDynamicSky();

  ref_ptr<ShaderInput1f> friction =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("friction"));
  friction->setUniformData(2.2f);
  application->addShaderInput(friction,0.0f,10.0f,4);

  ref_ptr<ShaderInput1f> frequency =
      ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("frequency"));
  frequency->setUniformData(0.25f);
  application->addShaderInput(frequency,0.0f,10.0f,4);

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
      material->setConstantUniforms(GL_TRUE);

      modelMat = ref_ptr<ModelTransformationState>::manage(
          new ModelTransformationState);
      modelMat->set_modelMat(transformation, 0.0f);
      modelMat->translate(Vec3f(0.0f, 0.0f, 0.0f), 0.0f);
      modelMat->setConstantUniforms(GL_TRUE);

      ref_ptr<ShaderInput> posAtt_ = ref_ptr<ShaderInput>::manage(
          new ShaderInput4f( "Position" ));
      mesh->setTransformFeedbackAttribute(posAtt_);

      ref_ptr<ShaderInput> norAtt_ = ref_ptr<ShaderInput>::manage(
          new ShaderInput3f( "norWorld" ));
      mesh->setTransformFeedbackAttribute(norAtt_);

      ref_ptr<StateNode> meshNode = renderTree->addMesh(mesh, modelMat, material);
#ifdef DEBUG_NORMAL
      ref_ptr<StateNode> &tfParent = renderTree->perspectivePass();
      map< string, ref_ptr<ShaderInput> > tfInputs =
          renderTree->collectParentInputs(*tfParent.get());

      ref_ptr<TFMeshState> tfState =
          ref_ptr<TFMeshState>::manage(new TFMeshState(mesh));
      tfState->joinStates(ref_ptr<State>::manage(
          new DebugNormal(tfInputs, 0.1)));

      ref_ptr<StateNode> tfNode = ref_ptr<StateNode>::manage(
          new StateNode(ref_ptr<State>::cast(tfState)));
      tfParent->addChild(tfNode);
#endif
    }

    list<AnimInterpoation> interpolations;
    interpolations.push_back(AnimInterpoation("pos","interpolate_elastic"));
    interpolations.push_back(AnimInterpoation("nor","interpolate_elastic"));

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

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
