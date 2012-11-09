
#include <ogle/render-tree/render-tree.h>
#include <ogle/models/cube.h>
#include <ogle/models/sphere.h>
#include <ogle/textures/image-texture.h>
#include <ogle/models/quad.h>
#include <ogle/states/particle-state.h>
#include <ogle/states/depth-state.h>
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
      GL_TRUE,
      Vec4f(0.0f)
  );

  renderTree->setLight();

  ref_ptr<ModelTransformationState> modelMat;
  ref_ptr<Material> material;

  {
    const GLuint numParticles = 10000;
    ref_ptr<ParticleState> particles =
        ref_ptr<ParticleState>::manage(new ParticleState(numParticles));

    ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
    depth->set_useDepthTest(GL_FALSE);
    depth->set_useDepthWrite(GL_FALSE);
    //particles->joinStates(ref_ptr<State>::cast(depth));
    particles->joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));

    //// configure shader input

    ref_ptr<ShaderInput1f> startPointSize =
        ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("startPointSize"));
    startPointSize->setUniformData(1.0);
    startPointSize->set_isConstant(GL_TRUE);
    particles->setInput(ref_ptr<ShaderInput>::cast(startPointSize));

    ref_ptr<ShaderInput1f> stopPointSize =
        ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("stopPointSize"));
    stopPointSize->setUniformData(24.0);
    stopPointSize->set_isConstant(GL_TRUE);
    particles->setInput(ref_ptr<ShaderInput>::cast(stopPointSize));

#define USE_PARTICLE_TEXTURE
#define BOUNCE_BACK
#define USE_RANDOM_COLOR
#ifdef USE_PARTICLE_TEXTURE
    ref_ptr<Texture> smokeTex = ref_ptr<Texture>::manage(new ImageTexture("res/textures/particle.png"));
    ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(smokeTex));
    texState->set_name("particleTexture");
    texState->setMapTo(MAP_TO_CUSTOM);
    particles->joinStates(ref_ptr<State>::cast(texState));
#endif

    // introduce particle attributes
#ifdef USE_RANDOM_COLOR
    ref_ptr<ShaderInput3f> particleColor =
        ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("col"));
    particleColor->setVertexData(numParticles, NULL);
    particles->setInput(ref_ptr<ShaderInput>::cast(particleColor));
#endif

    //// configure particle emitter

    ParticleState::Emitter emitter0(numParticles/2);
    ParticleState::Emitter emitter1(numParticles/2);
    //ParticleEmitter emitter2(numParticles/3);

    emitter0.values_.push_back(
        FuzzyShaderValue("pos", "vec3(-0.5,0.0,0.5)", "vec3(0.05,0.0,0.05)"));
    emitter1.values_.push_back(
        FuzzyShaderValue("pos", "vec3(-1.0,0.0,-1.0)"));
    //emitter2.values_.push_back(FuzzyValue("pos", "vec3(1.0,0.0,1.0)"));

    emitter0.values_.push_back(
        FuzzyShaderValue("velocity", "vec3(0.0, 3.2, 0.0)", "vec3(0.5,0.0,0.5)"));
    emitter1.values_.push_back(
        FuzzyShaderValue("velocity", "vec3(4.6, 4.6, 4.6)"));
    //emitter2.values_.push_back(FuzzyValue("velocity", "vec3(-3.6, 3.6, -3.6)", "vec3(1.5,0.0,1.5)"));

    emitter0.values_.push_back(
        FuzzyShaderValue("lifetime", "1.0", "0.2"));
    emitter1.values_.push_back(
        FuzzyShaderValue("lifetime", "1.0", "0.2"));
    //emitter2.values_.push_back(FuzzyValue("lifetime", "1.0", "0.2"));
#ifdef USE_RANDOM_COLOR
    emitter0.values_.push_back(
        FuzzyShaderValue("col", "vec3(0.0,0.0,0.0)", "vec3(1.0,1.0,1.0)"));
    emitter1.values_.push_back(
        FuzzyShaderValue("col", "vec3(0.0,0.0,0.0)", "vec3(1.0,1.0,1.0)"));
    //emitter2.values_.push_back(FuzzyValue("col", "vec3(0.0,0.0,0.0)", "vec3(1.0,1.0,1.0)"));
#endif
    particles->addEmitter(emitter0);
    particles->addEmitter(emitter1);
    //particles->addEmitter(emitter2);

    //// configure particle updater

    particles->addUpdater("reduceLifetime",
        "void reduceLifetime(float dt, inout uint seed) {\n"
        "    out_lifetime -= 0.1*dt;\n"
        "}");
    particles->addUpdater("applyNoise",
        "void applyNoise(float dt, inout uint seed) {\n"
        "    out_velocity += 1.0 * dt * random3(seed);\n"
        "}");
    particles->addUpdater("applyGravity",
        "void applyGravity(float dt, inout uint seed) {\n"
        "    out_velocity.y -= 9.81*dt;\n"
        "}");
#ifdef BOUNCE_BACK
    particles->addUpdater("applyBounceBox",
        "void applyBounceBox(float dt, inout uint seed) {\n"
        "    if(out_pos.x>1.0) { out_pos.x=1.0; out_velocity.x *= -1; };\n"
        "    if(out_pos.x<-1.0) { out_pos.x=-1.0; out_velocity.x *= -1; };\n"
        "    if(out_pos.z>1.0) { out_pos.z=1.0; out_velocity.z *= -1; };\n"
        "    if(out_pos.z<-1.0) { out_pos.z=-1.0; out_velocity.z *= -1; };\n"
        "    if(out_pos.y>1.0) { out_pos.y=1.0; out_velocity.y *= -1; };\n"
        "    if(out_pos.y<0.0) { out_pos.y=0.0; out_velocity.y *= -1; };\n"
        "}");
#endif
    particles->addUpdater("applyDamping",
        "void applyDamping(float dt, inout uint seed) {\n"
        "    out_velocity -= 1.0 * dt * out_velocity;\n"
        "}");

    material = ref_ptr<Material>::manage(new Material);
    material->set_useAlpha(GL_TRUE);
    material->setConstantUniforms(GL_TRUE);

    modelMat = ref_ptr<ModelTransformationState>::manage(
        new ModelTransformationState);
    modelMat->setConstantUniforms(GL_TRUE);

    ref_ptr<StateNode> meshNode = renderTree->addMesh(
        ref_ptr<MeshState>::cast(particles), modelMat, material, "");

    ShaderConfig shaderCfg;
    meshNode->configureShader(&shaderCfg);
#ifdef USE_PARTICLE_TEXTURE
    shaderCfg.define("HAS_DENSITY_TEXTURE", "TRUE");
#endif
    shaderCfg.define("HAS_LIFETIME_ALPHA", "TRUE");
    //shaderCfg.define("INVERT_LIFETIME_ALPHA", "TRUE");
    //shaderCfg.define("PARABOL_LIFETIME_ALPHA", "TRUE");

    particles->createResources(shaderCfg, "particles");
  }

  renderTree->setShowFPS();

  // blit fboState to screen. Scale the fbo attachment if needed.
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  return application->mainLoop();
}
