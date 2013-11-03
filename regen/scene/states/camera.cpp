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

#define REGEN_CAMERA_STATE_CATEGORY "camera"

CameraStateProvider::CameraStateProvider()
: StateProcessor(REGEN_CAMERA_STATE_CATEGORY)
{}

void CameraStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  ref_ptr<Camera> cam = parser->getResources()->getCamera(parser,input.getName());
  if(cam.get()==NULL) {
    REGEN_WARN("Unable to load Camera for '" << input.getDescription() << "'.");
    return;
  }
  state->joinStates(cam);
}
