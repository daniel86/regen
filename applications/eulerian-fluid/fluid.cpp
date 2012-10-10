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
    const Vec3i &size,
    GLboolean isLiquid)
: name_(name),
  size_(size),
  isLiquid_(isLiquid),
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

const Vec3i& Fluid::size()
{
  return size_;
}
const string& Fluid::name()
{
  return name_;
}
GLboolean Fluid::isLiquid()
{
  return isLiquid_;
}

GLfloat Fluid::liquidHeight()
{
  return liquidHeight_;
}
void Fluid::setLiquidHeight(GLfloat height)
{
  liquidHeight_ = height;
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

void Fluid::executeOperations(list<FluidOperation*> &operations)
{
  RenderState rs;
  GLint lastShaderID = -1;
  for(list<FluidOperation*>::iterator
      it=operations.begin(); it!=operations.end(); ++it)
  {
    FluidOperation *op = *it;
    op->execute(&rs, lastShaderID);
    lastShaderID = op->shader()->id();
  }
}
