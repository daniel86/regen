/*
 * fluid.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include "texture-updater.h"
using namespace ogle;

#include "texture-updater-xml.h"

// XXX: -> gl enum
static GLenum parsePixelType(const string &val)
{
  if(val == "GL_HALF_FLOAT") {
    return GL_HALF_FLOAT;
  }
  else if(val == "GL_FLOAT") {
    return GL_FLOAT;
  }
  else if(val == "GL_UNSIGNED_BYTE") {
    return GL_UNSIGNED_BYTE;
  }
  else {
    WARN_LOG("unknown pixel type '" << val << "'.");
    return GL_UNSIGNED_BYTE;
  }
}
static GLenum textureFormat(GLuint numComponents)
{
  switch (numComponents) {
  case 1: return GL_RED;
  case 2: return GL_RG;
  case 3: return GL_RGB;
  case 4: return GL_RGBA;
  }
  return GL_RGBA;
}
static GLenum textureInternalFormat(GLuint numComponents, GLenum pixelType)
{
  if(pixelType==GL_FLOAT) {
    switch (numComponents) {
    case 1: return GL_RED;
    case 2: return GL_RG;
    case 3: return GL_RGB;
    case 4: return GL_RGBA;
    }
  }
  else if(pixelType==GL_HALF_FLOAT) {
    switch (numComponents) {
    case 1: return GL_R16F;
    case 2: return GL_RG16F;
    case 3: return GL_RGB16F;
    case 4: return GL_RGBA16F;
    }
  }
  else {
    switch (numComponents) {
    case 1: return GL_R32F;
    case 2: return GL_RG32F;
    case 3: return GL_RGB32F;
    case 4: return GL_RGBA32F;
    }
  }
  return GL_RGBA;
}

TextureUpdater::TextureUpdater()
: Animation(GL_TRUE,GL_FALSE), dt_(0.0), framerate_(60)
{}

void TextureUpdater::operator>>(xml_node<> *doc)
{
  // load root node
  xml_node<> *root = XMLLoader::loadNode(doc, "TextureUpdater");
  // load shader configuration
  ShaderState::Config shaderConfig;
  XMLLoader::loadShaderConfig(root, shaderConfig);
  // apply updater configuration
  try {
    set_framerate( XMLLoader::readAttribute<GLint>(root, "framerate") );
  } catch(XMLLoader::Error &e) {}

  // load textures
  xml_node<> *buffers = XMLLoader::loadNode(root,"buffers");
  for(xml_node<> *buffersChild=XMLLoader::loadNode(buffers,"buffer");
      buffersChild; buffersChild= buffersChild->next_sibling("buffer"))
  {
    string name = XMLLoader::readAttribute<string>(buffersChild,"name");
    ref_ptr<Texture> tex;

    // check if a texture file is specified
    try {
      string fileAtt = XMLLoader::readAttribute<string>(buffersChild,"file");
      tex = TextureLoader::load(fileAtt);
    } catch(XMLLoader::Error &e) {}
    // check if a spectrum texture was requested
    try {
      Vec2f params = XMLLoader::readAttribute<Vec2f>(buffersChild,"spectrum");
      tex = TextureLoader::loadSpectrum(params.x, params.y, 256);
    } catch(XMLLoader::Error &e) {}

    if(tex.get()!=NULL) {
      ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
          new FrameBufferObject(tex->width(),tex->height(),1,GL_NONE,GL_NONE,GL_NONE));
      fbo->addTexture(tex);
      addBuffer(fbo, name);
      continue;
    }

    // create textures by parameters
    Vec3i size = XMLLoader::readAttribute<Vec3i>(buffersChild,"size");
    GLuint dim = XMLLoader::readAttribute<GLuint>(buffersChild,"components");
    GLuint count = XMLLoader::readAttribute<GLuint>(buffersChild,"count");
    GLenum pixelType = GL_UNSIGNED_BYTE;
    try {
      pixelType = parsePixelType( XMLLoader::readAttribute<string>(buffersChild,"pixelType") );
    } catch(XMLLoader::Error &e) {}

    ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
        new FrameBufferObject(size.x,size.y,size.z,GL_NONE,GL_NONE,GL_NONE));
    fbo->addTexture(count,
        size.z>1 ? GL_TEXTURE_3D : GL_TEXTURE_2D,
        textureFormat(dim),
        textureInternalFormat(dim, pixelType),
        pixelType);
    addBuffer(fbo, name);

    fbo->drawBuffers();
    glClearColor(0.0,0.0,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  // parse initial operations
  xml_node<> *operationsNode = root->first_node("init");
  if(operationsNode!=NULL)
  {
    for(xml_node<> *n = operationsNode->first_node("operation"); n!=NULL; n= n->next_sibling("operation"))
    {
      ref_ptr<TextureUpdateOperation> operation = readTextureUpdateOperationXML(this, n, shaderConfig);
      if(operation.get())
      { addOperation(operation,GL_TRUE); }
      else
      { ERROR_LOG("operation failed to parse."); }
    }
  }

  // parse operation loop
  operationsNode = root->first_node("loop");
  if(operationsNode!=NULL)
  {
    for(xml_node<> *n = operationsNode->first_node("operation"); n!=NULL; n= n->next_sibling("operation"))
    {
      ref_ptr<TextureUpdateOperation> operation = readTextureUpdateOperationXML(this, n, shaderConfig);
      if(operation.get())
      { addOperation(operation,GL_FALSE); }
      else
      { ERROR_LOG("operation failed to parse."); }
    }
  }
}

void TextureUpdater::addBuffer(const ref_ptr<FrameBufferObject> &buffer, const string &name)
{
  buffers_[name] = buffer;
}
ref_ptr<FrameBufferObject> TextureUpdater::getBuffer(const string &name)
{
  return buffers_[name];
}

void TextureUpdater::addOperation(const ref_ptr<TextureUpdateOperation> &operation, GLboolean isInitial)
{
  if(isInitial)
  { initialOperations_.push_back(operation); }
  else
  { operations_.push_back(operation); }
}
void TextureUpdater::removeOperation(TextureUpdateOperation *operation)
{
  for(OperationList::iterator it=operations_.begin(); it!=operations_.end(); ++it)
  {
    if(it->get() == operation) {
      operations_.erase(it);
      break;
    }
  }
  for(OperationList::iterator it=initialOperations_.begin(); it!=initialOperations_.end(); ++it)
  {
    if(it->get() == operation) {
      initialOperations_.erase(it);
      break;
    }
  }
}

void TextureUpdater::executeOperations(RenderState *rs, const OperationList &operations)
{
  rs->toggles().push(RenderState::DEPTH_TEST, GL_FALSE);
  rs->depthMask().push(GL_FALSE);

  for(OperationList::const_iterator it=operations.begin(); it!=operations.end(); ++it)
  { (*it)->executeOperation(rs); }

  rs->toggles().pop(RenderState::DEPTH_TEST);
  rs->depthMask().pop();
}

void TextureUpdater::glAnimate(RenderState *rs, GLdouble dt)
{
  dt_ += dt;
  if(dt_ > 1000.0/(GLdouble)framerate_) {
    executeOperations(rs, operations_);
    dt_ = 0.0;
  }
}

GLint TextureUpdater::framerate() const
{ return framerate_; }
void TextureUpdater::set_framerate(GLint framerate)
{ framerate_ = framerate; }

const TextureUpdater::OperationList& TextureUpdater::initialOperations()
{ return initialOperations_; }
const TextureUpdater::OperationList& TextureUpdater::operations()
{ return operations_; }
