/*
 * camera.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "camera.h"
using namespace regen::scene;
using namespace regen;

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

  return cam;
}
