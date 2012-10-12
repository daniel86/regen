/*
 * fluid.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include "fluid.h"

#include <ogle/states/render-state.h>

Fluid::Fluid(
    const string &name,
    GLfloat timestep,
    GLboolean isLiquid,
    GLboolean is2D)
: name_(name),
  timestep_(timestep),
  isLiquid_(isLiquid),
  is2D_(is2D),
  liquidHeight_(0.0f)
{
}
Fluid::~Fluid()
{
  for(list<FluidOperation*>::iterator
      it=initialOperations_.begin(); it!=initialOperations_.end(); ++it)
  {
    delete *it;
  }
  for(list<FluidOperation*>::iterator
      it=operations_.begin(); it!=operations_.end(); ++it)
  {
    delete *it;
  }
}

GLfloat Fluid::timestep() const {
  return timestep_;
}

const string& Fluid::name()
{
  return name_;
}
GLboolean Fluid::isLiquid()
{
  return isLiquid_;
}
GLboolean Fluid::is2D()
{
  return is2D_;
}

GLint Fluid::framerate()
{
  return framerate_;
}
void Fluid::set_framerate(GLint framerate)
{
  framerate_ = framerate;
}

GLfloat Fluid::liquidHeight()
{
  return liquidHeight_;
}
void Fluid::setLiquidHeight(GLfloat height)
{
  liquidHeight_ = height;
}
MeshState* Fluid::textureQuad()
{
  return textureQuad_;
}
void Fluid::set_textureQuad(MeshState *textureQuad)
{
  textureQuad_ = textureQuad;
}

/////////

void Fluid::addBuffer(FluidBuffer *buffer)
{
  buffers_[buffer->name()] = buffer;
}
FluidBuffer* Fluid::getBuffer(const string &name)
{
  return buffers_[name];
}

//////////

void Fluid::addOperation(FluidOperation *operation, GLboolean isInitial)
{
  if(isInitial) {
    initialOperations_.push_back(operation);
  } else {
    operations_.push_back(operation);
  }
}

const list<FluidOperation*>& Fluid::initialOperations()
{
  return initialOperations_;
}
const list<FluidOperation*>& Fluid::operations()
{
  return operations_;
}

void Fluid::executeOperations(const list<FluidOperation*> &operations)
{
  RenderState rs;
  GLint lastShaderID = -1;

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  // bind vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, textureQuad_->vertexBuffer());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureQuad_->vertexBuffer());

  for(list<FluidOperation*>::const_iterator
      it=operations.begin(); it!=operations.end(); ++it)
  {
    FluidOperation *op = *it;
    op->execute(&rs, lastShaderID);
    lastShaderID = op->shader()->id();
  }

  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}
