/*
 * texture-updater.cpp
 *
 *  Created on: 18.03.2013
 *      Author: daniel
 */

#include <applications/qt/qt-application.h>
#include <regen/config.h>

#include <regen/meshes/rectangle.h>
#include <regen/states/texture-state.h>
#include <regen/states/shader-state.h>
#include <regen/states/fbo-state.h>
#include <regen/states/blit-state.h>
#include <regen/states/shader-configurer.h>

#include "texture-updater-widget.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

using namespace regen;

// Resizes Framebuffer texture when the window size changed
class FramebufferResizer : public EventHandler
{
public:
  FramebufferResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
  : EventHandler(), fboState_(fbo), wScale_(wScale), hScale_(hScale) { }

  void call(EventObject *evObject, unsigned int id, void*) {
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

ref_ptr<Mesh> createTextureWidget(
    Application *app,
    const ref_ptr<TextureState> &texState,
    const ref_ptr<StateNode> &root)
{
  Rectangle::Config quadConfig;
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.isTangentRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  quadConfig.rotation = Vec3f(0.5*M_PI, 0.0*M_PI, 1.0*M_PI);
  quadConfig.posScale = Vec3f(1.0f);
  quadConfig.texcoScale = Vec2f(1.0f);
  quadConfig.levelOfDetail = 0;
  quadConfig.isTexcoRequired = GL_TRUE;
  quadConfig.isNormalRequired = GL_FALSE;
  quadConfig.centerAtOrigin = GL_TRUE;
  ref_ptr<Mesh> mesh = ref_ptr<Mesh>::manage(new Rectangle(quadConfig));

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
  // create and show application window
  ref_ptr<QtApplication> app = ref_ptr<QtApplication>::manage(new QtApplication(argc,argv));
  app->setupLogging();
  app->toplevelWidget()->setWindowTitle("Texture Updater");
  app->glWidget().setUpdateInterval(20);

  TextureUpdaterWidget *widget = new TextureUpdaterWidget(app.get());
  widget->setFixedSize(550,600);
  widget->show();
  app->show();

  // add a custom path for shader loading
  boost::filesystem::path shaderPath(PROJECT_SOURCE_DIR);
  shaderPath /= "applications";
  shaderPath /= "texture-updater";
  shaderPath /= "shader";
  app->addShaderPath(shaderPath);

  widget->openFile();

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

  // create the main widget
  createTextureWidget(app.get(), widget->texture(), sceneRoot);

  setBlitToScreen(app.get(), fbo, GL_COLOR_ATTACHMENT0);
  int exitCode = app->mainLoop();
  widget->writeConfig();
  return exitCode;
}
