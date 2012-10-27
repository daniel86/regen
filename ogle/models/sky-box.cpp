/*
 * sky-box.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "sky-box.h"

static UnitCube::Config cubeCfg(GLfloat far)
{
  UnitCube::Config cfg;
  cfg.posScale = Vec3f(far);
  cfg.isNormalRequired = false;
  cfg.texcoMode = UnitCube::TEXCO_MODE_CUBE_MAP;
  return cfg;
}

SkyBox::SkyBox(
    ref_ptr<Camera> cam,
    ref_ptr<Texture> tex,
    GLfloat far)
: UnitCube(cubeCfg(far)),
  tex_(tex),
  cam_(cam)
{
  tex->set_wrapping(GL_CLAMP_TO_EDGE);
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(tex));
  texState->addMapTo(MAP_TO_COLOR);
  joinStates(ref_ptr<State>::cast(texState));
}

void SkyBox::draw(GLuint numInstances)
{
  // only render back faces
  glCullFace(GL_FRONT);
  UnitCube::draw(numInstances);
  // switch back face culling on again
  glCullFace(GL_BACK);
}

void SkyBox::resize(GLfloat far)
{
  Config cfg;
  cfg.posScale = Vec3f(far);
  cfg.isNormalRequired = false;
  cfg.texcoMode = TEXCO_MODE_CUBE_MAP;
  updateAttributes(cfg);
}

void SkyBox::configureShader(ShaderConfig *cfg)
{
  UnitCube::configureShader(cfg);
  cfg->setIgnoreCameraTranslation();
}
