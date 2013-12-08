/*
 * camera.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "camera.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/resource-manager.h>

#define REGEN_CAMERA_CATEGORY "camera"

/**
 * Updates Camera Projection when window size changes.
 */
class ProjectionUpdater : public EventHandler
{
public:
  /**
   * Default constructor.
   * @param cam Camera reference.
   * @param windowViewport The window dimensions.
   */
  ProjectionUpdater(const ref_ptr<Camera> &cam,
      const ref_ptr<ShaderInput2i> &windowViewport)
  : EventHandler(), cam_(cam), windowViewport_(windowViewport) { }

  void call(EventObject*, EventData*) {
    const ref_ptr<Frustum> &frustum = cam_->frustum();
    cam_->updateFrustum(
        windowViewport_->getVertex(0),
        frustum->fov()->getVertex(0),
        frustum->near()->getVertex(0),
        frustum->far()->getVertex(0));
  }

protected:
  ref_ptr<Camera> cam_;
  ref_ptr<ShaderInput2i> windowViewport_;
};

CameraResource::CameraResource()
: ResourceProvider(REGEN_CAMERA_CATEGORY)
{}

ref_ptr<Camera> CameraResource::createResource(
    SceneParser *parser,
    SceneInputNode &input)
{
  const string &type = input.getValue<string>("type", "default");

  if(type == "default") {
    ref_ptr<Camera> cam = ref_ptr<Camera>::alloc();
    cam->set_isAudioListener(
        input.getValue<bool>("audio-listener", false));
    cam->position()->setVertex(0,
        input.getValue<Vec3f>("position", Vec3f(0.0f,2.0f,-2.0f)));

    Vec3f dir = input.getValue<Vec3f>("direction", Vec3f(0.0f,0.0f,1.0f));
    dir.normalize();
    cam->direction()->setVertex(0, dir);

    cam->updateFrustum(
        parser->getViewport()->getVertex(0),
        input.getValue<GLfloat>("fov", 45.0f),
        input.getValue<GLfloat>("near", 0.1f),
        input.getValue<GLfloat>("far", 200.0f));
    // Update frustum when window size changes
    parser->addEventHandler(Application::RESIZE_EVENT,
        ref_ptr<ProjectionUpdater>::alloc(cam, parser->getViewport()));
    parser->putState(input.getName(),cam);

    return cam;
  }
  else if(type == "reflection") {
    ref_ptr<Camera> userCamera =
        parser->getResources()->getCamera(parser, input.getValue("camera"));
    ref_ptr<MeshVector> mesh =
        parser->getResources()->getMesh(parser, input.getValue("mesh"));
    if(userCamera.get()==NULL) {
      REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
      return ref_ptr<Camera>();
    }
    if(mesh.get()==NULL || mesh->empty()) {
      REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
      return ref_ptr<Camera>();
    }
    const vector< ref_ptr<Mesh> > &vec = *mesh.get();

    ref_ptr<ReflectionCamera> cam = ref_ptr<ReflectionCamera>::alloc(
        userCamera,vec[0],input.getValue<GLuint>("vertex-index",0u));
    parser->putState(input.getName(),cam);
    return cam;
  }
  else {
    REGEN_WARN("Ignoring Camera '" << input.getDescription() << "' with unknown type.");
    return ref_ptr<Camera>();
  }

}
