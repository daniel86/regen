/*
 * shadow-map.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "shadow-map.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/resource-manager.h>
#include <regen/gl-types/gl-enum.h>

#define REGEN_SHADOW_MAP_CATEGORY "shadow-map"

ShadowMapResource::ShadowMapResource()
: ResourceProvider(REGEN_SHADOW_MAP_CATEGORY)
{}

ref_ptr<ShadowMap> ShadowMapResource::createResource(
    SceneParser *parser,
    SceneInputNode &input)
{
  if(!input.hasAttribute("light")) {
    REGEN_WARN("No light attribute defined for '" << input.getDescription() << "'.");
    return ref_ptr<ShadowMap>();
  }
  if(!input.hasAttribute("camera")) {
    REGEN_WARN("No camera attribute defined for '" << input.getDescription() << "'.");
    return ref_ptr<ShadowMap>();
  }
  const string cameraID = input.getValue("camera");
  const string lightID  = input.getValue("light");
  ref_ptr<Light> light = parser->getResources()->getLight(parser,lightID);
  ref_ptr<Camera> cam  = parser->getResources()->getCamera(parser,cameraID);

  if(light.get()==NULL) {
    REGEN_WARN("No Light with ID '" << lightID <<
        "' known to node " << input.getDescription() << ".");
    return ref_ptr<ShadowMap>();
  }
  if(cam.get()==NULL) {
    REGEN_WARN("No Camera with ID '" << cameraID <<
        "' known to node " << input.getDescription() << ".");
    return ref_ptr<ShadowMap>();
  }

  ShadowMap::Config shadowConfig; {
    shadowConfig.size        = input.getValue<GLuint>("size", 1024u);
    shadowConfig.numLayer    = input.getValue<GLuint>("num-layer", 3u);
    shadowConfig.splitWeight = input.getValue<GLdouble>("split-weight", 0.9);
    shadowConfig.depthType = glenum::pixelType(
        input.getValue<string>("pixel-type", "FLOAT"));

    GLint depthSize = input.getValue<GLint>("pixel-size", 16);
    if(depthSize<=16)
      shadowConfig.depthFormat = GL_DEPTH_COMPONENT16;
    else if(depthSize<=24)
      shadowConfig.depthFormat = GL_DEPTH_COMPONENT24;
    else
      shadowConfig.depthFormat = GL_DEPTH_COMPONENT32;
  }
  ref_ptr<ShadowMap> sm = ref_ptr<ShadowMap>::alloc(light,cam,shadowConfig);

  // Hacks for shadow issues
  sm->setCullFrontFaces(
      input.getValue<bool>("cull-front-faces",true));
  if(input.hasAttribute("polygon-offset")) {
    Vec2f x = input.getValue<Vec2f>("polygon-offset",Vec2f(0.0f,0.0f));
    sm->setPolygonOffset(x.x,x.y);
  }
  // Hide cube shadow map faces.
  if(input.hasAttribute("hide-faces")) {
    const string val = input.getValue<string>("hide-faces","");
    vector<string> faces;
    boost::split(faces,val,boost::is_any_of(","));
    for(vector<string>::iterator it=faces.begin();
        it!=faces.end(); ++it) {
      int faceIndex = atoi(it->c_str());
      sm->set_isCubeFaceVisible(
          GL_TEXTURE_CUBE_MAP_POSITIVE_X+faceIndex, GL_FALSE);
    }
  }

  // Setup moment computation. Moments make it possible to
  // apply blur filter to shadow map.
  if(input.getValue<bool>("compute-moments",false)) {
    sm->setComputeMoments();

    if(input.getValue<bool>("blur",false)) {
      sm->createBlurFilter(
          input.getValue<GLuint>("blur-size",GLuint(4)),
          input.getValue<GLfloat>("blur-sigma",GLfloat(2.0)),
          input.getValue<GLboolean>("blur-downsample-twice",GL_FALSE));
    }
  }
  parser->putState(input.getName(),sm);

  return sm;
}


