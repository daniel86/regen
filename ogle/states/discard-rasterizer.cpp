/*
 * discard-rasterizer.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "discard-rasterizer.h"

DiscardRasterizer::DiscardRasterizer(): State()
{
}
void DiscardRasterizer::enable(RenderState *state)
{
  glEnable(GL_RASTERIZER_DISCARD);
}
void DiscardRasterizer::disable(RenderState *state)
{
  glDisable(GL_RASTERIZER_DISCARD);
}
