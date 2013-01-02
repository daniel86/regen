/*
 * video-player.cpp
 *
 *  Created on: 01.01.2013
 *      Author: daniel
 */

#include <ogle/render-tree/render-tree.h>
#include <ogle/meshes/box.h>
#include <ogle/meshes/sphere.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/av/audio.h>
#include <ogle/animations/animation-manager.h>

#include <applications/application-config.h>
#include <applications/qt-ogle-application.h>
#include <applications/test-render-tree.h>
#include <applications/test-camera-manipulator.h>

#include "video-player-widget.h"

int main(int argc, char** argv)
{
  TestRenderTree *renderTree = new TestRenderTree;

  // create a qt application
  QtOGLEApplication *application = new QtOGLEApplication(renderTree, argc, argv);
  application->setWaitForVSync(GL_TRUE);
  application->set_windowTitle("OpenGL player");

  // create the main widget and connect it to applications key events
  ref_ptr<VideoPlayerWidget> widget =
      ref_ptr<VideoPlayerWidget>::manage(new VideoPlayerWidget(application));
  application->connect(OGLEApplication::KEY_EVENT, ref_ptr<EventCallable>::cast(widget));
  application->connect(OGLEApplication::BUTTON_EVENT, ref_ptr<EventCallable>::cast(widget));

  widget->show();
  application->show();

  // setup the render target
  ref_ptr<FBOState> fboState = renderTree->setRenderToTexture(
      1.0f,1.0f, GL_RGB, GL_DEPTH_COMPONENT24, GL_FALSE, GL_FALSE, Vec4f(0.0f));

  // configure OpenAL for the video player
  AudioSystem &as = AudioSystem::get();
  as.set_listenerPosition( Vec3f(0.0) );
  as.set_listenerVelocity( Vec3f(0.0) );
  as.set_listenerOrientation( Vec3f(0.0,0.0,1.0), UP_VECTOR );

  // add a GUI element that covers the complete screen
  Rectangle::Config quadConfig;
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 0.0*M_PI);
  quadConfig.posScale = Vec3f(1.0f, 1.0f, 1.0f);
  quadConfig.texcoScale = Vec2f(-1.0f, 1.0f);
  ref_ptr<MeshState> quad = ref_ptr<MeshState>::manage(new Rectangle(quadConfig));
  quad->shaderDefine("USE_NORMALIZED_COORDINATES", "TRUE");
  ref_ptr<TextureState> texState =
      ref_ptr<TextureState>::manage(new TextureState(widget->texture()));
  texState->setMapTo(MAP_TO_COLOR);
  quad->joinStates(ref_ptr<State>::cast(texState));
  renderTree->addGUIElement(quad);

  // blit FBO to screen
  renderTree->setBlitToScreen(fboState->fbo(), GL_COLOR_ATTACHMENT0);

  // enter qt's main loop
  return application->mainLoop();
}
