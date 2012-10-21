/*
 * fluid.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include "texture-updater.h"
#include "texture-updater-xml.h"

#include <ogle/states/render-state.h>

TextureUpdater* TextureUpdater::readFromXML(MeshState *textureQuad, const string &xmlFile)
{
  return readTextureUpdaterFileXML(textureQuad,xmlFile);
}

TextureUpdater::TextureUpdater(const string &name)
: Animation(),
  name_(name),
  dt_(0.0),
  framerate_(20)
{
}
TextureUpdater::~TextureUpdater()
{
  for(list<TextureUpdateOperation*>::iterator
      it=initialOperations_.begin(); it!=initialOperations_.end(); ++it)
  {
    delete *it;
  }
  for(list<TextureUpdateOperation*>::iterator
      it=operations_.begin(); it!=operations_.end(); ++it)
  {
    delete *it;
  }
}

const string& TextureUpdater::name()
{
  return name_;
}

GLint TextureUpdater::framerate()
{
  return framerate_;
}
void TextureUpdater::set_framerate(GLint framerate)
{
  framerate_ = framerate;
}

MeshState* TextureUpdater::textureQuad()
{
  return textureQuad_;
}
void TextureUpdater::set_textureQuad(MeshState *textureQuad)
{
  textureQuad_ = textureQuad;
}

//////////

void TextureUpdater::animate(GLdouble dt)
{
}
void TextureUpdater::updateGraphics(GLdouble dt)
{
  dt_ += dt;
  if(dt_ > 1000.0/(double)framerate_) {
    dt_ = 0.0;
    executeOperations(operations());
  }
}

/////////

void TextureUpdater::addBuffer(TextureBuffer *buffer)
{
  buffers_[buffer->name()] = buffer;
}
TextureBuffer* TextureUpdater::getBuffer(const string &name)
{
  return buffers_[name];
}

//////////

void TextureUpdater::addOperation(TextureUpdateOperation *operation, GLboolean isInitial)
{
  if(isInitial) {
    initialOperations_.push_back(operation);
  } else {
    operations_.push_back(operation);
  }
}
void TextureUpdater::removeOperation(TextureUpdateOperation *operation)
{
  for(list<TextureUpdateOperation*>::iterator
      it=operations_.begin(); it!=operations_.end(); ++it)
  {
    if(*it == operation) {
      operations_.erase(it);
      break;
    }
  }
  for(list<TextureUpdateOperation*>::iterator
      it=initialOperations_.begin(); it!=initialOperations_.end(); ++it)
  {
    if(*it == operation) {
      initialOperations_.erase(it);
      break;
    }
  }
}

list<TextureUpdateOperation*>& TextureUpdater::initialOperations()
{
  return initialOperations_;
}
list<TextureUpdateOperation*>& TextureUpdater::operations()
{
  return operations_;
}

void TextureUpdater::executeOperations(const list<TextureUpdateOperation*> &operations)
{
  RenderState rs;
  GLint lastShaderID = -1;

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  // bind vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, textureQuad_->vertexBuffer());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, textureQuad_->vertexBuffer());

  for(list<TextureUpdateOperation*>::const_iterator
      it=operations.begin(); it!=operations.end(); ++it)
  {
    TextureUpdateOperation *op = *it;
    op->execute(&rs, lastShaderID);
    lastShaderID = op->shader()->id();
  }

  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}
