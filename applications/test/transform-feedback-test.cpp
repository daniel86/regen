
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/states/shader-state.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/string-util.h>

#include <applications/application-config.h>
#ifdef USE_FLTK_TEST_APPLICATIONS
  #include <applications/fltk-ogle-application.h>
#else
  #include <applications/glut-ogle-application.h>
#endif

#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

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
      GeometryShaderInput inputPrimitive,
      GLfloat normalLength=0.1)
  : ShaderState()
  {
    map<string,string> shaderConfig;
    switch(inputPrimitive) {
    case GS_INPUT_POINTS:
      shaderConfig["GS_INPUT_PRIMITIVE"] = "points"; break;
    case GS_INPUT_LINES_ADJACENCY:
      shaderConfig["GS_INPUT_PRIMITIVE"] = "lines_adjacency"; break;
    case GS_INPUT_LINES:
      shaderConfig["GS_INPUT_PRIMITIVE"] = "lines"; break;
    case GS_INPUT_TRIANGLES:
      shaderConfig["GS_INPUT_PRIMITIVE"] = "triangles"; break;
    case GS_INPUT_TRIANGLES_ADJACENCY:
      shaderConfig["GS_INPUT_PRIMITIVE"] = "triangles_adjacency"; break;
    }
    shaderConfig["NORMAL_LENGTH"] = FORMAT_STRING(normalLength);
    // configuration using macros
    map<GLenum,string> shaderNames;
    shaderNames[GL_FRAGMENT_SHADER] = "debug-normal.fs";
    shaderNames[GL_VERTEX_SHADER]   = "debug-normal.vs";
    shaderNames[GL_GEOMETRY_SHADER] = "debug-normal.gs";

    shader_ = Shader::create(shaderConfig, inputs, shaderNames);
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
  virtual string name(){ return FORMAT_STRING("DebugNormal"); }
};

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

#ifdef USE_FLTK_TEST_APPLICATIONS
  OGLEFltkApplication *application = new OGLEFltkApplication(renderTree, argc, argv);
#else
  OGLEGlutApplication *application = new OGLEGlutApplication(renderTree, argc, argv);
#endif
  application->set_windowTitle("Transform Feedback");
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

  ref_ptr<Light> &light = renderTree->setLight();
  light->setConstantUniforms(GL_TRUE);

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
        new ShaderInput3f( "norWorld" ));
    sphereState->setTransformFeedbackAttribute(norAtt_);

    ref_ptr<Material> material = ref_ptr<Material>::manage(new Material);
    material->set_shading(Material::PHONG_SHADING);
    material->set_chrome();

    ref_ptr<StateNode> meshNode = renderTree->addMesh(sphereState, modelMat, material);

    ref_ptr<StateNode> &tfParent = renderTree->perspectivePass();
    map< string, ref_ptr<ShaderInput> > tfInputs =
        renderTree->collectParentInputs(*tfParent.get());

    ref_ptr<TFMeshState> tfState =
        ref_ptr<TFMeshState>::manage(new TFMeshState(sphereState));
    tfState->joinStates(ref_ptr<State>::manage(
        new DebugNormal(tfInputs, GS_INPUT_TRIANGLES, 0.1)));

    ref_ptr<StateNode> tfNode = ref_ptr<StateNode>::manage(
        new StateNode(ref_ptr<State>::cast(tfState)));
    renderTree->addChild(tfParent, tfNode);
  }

  // makes sense to add sky box last, because it looses depth test against
  // all other objects
  renderTree->addSkyBox("res/textures/cube-stormydays.jpg");
  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
