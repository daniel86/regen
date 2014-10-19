/*
 * sky-box.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <climits>

#include <regen/meshes/rectangle.h>
#include <regen/states/atomic-states.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>

#include "sky-box.h"
using namespace regen;

static Box::Config cubeCfg(GLuint levelOfDetail)
{
  Box::Config cfg;
  cfg.isNormalRequired = GL_FALSE;
  cfg.isTangentRequired = GL_FALSE;
  cfg.texcoMode = Box::TEXCO_MODE_CUBE_MAP;
  cfg.usage = VBO::USAGE_STATIC;
  cfg.levelOfDetail = levelOfDetail;
  return cfg;
}

SkyBox::SkyBox(GLuint levelOfDetail, const std::string &shaderKey)
: Box(cubeCfg(levelOfDetail)), HasShader(shaderKey)
{
  joinStates(ref_ptr<CullFaceState>::alloc(GL_FRONT));

  ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
  depth->set_depthFunc(GL_LEQUAL);
  joinStates(depth);

  joinStates(shaderState());

  shaderDefine("IGNORE_VIEW_TRANSLATION", "TRUE");
}

void SkyBox::setCubeMap(const ref_ptr<TextureCube> &cubeMap)
{
  cubeMap_ = cubeMap;
  if(texState_.get()) {
    disjoinStates(texState_);
  }
  texState_ = ref_ptr<TextureState>::alloc(cubeMap_);
  texState_->set_mapTo(TextureState::MAP_TO_COLOR);
  joinStatesFront(texState_);
}
const ref_ptr<TextureCube>& SkyBox::cubeMap() const
{
  return cubeMap_;
}

