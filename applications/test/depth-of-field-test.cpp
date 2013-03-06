
#include "factory.h"
using namespace ogle;

#define USE_HDR
#define USE_HUD

int main(int argc, char** argv)
{
  ref_ptr<OGLEFltkApplication> app = initApplication(argc,argv,"DoF");

  // create a root node for everything that needs camera as input
  ref_ptr<PerspectiveCamera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 1.2f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 20.0f );
  manipulator->set_degree( 0.0f );
  manipulator->setStepLength( M_PI*0.0 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
#ifdef USE_HDR
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get(),1.0,1.0,GL_RGB16F);
#else
  ref_ptr<FBOState> gBufferState = createGBuffer(app.get(),1.0,1.0,GL_RGBA);
#endif
  ref_ptr<StateNode> gBufferParent = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gBufferState)));
  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage( new StateNode);
  sceneRoot->addChild(gBufferParent);
  gBufferParent->addChild(gBufferNode);

  ref_ptr<Texture> gDiffuseTexture = gBufferState->fbo()->colorBuffer()[0];
  ref_ptr<Texture> gDepthTexture = gBufferState->fbo()->depthTexture();
  sceneRoot->addChild(gBufferNode);
  createBox(app.get(), gBufferNode);
  createSphere(app.get(), gBufferNode);
  createQuad(app.get(), gBufferNode);

  ref_ptr<DeferredShading> deferredShading = createShadingPass(
      app.get(), gBufferState->fbo(), sceneRoot);

  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gBufferState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);
  // add a sky box
  ref_ptr<DynamicSky> sky = createSky(app.get(), backgroundNode);
  deferredShading->addLight(ref_ptr<Light>::cast(sky->sun()));

  ref_ptr<FilterSequence> blur = createBlurState(
      app.get(), gDiffuseTexture, backgroundNode, 4, 2.0);
  // switch gDiffuseTexture buffer (last rendering was ontop)
  blur->joinStatesFront(ref_ptr<State>::manage(new PingPongTextureBuffer(gDiffuseTexture)));
  ref_ptr<Texture> blurTexture = blur->output();

  ref_ptr<DepthOfField> dof =
      createDoFState(app.get(), gDiffuseTexture, blurTexture, gDepthTexture, backgroundNode);
  dof->joinStatesFront(ref_ptr<State>::manage(
      new DrawBufferTex(gDiffuseTexture, GL_COLOR_ATTACHMENT0, GL_FALSE)));

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
