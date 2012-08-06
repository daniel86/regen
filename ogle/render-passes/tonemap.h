/*
 * tonemap.h
 *
 *  Created on: 31.07.2012
 *      Author: daniel
 */

#ifndef TONEMAP_H_
#define TONEMAP_H_

#include <scene.h>
#include <render-pass.h>
#include <texture.h>
#include <shader.h>

class TonemapPass : public UnitOrthoRenderPass
{
public:
  TonemapPass(Scene *scene,
      ref_ptr<Texture2D> blurredTex);
  virtual void render();
protected:
  ref_ptr<Shader> tonemapShader_;
  ref_ptr<Texture2D> blurredTex_;
};

#endif /* TONEMAP_H_ */
