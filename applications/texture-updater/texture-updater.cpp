/*
 * texture-updater.cpp
 *
 *  Created on: 18.03.2013
 *      Author: daniel
 */

#include <applications/qt/qt-application.h>
#include <ogle/config.h>

#include <ogle/meshes/rectangle.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/blit-state.h>
#include <ogle/states/shader-configurer.h>

#include "texture-updater-widget.h"

using namespace ogle;

// Resizes Framebuffer texture when the window size changed
class FramebufferResizer : public EventHandler
{
public:
  FramebufferResizer(const ref_ptr<FBOState> &fbo, GLfloat wScale, GLfloat hScale)
  : EventHandler(), fboState_(fbo), wScale_(wScale), hScale_(hScale) { }

  virtual void call(EventObject *evObject, void*) {
    OGLEApplication *app = (OGLEApplication*)evObject;
    fboState_->resize(app->glWidth()*wScale_, app->glHeight()*hScale_);
  }

protected:
  ref_ptr<FBOState> fboState_;
  GLfloat wScale_, hScale_;
};

void setBlitToScreen(
    OGLEApplication *app,
    const ref_ptr<FrameBufferObject> &fbo,
    GLenum attachment)
{
  ref_ptr<State> blitState = ref_ptr<State>::manage(
      new BlitToScreen(fbo, app->glSizePtr(), attachment));
  app->renderTree()->addChild(
      ref_ptr<StateNode>::manage(new StateNode(blitState)));
}

ref_ptr<Mesh> createTextureWidget(
    OGLEApplication *app,
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
  // create and show application window
  ref_ptr<RootNode> tree = ref_ptr<RootNode>::manage(new RootNode);
  ref_ptr<QtApplication> app = ref_ptr<QtApplication>::manage(
      new QtApplication(tree,argc,argv));
  app->set_windowTitle("Texture Updater");
  app->glWidget().setUpdateInterval(20);

  TextureUpdaterWidget *widget = new TextureUpdaterWidget(app.get());
  widget->setFixedSize(550,600);
  widget->show();
  app->show();
  // set the render state that is used during tree traversal
  tree->set_renderState(ref_ptr<RenderState>::manage(new RenderState));

  // add a custom path for shader loading
  boost::filesystem::path shaderPath(PROJECT_SOURCE_DIR);
  shaderPath /= "applications";
  shaderPath /= "texture-updater";
  shaderPath /= "shader";
  OGLEApplication::setupGLSWPath(shaderPath);

  widget->openFile();

  // create render target
  ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(app->glWidth(), app->glHeight(), 1, GL_NONE,GL_NONE,GL_NONE));
  ref_ptr<Texture> target = fbo->addTexture(1, GL_TEXTURE_2D, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
  ref_ptr<FBOState> fboState = ref_ptr<FBOState>::manage(new FBOState(fbo));
  fboState->addDrawBuffer(GL_COLOR_ATTACHMENT0);
  // resize fbo with window
  app->connect(OGLEApplication::RESIZE_EVENT, ref_ptr<EventHandler>::manage(
      new FramebufferResizer(fboState,1.0,1.0)));

  // create a root node (that binds the render target)
  ref_ptr<StateNode> sceneRoot = ref_ptr<StateNode>::manage(
      new StateNode(ref_ptr<State>::cast(fboState)));
  app->renderTree()->addChild(sceneRoot);

  // create the main widget
  createTextureWidget(app.get(), widget->texture(), sceneRoot);

  setBlitToScreen(app.get(), fbo, GL_COLOR_ATTACHMENT0);
  return app->mainLoop();
}
