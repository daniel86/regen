/*
 * fluid.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include "texture-updater.h"
#include "texture-updater-xml.h"

#include <ogle/states/render-state.h>

TextureUpdater::TextureUpdater()
: Animation(),
  name_(""),
  dt_(0.0),
  framerate_(60)
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

void operator>>(istream &inputfile, TextureUpdater &v)
{
  vector<char> buffer((istreambuf_iterator<char>(inputfile)),
               istreambuf_iterator<char>( ));
  buffer.push_back('\0');
  readTextureUpdaterXML(&v, &buffer[0]);
}

void TextureUpdater::parseConfig(const map<string,string> &cfg)
{
  map<string,string>::const_iterator needle;

  needle = cfg.find("name");
  if(needle != cfg.end()) {
    name_ = needle->second;
  }

  needle = cfg.find("framerate");
  if(needle != cfg.end()) {
    GLint rate=framerate_;
    stringstream ss(needle->second);
    ss >> rate;
    set_framerate(rate);
  }
}

const string& TextureUpdater::name() const
{
  return name_;
}

GLint TextureUpdater::framerate() const
{
  return framerate_;
}
void TextureUpdater::set_framerate(GLint framerate)
{
  framerate_ = framerate;
}

//////////


void TextureUpdater::animate(GLdouble dt) {}
void TextureUpdater::glAnimate(GLdouble dt) {
  dt_ += dt;
  if(dt_ > 1000.0/(double)framerate_) {
    dt_ = 0.0;
    executeOperations(operations());
  }
}
GLboolean TextureUpdater::useGLAnimation() const {
  return GL_TRUE;
}
GLboolean TextureUpdater::useAnimation() const {
  return GL_FALSE;
}

/////////

void TextureUpdater::addBuffer(SimpleRenderTarget *buffer)
{
  buffers_[buffer->name()] = buffer;
}
SimpleRenderTarget* TextureUpdater::getBuffer(const string &name)
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
map<string,SimpleRenderTarget*>& TextureUpdater::buffers()
{
  return buffers_;
}

void TextureUpdater::executeOperations(const list<TextureUpdateOperation*> &operations)
{
  RenderState rs;
  GLint lastShaderID = -1;

  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  for(list<TextureUpdateOperation*>::const_iterator
      it=operations.begin(); it!=operations.end(); ++it)
  {
    TextureUpdateOperation *op = *it;
    op->updateTexture(&rs, lastShaderID);
    lastShaderID = op->shader()->id();
  }

  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}
