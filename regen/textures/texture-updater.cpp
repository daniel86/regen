/*
 * fluid.cpp
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <regen/utility/xml.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/meshes/rectangle.h>
#include <regen/states/state-configurer.h>
#include <regen/utility/filesystem.h>

#include "noise-texture.h"
#include "texture-loader.h"
#include "texture-updater.h"
using namespace regen;
using namespace rapidxml;

TextureUpdateOperation::TextureUpdateOperation(const ref_ptr<FBO> &outputBuffer)
: State(), numIterations_(1)
{
  textureQuad_ = Rectangle::getUnitQuad();

  outputTexture_ = outputBuffer->firstColorBuffer();
  outputBuffer_ = ref_ptr<FBOState>::alloc(outputBuffer);
  joinStates(outputBuffer_);

  shader_ = ref_ptr<ShaderState>::alloc();
  joinStates(shader_);

  Texture3D *tex3D = dynamic_cast<Texture3D*>(outputTexture_.get());
  numInstances_ = (tex3D==NULL ? 1 : tex3D->depth());

  set_blendMode(BLEND_MODE_SRC);
}

void TextureUpdateOperation::createShader(const StateConfig &cfg, const string &key)
{
  StateConfigurer cfg_(cfg);
  cfg_.addState(this);
  cfg_.addState(textureQuad_.get());
  shader_->createShader(cfg_.cfg(), key);
  textureQuad_->initializeResources(RenderState::get(), cfg_.cfg(), shader_->shader());

  for(list<TextureBuffer>::iterator it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
  { it->loc = shader_->shader()->samplerLocation(it->nameInShader); }
}

void TextureUpdateOperation::set_blendMode(BlendMode blendMode)
{
  blendMode_ = blendMode;

  if(blendState_.get()!=NULL) disjoinStates(blendState_);
  blendState_ = ref_ptr<BlendState>::alloc(blendMode);
  joinStates(blendState_);
}

void TextureUpdateOperation::set_clearColor(const Vec4f &clearColor)
{
  ClearColorState::Data data;
  data.colorBuffers.buffers_.push_back(GL_COLOR_ATTACHMENT0);
  data.clearColor = clearColor;
  outputBuffer_->setClearColor(data);
}

void TextureUpdateOperation::set_numIterations(GLuint numIterations)
{
  numIterations_ = numIterations;
}

void TextureUpdateOperation::addInputBuffer(const ref_ptr<Texture> &buffer, const string &nameInShader)
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
    { outputTexture_->nextObject(); }
    // setup render target
    outputBuffer_->fbo()->drawBuffers().push(DrawBuffers(GL_COLOR_ATTACHMENT0 +
        (outputTexture_->objectIndex()+1) % outputTexture_->numObjects()));
    for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
    {
      it->buffer->begin(rs, it->channel);
      glUniform1i(it->loc, it->channel);
    }

    textureQuad_->enable(rs);
    textureQuad_->disable(rs);
    outputBuffer_->fbo()->drawBuffers().pop();
    outputTexture_->nextObject();
    for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
    { it->buffer->end(rs); }
  }
  // release input texture channels
  for(it=inputBuffer_.begin(); it!=inputBuffer_.end(); ++it)
  { rs->releaseTextureChannel(); }

  disable(rs);
}

const ref_ptr<Shader>& TextureUpdateOperation::shader()
{ return shader_->shader(); }
const ref_ptr<FBO>& TextureUpdateOperation::outputBuffer()
{ return outputBuffer_->fbo(); }
const ref_ptr<Texture>& TextureUpdateOperation::outputTexture()
{ return outputTexture_; }

//////////////////////
/////////////////////

TextureUpdater::TextureUpdater()
: Animation(GL_TRUE,GL_FALSE), dt_(0.0), framerate_(60), initialOperationAdded_(GL_FALSE)
{}

static void parseOperations(
    TextureUpdater *updater,
    xml_node<> *root,
    GLboolean isInitial,
    const map<string, ref_ptr<FBO> > &buffers,
    const StateConfig &globalShaderConfig)
{
  map<string, ref_ptr<FBO> >::const_iterator it;

  for(xml_node<> *n=root->first_node("operation"); n!=NULL; n= n->next_sibling("operation"))
  {
    string outputName = xml::readAttribute<string>(n, "out");
    string shaderKey = xml::readAttribute<string>(n, "shader");

    it = buffers.find(outputName);
    if(it==buffers.end()) {
      throw xml::Error(REGEN_STRING("no buffer named '" << outputName << "' known."));
    }
    ref_ptr<FBO> buffer = it->second;

    // load operation
    ref_ptr<TextureUpdateOperation> operation =
        ref_ptr<TextureUpdateOperation>::alloc(buffer);
    // read XML configuration
    try {
      operation->set_blendMode( xml::readAttribute<BlendMode>(n, "blend") );
    } catch(xml::Error &e) {}
    try {
      operation->set_clearColor( xml::readAttribute<Vec4f>(n, "clearColor") );
    } catch(xml::Error &e) {}
    try {
      operation->set_numIterations( xml::readAttribute<GLuint>(n, "iterations") );
    } catch(xml::Error &e) {}

    // compile shader
    StateConfig shaderConfig(globalShaderConfig);
    xml::loadStateConfig(n, shaderConfig);
    operation->createShader(shaderConfig, shaderKey);

    // load uniforms
    for(xml_attribute<>* attr=n->first_attribute(); attr; attr=attr->next_attribute())
    {
      if(!boost::starts_with(attr->name(), "in_")) continue;
      string uniformName = string(attr->name()).substr(3);

      if(operation->shader()->samplerLocation(uniformName)!=-1) {
        it = buffers.find(attr->value());
        if(it==buffers.end()) {
          REGEN_WARN("no buffer named '" << attr->value() << "' known for operation.");
        } else {
          operation->addInputBuffer(it->second->firstColorBuffer(), uniformName);
        }
      }
      else if(operation->shader()->uniformLocation(uniformName)!=-1) {
        ref_ptr<ShaderInput> in = operation->shader()->createUniform(uniformName);
        if(in.get()) {
          // parse value
          stringstream ss(attr->value());
          (*in.get()) << ss;
          // make applyInputs() apply this value
          operation->shader()->setInput(in);
        }
      }
      else {
        REGEN_WARN("'" << uniformName << "' is not an active uniform name for operation.");
      }
    }
    updater->addOperation(operation,isInitial);
  }
}

void TextureUpdater::operator>>(const string &xmlString)
{
  GL_ERROR_LOG();
  map<string, ref_ptr<FBO> > bufferMap;
  rapidxml::xml_document<> doc;

  ifstream xmlInput(xmlString.c_str());
  vector<char> buffer((
      istreambuf_iterator<char>(xmlInput)),
      istreambuf_iterator<char>());
  buffer.push_back('\0');
  doc.parse<0>( &buffer[0] );

  // load root node
  xml_node<> *root = xml::loadNode(&doc, "TextureUpdater");
  // load shader configuration
  StateConfig shaderConfig;
  xml::loadStateConfig(root, shaderConfig);
  // apply updater configuration
  try {
    set_framerate( xml::readAttribute<GLint>(root, "framerate") );
  } catch(xml::Error &e) {}

  // load textures
  xml_node<> *buffers = xml::loadNode(root,"buffers");
  for(xml_node<> *buffersChild=xml::loadNode(buffers,"buffer");
      buffersChild; buffersChild= buffersChild->next_sibling("buffer"))
  {
    string name = xml::readAttribute<string>(buffersChild,"name");
    ref_ptr<Texture> tex;

    // check if a texture file is specified
    try {
      string path = xml::readAttribute<string>(buffersChild,"file");

      PathChoice texPaths;
      texPaths.choices_.push_back(filesystemPath(
          REGEN_SOURCE_DIR, path, "/"));
      texPaths.choices_.push_back(filesystemPath(
          REGEN_INSTALL_PREFIX, string("share/")+path, "/"));

      tex = textures::load(texPaths.firstValidPath());
    } catch(xml::Error &e) {}
    // check if a spectrum texture was requested
    try {
      Vec2f params = xml::readAttribute<Vec2f>(buffersChild,"spectrum");
      tex = textures::loadSpectrum(params.x, params.y, 256);
    } catch(xml::Error &e) {}

    if(tex.get()!=NULL) {
      ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(tex->width(),tex->height(),1);
      fbo->addTexture(tex);
      bufferMap[name] = fbo;
      continue;
    }

    // create textures by parameters
    Vec3i size = xml::readAttribute<Vec3i>(buffersChild,"size");
    GLuint dim = xml::readAttribute<GLuint>(buffersChild,"components");
    GLuint count = xml::readAttribute<GLuint>(buffersChild,"count");
    GLenum pixelType = GL_UNSIGNED_BYTE;
    try {
      pixelType = glenum::pixelType( xml::readAttribute<string>(buffersChild,"pixelType") );
    } catch(xml::Error &e) {}

    ref_ptr<FBO> fbo = ref_ptr<FBO>::alloc(size.x,size.y,size.z);
    fbo->addTexture(count,
            size.z>1 ? GL_TEXTURE_3D : GL_TEXTURE_2D,
            glenum::textureFormat(dim),
            glenum::textureInternalFormat(pixelType,dim,16),
            pixelType);
    bufferMap[name] = fbo;

    fbo->drawBuffers().push(fbo->colorBuffers());
    glClear(GL_COLOR_BUFFER_BIT);
    fbo->drawBuffers().pop();
  }

  operations_.clear();
  initialOperations_.clear();
  // parse initial operations
  xml_node<> *operationsNode = root->first_node("init");
  if(operationsNode!=NULL) parseOperations(this, operationsNode, GL_TRUE, bufferMap, shaderConfig);
  // parse operation loop
  operationsNode = root->first_node("loop");
  if(operationsNode!=NULL) parseOperations(this, operationsNode, GL_FALSE, bufferMap, shaderConfig);
  GL_ERROR_LOG();
}

void TextureUpdater::addOperation(const ref_ptr<TextureUpdateOperation> &operation, GLboolean isInitial)
{
  if(isInitial) {
    initialOperations_.push_back(operation);
    initialOperationAdded_ = GL_TRUE;
  }
  else {
    operations_.push_back(operation);
  }
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
  GL_ERROR_LOG();
  rs->toggles().push(RenderState::DEPTH_TEST, GL_FALSE);
  rs->depthMask().push(GL_FALSE);

  for(OperationList::const_iterator it=operations.begin(); it!=operations.end(); ++it)
  { (*it)->executeOperation(rs); }

  rs->depthMask().pop();
  rs->toggles().pop(RenderState::DEPTH_TEST);
  GL_ERROR_LOG();
}

void TextureUpdater::glAnimate(RenderState *rs, GLdouble dt)
{
  dt_ += dt;

  if(initialOperationAdded_) {
    executeOperations(rs, initialOperations_);
    initialOperationAdded_ = GL_FALSE;
  }

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
ref_ptr<FBO> TextureUpdater::outputBuffer()
{
  if(operations_.empty()) return ref_ptr<FBO>();
  return (*operations_.rbegin())->outputBuffer();
}

const TextureUpdater::OperationList& TextureUpdater::initialOperations()
{ return initialOperations_; }
const TextureUpdater::OperationList& TextureUpdater::operations()
{ return operations_; }
