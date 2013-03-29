/*
 * video-player.cpp
 *
 *  Created on: 01.01.2013
 *      Author: daniel
 */

#include <applications/qt/qt-application.h>
#include <regen/config.h>

#include <regen/meshes/rectangle.h>
#include <regen/states/texture-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/fbo-state.h>
#include <regen/states/blit-state.h>
#include <regen/av/audio.h>
#include <regen/states/shader-configurer.h>

#include "video-player-widget.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

// Resizes Framebuffer texture when the window size changed
class FramebufferResizer : public EventHandler
{
public:
  FramebufferResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
  : EventHandler(), fboState_(fbo), wScale_(wScale), hScale_(hScale) { }

  void call(EventObject *evObject, EventData*) {
    Application *app = (Application*)evObject;
    const Vec2i& winSize = app->windowViewport()->getVertex2i(0);
    fboState_->resize(winSize.x*wScale_, winSize.y*hScale_);
  }

protected:
  ref_ptr<FBOState> fboState_;
  GLfloat wScale_, hScale_;
};

void setBlitToScreen(
    Application *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, app->windowViewport(), attachment));
  app->renderTree()->addChild(
      ref_ptr<StateNode>::manage(new StateNode(blitState)));
}

ref_ptr<Mesh> createVideoWidget(
    Application *app,
    const ref_ptr<Texture> &videoTexture,
    const ref_ptr<StateNode> &root)
{
  Rectangle::Config quadConfig;
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.isTangentRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 0.0*M_PI);
  quadConfig.posScale = Vec3f(1.0f);
  quadConfig.texcoScale = Vec2f(-1.0f, 1.0f);
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  ref_ptr<Mesh> mesh = ref_ptr<Mesh>::manage(new Rectangle(quadConfig));

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(videoTexture));
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  mesh->joinStates(ref_ptr<State>::cast(texState));

  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::manage(new ShaderState);
  mesh->joinStates(ref_ptr<State>::cast(shaderState));

  ref_ptr<StateNode> meshNode = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(mesh)));
  root->addChild(meshNode);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addNode(meshNode.get());
  shaderConfigurer.define("USE_NORMALIZED_COORDINATES", "TRUE");
  shaderState->createShader(shaderConfigurer.cfg(), "gui");

  return mesh;
}

int main(int argc, char** argv)
{
#ifdef Q_WS_X11
  XInitThreads();
#endif
  QGLFormat glFormat(
    QGL::SingleBuffer
   |QGL::NoAlphaChannel
   |QGL::NoAccumBuffer
   |QGL::NoDepthBuffer
   |QGL::NoStencilBuffer
   |QGL::NoStereoBuffers
   |QGL::NoSampleBuffers);
  glFormat.setSwapInterval(0);
  glFormat.setDirectRendering(true);
  glFormat.setRgba(true);
  glFormat.setOverlay(false);
  // XXX: text not rendering with core profile...
  //glFormat.setVersion(3,3);
  //glFormat.setProfile(QGLFormat::CoreProfile);

  // create and show application window
  ref_ptr<QtApplication> app = ref_ptr<QtApplication>::manage(
      new QtApplication(argc,argv,glFormat));
  app->setupLogging();
  app->toplevelWidget()->setWindowTitle("OpenGL player");
  app->glWidget().setUpdateInterval(50);

  // add a custom path for shader loading
  boost::filesystem::path shaderPath(PROJECT_SOURCE_DIR);
  shaderPath /= "applications";
  shaderPath /= "test";
  shaderPath /= "shader";
  app->addShaderPath(shaderPath);

  // create the main widget and connect it to applications key events
  ref_ptr<VideoPlayerWidget> widget =
      ref_ptr<VideoPlayerWidget>::manage(new VideoPlayerWidget(app.get()));
  widget->show();
  app->show();

  // configure OpenAL for the video player
  AudioSystem &as = AudioSystem::get();
  as.set_listenerPosition( Vec3f(0.0) );
  as.set_listenerVelocity( Vec3f(0.0) );
  as.set_listenerOrientation( Vec3f(0.0,0.0,1.0), Vec3f::up() );

  // create render target
  const Vec2i& winSize = app->windowViewport()->getVertex2i(0);
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(winSize.x, winSize.y, 1, GL_NONE,GL_NONE,GL_NONE));
  ref_ptr<Texture> target = fbo->addTexture(1, GL_TEXTURE_2D, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0);
  // resize fbo with window
  app->connect(Application::RESIZE_EVENT, ref_ptr<EventHandler>::manage(
      new FramebufferResizer(fboState,1.0,1.0)));

  // create a root node (that binds the render target)
  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));
  app->renderTree()->addChild(sceneRoot);
  // add the video widget to the root node
  createVideoWidget(app.get(), widget->texture(), sceneRoot);

  setBlitToScreen(app.get(), fbo, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
