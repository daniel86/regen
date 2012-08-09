/*
 * sky-box.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "sky-box.h"

SkyBox::SkyBox(
    ref_ptr<Camera> &cam,
    ref_ptr<Texture> &tex,
    GLfloat far)
: Cube(),
  tex_(tex),
  cam_(cam)
{
  tex->set_wrapping(GL_CLAMP_TO_EDGE);
  tex->addMapTo(MAP_TO_COLOR);
  tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
  tex->setupMipmaps(GL_DONT_CARE);

  createVertexData(
      Vec3f(0.0), // rotation
      Vec3f(far), // scale
      Vec2f(1.0f,1.0f), // uv scale
      false, // no normals
      true,
      true // generate cubemap uv
      );

  ref_ptr<State> texState = ref_ptr<State>::manage(new TextureState(tex));
  joinStates(texState);
}

void SkyBox::draw()
{
  glCullFace(GL_FRONT);
  Cube::draw();
  glCullFace(GL_BACK);
}

void SkyBox::resize(GLfloat far)
{
  createVertexData(
      Vec3f(0.0), // rotation
      Vec3f(far), // scale
      Vec2f(1.0f), // uv scale
      true, // no normals
      true,
      true // generate cubemap uv
      );
}

void SkyBox::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->ignoreCameraTranslation = true;
}
