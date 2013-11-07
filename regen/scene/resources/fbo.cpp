/*
 * fbo.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "fbo.h"
#include "texture.h"
using namespace regen::scene;
using namespace regen;

#include <regen/gl-types/gl-enum.h>
#include <regen/scene/resources/texture.h>

#define REGEN_FBO_CATEGORY "fbo"

/**
 * Resizes Framebuffer texture when the window size changed.
 */
class FBOResizer : public EventHandler
{
public:
  FBOResizer(const ref_ptr<FBO> &fbo,
      const ref_ptr<ShaderInput2i> &windowViewport,
      GLfloat wScale, GLfloat hScale)
  : EventHandler(),
    fbo_(fbo),
    windowViewport_(windowViewport),
    wScale_(wScale), hScale_(hScale) { }

  void call(EventObject*, EventData*) {
    const Vec2i& winSize = windowViewport_->getVertex(0);
    fbo_->resize(winSize.x*wScale_, winSize.y*hScale_, 1);
  }

protected:
  ref_ptr<FBO> fbo_;
  ref_ptr<ShaderInput2i> windowViewport_;
  GLfloat wScale_, hScale_;
};

FBOResource::FBOResource()
: ResourceProvider(REGEN_FBO_CATEGORY)
{}

ref_ptr<FBO> FBOResource::createResource(
    SceneParser *parser, SceneInputNode &input)
{
  string sizeMode = input.getValue<string>("size-mode", "abs");
  Vec3f relSize   = input.getValue<Vec3f>("size", Vec3f(256.0,256.0,1.0));
  Vec3i absSize   = TextureResource::getSize(
      parser->getViewport(), sizeMode, relSize);

  ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(absSize.x,absSize.y);
  if(sizeMode == "rel") {
    ref_ptr<FBOResizer> resizer = ref_ptr<FBOResizer>::alloc(
          fbo,
          parser->getViewport(),
          relSize.x,
          relSize.y);
    parser->addEventHandler(Application::RESIZE_EVENT, resizer);
  }

  const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
  for(list< ref_ptr<SceneInputNode> >::const_iterator
      it=childs.begin(); it!=childs.end(); ++it)
  {
    ref_ptr<SceneInputNode> n = *it;

    if(n->getCategory() == "texture") {
      fbo->addTexture(TextureResource().createResource(parser,*n.get()));
    }
    else if(n->getCategory() == "depth") {
      GLint depthSize  = n->getValue<GLint>("pixel-size", 16);
      GLenum depthType = glenum::pixelType(
          n->getValue<string>("pixel-type", "UNSIGNED_BYTE"));

      GLenum depthFormat;
      if(depthSize<=16)      depthFormat=GL_DEPTH_COMPONENT16;
      else if(depthSize<=24) depthFormat=GL_DEPTH_COMPONENT24;
      else                   depthFormat=GL_DEPTH_COMPONENT32;

      fbo->createDepthTexture(GL_TEXTURE_2D,depthFormat,depthType);
    }
    else {
      REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
    }
  }

  return fbo;
}
