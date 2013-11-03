/*
 * tesselation.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "tesselation.h"
using namespace regen::scene;
using namespace regen;

#define REGEN_TESS_STATE_CATEGORY "tesselation"

TesselationStateProvider::TesselationStateProvider()
: StateProcessor(REGEN_TESS_STATE_CATEGORY)
{}

void TesselationStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  ref_ptr<TesselationState> tess = ref_ptr<TesselationState>::alloc(
      input.getValue<GLuint>("num-patch-vertices",3u));

  tess->innerLevel()->setVertex(0,
      input.getValue<Vec4f>("inner-level",Vec4f(8.0f)));
  tess->outerLevel()->setVertex(0,
      input.getValue<Vec4f>("outer-level",Vec4f(8.0f)));
  tess->lodFactor()->setVertex(0,
      input.getValue<GLfloat>("lod-factor",4.0f));
  tess->set_lodMetric(input.getValue<TesselationState::LoDMetric>(
      "lod-metric",TesselationState::CAMERA_DISTANCE_INVERSE));

  state->joinStates(tess);
}


