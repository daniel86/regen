
#include <regen/utility/filesystem.h>
#include "factory.h"
using namespace regen;

#define USE_HDR
#define USE_HUD

int main(int argc, char** argv)
{
  ref_ptr<QtApplication> app = initApplication(argc,argv,"HDR Reflection Map");

#ifdef USE_HDR
  ref_ptr<TextureCube> reflectionMap = createStaticReflectionMap(app.get(),filesystemPath(
      REGEN_SOURCE_DIR, "res/textures/cube-maps/grace.hdr"), GL_TRUE, GL_R11F_G11F_B10F);
#else
  ref_ptr<TextureCube> reflectionMap = createStaticReflectionMap(app.get(),filesystemPath(
      REGEN_SOURCE_DIR, "res/textures/cube-maps/stormydays.jpg", GL_FALSE, GL_RGBA));
#endif
  RenderState::get()->activeTexture().push(GL_TEXTURE7);
  RenderState::get()->textures().push(7,
      TextureBind(reflectionMap->targetType(), reflectionMap->id()));
  reflectionMap->set_wrapping(GL_CLAMP_TO_EDGE);
  RenderState::get()->textures().pop(7);
  RenderState::get()->activeTexture().pop();

  // create a root node for everything that needs camera as input
  ref_ptr<Camera> cam = createPerspectiveCamera(app.get());
  ref_ptr<LookAtCameraManipulator> manipulator = createLookAtCameraManipulator(app.get(), cam);
  manipulator->set_height( 0.0f );
  manipulator->set_lookAt( Vec3f(0.0f) );
  manipulator->set_radius( 2.0f );
  manipulator->set_degree( 0.0f );
  manipulator->setStepLength( M_PI*0.001 );

  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(cam)));
  app->renderTree()->addChild(sceneRoot);

  // create a GBuffer node. All opaque meshes should be added to
  // this node. Shading is done deferred.
#ifdef USE_HDR
  ref_ptr<FBOState> gTargetState = createGBuffer(app.get(),1.0,1.0,GL_RGB16F);
#else
  ref_ptr<FBOState> gTargetState = createGBuffer(app.get(),1.0,1.0,GL_RGBA);
#endif
  ref_ptr<StateNode> gTargetNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(gTargetState)));
  sceneRoot->addChild(gTargetNode);
  ref_ptr<Texture> gDiffuseTexture = gTargetState->fbo()->colorBuffer()[0];

  ref_ptr<StateNode> gBufferNode = ref_ptr<StateNode>::manage(new StateNode);
  gTargetNode->addChild(gBufferNode);
  createReflectionSphere(app.get(), reflectionMap, gBufferNode);

  // create root node for background rendering, draw ontop gDiffuseTexture
  ref_ptr<StateNode> backgroundNode = createBackground(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  sceneRoot->addChild(backgroundNode);
  createSkyCube(app.get(), reflectionMap, backgroundNode);

  ref_ptr<FilterSequence> blur = createBlurState(
      app.get(), gDiffuseTexture, backgroundNode, 10, 2.5);
  // switch gDiffuseTexture buffer (last rendering was ontop)
  blur->joinStatesFront(ref_ptr<State>::manage(new TexturePingPong(gDiffuseTexture)));
  ref_ptr<Texture> blurTexture = blur->output();

  ref_ptr<Tonemap> toenmap =
      createTonemapState(app.get(), gDiffuseTexture, blurTexture, backgroundNode);
  toenmap->blurAmount()->setVertex1f(0,0.5);
  toenmap->effectAmount()->setVertex1f(0,0.2);
  toenmap->exposure()->setVertex1f(0,16.0);
  toenmap->gamma()->setVertex1f(0,0.5);
  toenmap->radialBlurSamples()->setVertex1f(0,36.0);
  toenmap->radialBlurStartScale()->setVertex1f(0,1.0);
  toenmap->radialBlurScaleMul()->setVertex1f(0,0.9555);
  toenmap->joinStatesFront(ref_ptr<State>::manage(new DrawBufferUpdate(
      gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0)));

#ifdef USE_HUD
  // create HUD with FPS text, draw ontop gDiffuseTexture
  ref_ptr<StateNode> guiNode = createHUD(
      app.get(), gTargetState->fbo(),
      gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  app->renderTree()->addChild(guiNode);
  createLogoWidget(app.get(), guiNode);
  createFPSWidget(app.get(), guiNode);
#endif

  setBlitToScreen(app.get(), gTargetState->fbo(), gDiffuseTexture, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}

