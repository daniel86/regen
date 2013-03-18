/*
 * fluid.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <ogle/utility/xml.h>
#include <ogle/gl-types/gl-enum.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/states/shader-configurer.h>

#include "texture-loader.h"
#include "texture-updater.h"
using namespace ogle;
using namespace rapidxml;

TextureUpdateOperation::TextureUpdateOperation(const ref_ptr<FrameBufferObject> &outputBuffer)
: State(), numIterations_(1)
{
  textureQuad_ = ref_ptr<Mesh>::cast(Rectangle::getUnitQuad());

  outputTexture_ = outputBuffer->firstColorBuffer();
  outputBuffer_ = ref_ptr<FBOState>::manage(new FBOState(outputBuffer));
  joinStates(ref_ptr<State>::cast(outputBuffer_));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));

  Texture3D *tex3D = dynamic_cast<Texture3D*>(outputTexture_.get());
  numInstances_ = (tex3D==NULL ? 1 : tex3D->depth());

  set_blendMode(BLEND_MODE_SRC);
}

void TextureUpdateOperation::createShader(const ShaderState::Config &cfg, const string &key)
{
  ShaderConfigurer cfg_(cfg);
  cfg_.addState(this);
  cfg_.addState(textureQuad_.get());
  shader_->createShader(cfg_.cfg(), key);

  for(list<TextureBuffer>::iterator it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
  { it->loc = shader_->shader()->samplerLocation(it->nameInShader); }
}

void TextureUpdateOperation::set_blendMode(BlendMode blendMode)
{
  blendMode_ = blendMode;

  if(blendState_.get()!=NULL) disjoinStates(blendState_);
  blendState_ = ref_ptr<State>::manage(new BlendState(blendMode));
  joinStates(blendState_);
}

void TextureUpdateOperation::set_clearColor(const Vec4f &clearColor)
{
  ClearColorState::Data data;
  data.colorBuffers.push_back(GL_COLOR_ATTACHMENT0);
  data.clearColor = clearColor;
  outputBuffer_->setClearColor(data);
}

void TextureUpdateOperation::set_numIterations(GLuint numIterations)
{
  numIterations_ = numIterations;
}

void TextureUpdateOperation::addInputBuffer(const ref_ptr<FrameBufferObject> &buffer, const string &nameInShader)
{
  TextureBuffer b;
  b.buffer = buffer;
  b.loc = shader_->shader().get() ? shader_->shader()->samplerLocation(nameInShader) : 0;
  b.nameInShader = nameInShader;
  inputBuffer_.push_back(b);
}

void TextureUpdateOperation::executeOperation(RenderState *rs)
{
  list<TextureBuffer>::iterator it;

  enable(rs);
  // reserve input texture channels
  for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
  { it->channel = rs->reserveTextureChannel(); }

  for(register unsigned int i=0u; i<numIterations_; ++i)
  {
    if(blendMode_!=BLEND_MODE_SRC)
    { outputTexture_->nextBuffer(); }
    // setup render target
    glDrawBuffer(GL_COLOR_ATTACHMENT0 +
        (outputTexture_->bufferIndex()+1) % outputTexture_->numBuffers());
    // setup shader input textures
    for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
    {
      it->buffer->firstColorBuffer()->activate(it->channel);
      glUniform1i(it->loc, it->channel);
    }

    textureQuad_->draw(numInstances_);
    outputTexture_->nextBuffer();
  }
  // release input texture channels
  for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
  { rs->releaseTextureChannel(); }

  disable(rs);
}

const ref_ptr<Shader>& TextureUpdateOperation::shader()
{ return shader_->shader(); }
const ref_ptr<FrameBufferObject>& TextureUpdateOperation::outputBuffer()
{ return outputBuffer_->fbo(); }
const ref_ptr<Texture>& TextureUpdateOperation::outputTexture()
{ return outputTexture_; }

//////////////////////
/////////////////////

TextureUpdater::TextureUpdater()
: Animation(GL_TRUE,GL_FALSE), dt_(0.0), framerate_(60)
{}

static void parseOperations(
    TextureUpdater *updater,
    xml_node<> *root,
    GLboolean isInitial,
    const map<string, ref_ptr<FrameBufferObject> > &buffers,
    const ShaderState::Config &globalShaderConfig)
{
  map<string, ref_ptr<FrameBufferObject> >::const_iterator it;

  for(xml_node<> *n=root->first_node("operation"); n!=NULL; n= n->next_sibling("operation"))
  {
    string outputName = XMLLoader::readAttribute<string>(n, "out");
    string shaderKey = XMLLoader::readAttribute<string>(n, "shader");

    it = buffers.find(outputName);
    if(it==buffers.end()) {
      throw XMLLoader::Error(FORMAT_STRING("no buffer named '" << outputName << "' known."));
    }
    ref_ptr<FrameBufferObject> buffer = it->second;

    // load operation
    ref_ptr<TextureUpdateOperation> operation =
        ref_ptr<TextureUpdateOperation>::manage(new TextureUpdateOperation(buffer));
    // read XML configuration
    try {
      operation->set_blendMode( XMLLoader::readAttribute<BlendMode>(n, "blend") );
    } catch(XMLLoader::Error &e) {}
    try {
      operation->set_clearColor( XMLLoader::readAttribute<Vec4f>(n, "clearColor") );
    } catch(XMLLoader::Error &e) {}
    try {
      operation->set_numIterations( XMLLoader::readAttribute<GLuint>(n, "iterations") );
    } catch(XMLLoader::Error &e) {}

    // compile shader
    ShaderState::Config shaderConfig(globalShaderConfig);
    XMLLoader::loadShaderConfig(n, shaderConfig);
    operation->createShader(shaderConfig, shaderKey);

    // load uniforms
    for(xml_attribute<>* attr=n->first_attribute(); attr; attr=attr->next_attribute())
    {
      if(!boost::starts_with(attr->name(), "in_")) continue;
      string uniformName = string(attr->name()).substr(3);

      if(operation->shader()->isSampler(uniformName)) {
        it = buffers.find(attr->value());
        if(it==buffers.end()) {
          WARN_LOG("no buffer named '" << attr->value() << "' known for operation.");
        } else {
          operation->addInputBuffer(it->second, uniformName);
        }
      }
      else if(operation->shader()->isUniform(uniformName)) {
        ref_ptr<ShaderInput> in = operation->shader()->input(uniformName);
        // parse value
        stringstream ss(attr->value());
        (*in.get()) << ss;
        // make applyInputs() apply this value
        operation->shader()->setInput(in);
      }
      else {
        WARN_LOG("'" << uniformName << "' is not an active uniform name for operation.");
      }
    }
    updater->addOperation(operation,isInitial);
  }
}

void TextureUpdater::operator>>(const string &xmlString)
{
  map<string, ref_ptr<FrameBufferObject> > bufferMap;
  rapidxml::xml_document<> doc;

  ifstream xmlInput(xmlString.c_str());
  vector<char> buffer((
      istreambuf_iterator<char>(xmlInput)),
      istreambuf_iterator<char>());
  buffer.push_back('\0');
  doc.parse<0>( &buffer[0] );

  // load root node
  xml_node<> *root = XMLLoader::loadNode(&doc, "TextureUpdater");
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
      tex = TextureLoader::load( XMLLoader::readAttribute<string>(buffersChild,"file") );
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
      bufferMap[name] = fbo;
      continue;
    }

    // create textures by parameters
    Vec3i size = XMLLoader::readAttribute<Vec3i>(buffersChild,"size");
    GLuint dim = XMLLoader::readAttribute<GLuint>(buffersChild,"components");
    GLuint count = XMLLoader::readAttribute<GLuint>(buffersChild,"count");
    GLenum pixelType = GL_UNSIGNED_BYTE;
    try {
      pixelType = GLEnum::pixelType( XMLLoader::readAttribute<string>(buffersChild,"pixelType") );
    } catch(XMLLoader::Error &e) {}

    ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
        new FrameBufferObject(size.x,size.y,size.z,GL_NONE,GL_NONE,GL_NONE));
    fbo->addTexture(count,
        size.z>1 ? GL_TEXTURE_3D : GL_TEXTURE_2D,
        GLEnum::textureFormat(dim),
        GLEnum::textureInternalFormat(pixelType,dim,16),
        pixelType);
    bufferMap[name] = fbo;

    fbo->drawBuffers();
    glClearColor(0.0,0.0,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  // parse initial operations
  xml_node<> *operationsNode = root->first_node("init");
  if(operationsNode!=NULL) parseOperations(this, operationsNode, GL_TRUE, bufferMap, shaderConfig);
  // parse operation loop
  operationsNode = root->first_node("loop");
  if(operationsNode!=NULL) parseOperations(this, operationsNode, GL_FALSE, bufferMap, shaderConfig);
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

void TextureUpdater::set_framerate(GLint framerate)
{ framerate_ = framerate; }

ref_ptr<Texture> TextureUpdater::outputTexture()
{
  if(operations_.empty()) return ref_ptr<Texture>();
  return (*operations_.rbegin())->outputTexture();
}
ref_ptr<FrameBufferObject> TextureUpdater::outputBuffer()
{
  if(operations_.empty()) return ref_ptr<FrameBufferObject>();
  return (*operations_.rbegin())->outputBuffer();
}

const TextureUpdater::OperationList& TextureUpdater::initialOperations()
{ return initialOperations_; }
const TextureUpdater::OperationList& TextureUpdater::operations()
{ return operations_; }

//////////////////////
/////////////////////
