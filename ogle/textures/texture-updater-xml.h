
#ifndef TEXTURE_UPDATER_XML_H_
#define TEXTURE_UPDATER_XML_H_

#include <boost/algorithm/string.hpp>

#include <ogle/utility/logging.h>
#include <ogle/utility/string-util.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/textures/texture-loader.h>

#include <vector>
#include <string>

#include <ogle/external/rapidxml/rapidxml.hpp>
#include <ogle/external/rapidxml/rapidxml_utils.hpp>
#include <ogle/external/rapidxml/rapidxml_print.hpp>

using namespace rapidxml;
typedef xml_node<> TextureUpdateNode;

using namespace ogle;

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

static bool readTextureUpdateBuffersXML(
    TextureUpdater *textureUpdater,
    TextureUpdateNode *parent)
{
  TextureUpdateNode *child = parent->first_node("buffer");
  if(child==NULL) {
    ERROR_LOG("'buffer' tag missing for texture-updater '" << textureUpdater->name() << "'.");
    return false;
  }

  for(; child; child= child->next_sibling("buffer"))
  {
    xml_attribute<>* nameAtt = child->first_attribute("name");
    if(nameAtt==NULL) {
      ERROR_LOG("no 'name' tag defined for buffer.");
      return NULL;
    }
    string name = nameAtt->value();

    // check if a texture file is specified
    xml_attribute<>* fileAtt = child->first_attribute("file");
    if(fileAtt!=NULL) {
      ref_ptr<Texture> tex = TextureLoader::load(fileAtt->value());
      ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
          new FrameBufferObject(tex->width(),tex->height(),1,GL_NONE,GL_NONE,GL_NONE));
      fbo->addColorAttachment(*tex.get());
      textureUpdater->addBuffer(fbo, name);
      continue;
    }

    // check if a spectrum texture was requested
    xml_attribute<>* spectrumAtt = child->first_attribute("spectrum");
    if(spectrumAtt!=NULL) {
      Vec2f params;
      stringstream ss(spectrumAtt->value());
      ss >> params;
      GLint numTexels = 256;
      GLenum mimpmapFlag = GL_DONT_CARE;
      ref_ptr<Texture> tex = TextureLoader::loadSpectrum(
          params.x, params.y, numTexels, mimpmapFlag);
      ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
          new FrameBufferObject(tex->width(),tex->height(),1,GL_NONE,GL_NONE,GL_NONE));
      fbo->addColorAttachment(*tex.get());
      textureUpdater->addBuffer(fbo, name);
      continue;
    }

    // create textures by parameters
    {
      xml_attribute<>* sizeAtt = child->first_attribute("size");
      if(sizeAtt==NULL) {
        ERROR_LOG("no 'size' tag defined for texture-updater '" << textureUpdater->name() << "'.");
        return NULL;
      }
      Vec3i size(0); {
        stringstream ss(sizeAtt->value());
        ss >> size;
        if(size.z<1) { size.z=1; }
      }

      xml_attribute<>* dimAtt = child->first_attribute("components");
      if(dimAtt==NULL) {
        ERROR_LOG("no 'components' tag defined for texture-updater '" << textureUpdater->name() << "'.");
        return NULL;
      }
      GLuint dim; {
        stringstream ss(dimAtt->value());
        ss >> dim;
      }

      xml_attribute<>* countAtt = child->first_attribute("count");
      if(countAtt==NULL) {
        ERROR_LOG("no 'count' tag defined for texture-updater '" << textureUpdater->name() << "'.");
        return NULL;
      }
      GLuint count; {
        stringstream ss(countAtt->value());
        ss >> count;
      }

      xml_attribute<>* pixelTypeAtt = child->first_attribute("pixelType");
      GLenum pixelType = GL_UNSIGNED_BYTE;
      if(pixelTypeAtt!=NULL) {
        pixelType = parsePixelType(pixelTypeAtt->value());
      }

      ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
          new FrameBufferObject(size.x,size.y,size.z,GL_NONE,GL_NONE,GL_NONE));
      fbo->addTexture(count,
          size.z>1 ? GL_TEXTURE_3D : GL_TEXTURE_2D,
          textureFormat(dim),
          textureInternalFormat(dim, pixelType),
          pixelType);
      textureUpdater->addBuffer(fbo, name);
    }
  }

  return true;
}

static TextureUpdateOperation* readTextureUpdateOperationXML(
    TextureUpdater *textureUpdater,
    TextureUpdateNode *node,
    GLuint shaderVersion,
    const map<string,string> &shaderConfig,
    const map<string,string> &updaterConfig)
{
  xml_attribute<>* outputAtt = node->first_attribute("out");
  if(outputAtt==NULL) {
    ERROR_LOG("no 'out' tag defined for texture-updater '" << textureUpdater->name() << "'.");
    return NULL;
  }
  FrameBufferObject *buffer = textureUpdater->getBuffer(outputAtt->value());
  if(buffer==NULL) {
    ERROR_LOG("no buffer named '" << outputAtt->value() <<
        "' known for texture-updater '" << textureUpdater->name() << "'.");
    return NULL;
  }

  map<string,string> operationConfig(updaterConfig);
  map<string,string> opShaderConfig(shaderConfig);

  for (xml_attribute<>* attr=node->first_attribute();
      attr; attr=attr->next_attribute())
  {
    string name = attr->name();
    const char *nameC = name.c_str();
    if(toupper(nameC[0])==nameC[0]) {
      opShaderConfig[name] = attr->value();
    } else if(name!="out" && !boost::starts_with(name, "in_")) {
      operationConfig[name] = attr->value();
    }
  }

  // check if shader code specified inline
  static const char* shaderNames[] = { "fs", "vs", "gs", "tes", "tcs" };
  for(unsigned int i=0; i<sizeof(shaderNames)/sizeof(char*); ++i) {
    xml_node<> *shaderNode = node->first_node(shaderNames[i]);
    if(shaderNode!=NULL) {
      operationConfig[shaderNames[i]] = shaderNode->value();
    }
  }

  TextureUpdateOperation *operation = new TextureUpdateOperation(
      buffer, shaderVersion, operationConfig, opShaderConfig);

  Shader *operationShader = operation->shader();
  if(operationShader==NULL) {
    ERROR_LOG("no shader was loaded for operation.");
    delete operation;
    return NULL;
  }

  operationShader->setInput(
      ref_ptr<ShaderInput>::cast(buffer->inverseViewport()));
  operationShader->setInput(
      ref_ptr<ShaderInput>::cast(buffer->viewport()));

  // load uniforms
  for (xml_attribute<>* attr=node->first_attribute();
      attr; attr=attr->next_attribute())
  {
    if(boost::starts_with(attr->name(), "in_")) {
      string uniformName = string(attr->name()).substr(3);

      if(operationShader->isSampler(uniformName)) {
        FrameBufferObject *inputBuffer = textureUpdater->getBuffer(attr->value());
        if(inputBuffer==NULL) {
          ERROR_LOG("no buffer named '" << outputAtt->value() <<
              "' known for operation.");
        } else {
          operation->addInputBuffer(inputBuffer,
              operationShader->samplerLocation(uniformName), uniformName);
        }

      } else if(operationShader->isUniform(uniformName)) {
        ref_ptr<ShaderInput> inRef = operationShader->input(uniformName);
        ShaderInput &in = *inRef.get();
        // parse value
        stringstream ss(attr->value());
        in << ss;
        // make applyInputs() apply this value
        operationShader->setInput(inRef);
      } else {
        WARN_LOG("'" << uniformName <<
            "' is not an active uniform name for operation.");
      }
    }
  }

  return operation;
}

static list<TextureUpdateOperation*> readTextureUpdateOperationsXML(
    TextureUpdater *textureUpdater,
    TextureUpdateNode *parent,
    GLuint shaderVersion,
    const map<string,string> &shaderConfig,
    const map<string,string> &updaterConfig)
{
  list<TextureUpdateOperation*> operations;
  for(TextureUpdateNode *child = parent->first_node("operation");
      child!=NULL;
      child= child->next_sibling("operation"))
  {
    TextureUpdateOperation *operation =
        readTextureUpdateOperationXML(textureUpdater, child, shaderVersion, shaderConfig, updaterConfig);

    if(operation!=NULL) {
      operations.push_back(operation);
    } else {
      ERROR_LOG("operation failed to parse for texture-updater '" << textureUpdater->name() << "'.");
    }
  }
  return operations;
}

static void readTextureUpdaterXML(TextureUpdater *textureUpdater, char *xmlString)
{
  // character type defaults to char
  xml_document<> doc;
  doc.parse<0>(xmlString);

  // load fluid root node
  xml_node<> *root = doc.first_node("texture-updater");
  if(root==NULL) {
    ERROR_LOG("'texture-updater' tag missing in texture-updater definition file.");
    return;
  }

  GLuint shaderVersion = 150;
  map<string,string> shaderConfig, updaterConfig;
  for (xml_attribute<>* attr=root->first_attribute();
      attr; attr=attr->next_attribute())
  {
    string name = attr->name();
    const char *nameC = name.c_str();
    if(toupper(nameC[0])==nameC[0]) {
      shaderConfig[name] = attr->value();
    } else {
      updaterConfig[name] = attr->value();
    }
  }

  textureUpdater->parseConfig(updaterConfig);

  xml_node<> *buffers = root->first_node("buffers");
  if(buffers==NULL) {
    ERROR_LOG("'buffers' tag missing in texture-updater definition file.");
    return;
  }
  if(!readTextureUpdateBuffersXML(textureUpdater, buffers)) {
    return;
  }

  xml_node<> *operationsNode = root->first_node("init");
  if(operationsNode!=NULL) {
    list<TextureUpdateOperation*> initialOperations =
        readTextureUpdateOperationsXML(textureUpdater,operationsNode,shaderVersion,shaderConfig,updaterConfig);
    for(list<TextureUpdateOperation*>::iterator
        it=initialOperations.begin(); it!=initialOperations.end(); ++it)
    {
      textureUpdater->addOperation(*it, GL_TRUE);
    }
  }
  operationsNode = root->first_node("loop");
  if(operationsNode!=NULL) {
    list<TextureUpdateOperation*> operations =
        readTextureUpdateOperationsXML(textureUpdater,operationsNode,shaderVersion,shaderConfig,updaterConfig);
    for(list<TextureUpdateOperation*>::iterator
        it=operations.begin(); it!=operations.end(); ++it)
    {
      textureUpdater->addOperation(*it, GL_FALSE);
    }
  }
}

#endif // TEXTURE_UPDATER_XML_H_
