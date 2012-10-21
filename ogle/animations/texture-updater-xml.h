
#ifndef TEXTURE_UPDATER_XML_H_
#define TEXTURE_UPDATER_XML_H_

#include <boost/algorithm/string.hpp>

#include <ogle/utility/logging.h>
#include <ogle/utility/string-util.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/textures/image-texture.h>
#include <ogle/textures/spectral-texture.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <ogle/external/rapidxml/rapidxml.hpp>
#include <ogle/external/rapidxml/rapidxml_utils.hpp>
#include <ogle/external/rapidxml/rapidxml_print.hpp>
using namespace rapidxml;

typedef xml_node<> TextureUpdateNode;

static TextureBuffer::PixelType parsePixelType(const string &val)
{
  if(val == "f16" ||
      val=="16f" ||
      val == "F16" ||
      val == "16F") {
    return TextureBuffer::F16;
  } else if(val == "f32" ||
      val=="32f" ||
      val == "F32" ||
      val == "32F") {
    return TextureBuffer::F32;
  } else if(val == "byte") {
    return TextureBuffer::BYTE;
  } else {
    WARN_LOG("unknown pixel type '" << val << "'.");
    return TextureBuffer::F16;
  }
}
static TextureUpdateOperation::Mode parseOperationMode(const string &val)
{
  if(val == "modifyState" || val == "modify") {
    return TextureUpdateOperation::MODIFY_STATE;
  } else if(val == "newState" || val == "new") {
    return TextureUpdateOperation::NEW_STATE;
  } else {
    WARN_LOG("unknown mode '" << val << "'.");
    return TextureUpdateOperation::NEW_STATE;
  }
}
static TextureBlendMode parseOperationBlendMode(const string &val)
{
  if(val == "src") {
    return BLEND_MODE_SRC;
  } else if(val == "srcAlpha") {
    return BLEND_MODE_SRC_ALPHA;
  } else if(val == "alpha") {
    return BLEND_MODE_ALPHA;
  } else if(val == "mul") {
    return BLEND_MODE_MULTIPLY;
  } else if(val == "smoothAdd" || val == "average") {
    return BLEND_MODE_SMOOTH_ADD;
  } else if(val == "add") {
    return BLEND_MODE_ADD;
  } else if(val == "sub") {
    return BLEND_MODE_SUBSTRACT;
  } else if(val == "reverseSub") {
    return BLEND_MODE_REVERSE_SUBSTRACT;
  } else if(val == "lighten") {
    return BLEND_MODE_LIGHTEN;
  } else if(val == "darken") {
    return BLEND_MODE_DARKEN;
  } else if(val == "screen") {
    return BLEND_MODE_SCREEN;
  } else {
    WARN_LOG("unknown blend mode '" << val << "'.");
    return BLEND_MODE_SRC;
  }
}

////////////////////////////
/////////////  BUFFERS  ////
////////////////////////////
////  Example:
////  <buffers>
////      <buffer name="velocity" components="3" count="2" pixelType="16F" />
////      .....
////  </buffers>

#define XML_BUFFERS_TAG_NAME "buffers"
#define XML_BUFFER_TAG_NAME "buffer"
#define XML_BUFFER_NAME_TAG "name"
#define XML_BUFFER_SIZE_TAG "size"
#define XML_BUFFER_COMPONENTS_TAG "components"
#define XML_BUFFER_COUNT_TAG "count"
#define XML_BUFFER_PIXEL_TYPE_TAG "pixelType"
#define XML_BUFFER_FILE_TAG "file"
#define XML_BUFFER_SPECTRUM_TAG "spectrum"

static bool parseBuffers(TextureUpdater *textureUpdater, TextureUpdateNode *parent)
{
  TextureUpdateNode *child = parent->first_node(XML_BUFFER_TAG_NAME);
  if(child==NULL) {
    ERROR_LOG("'" << XML_BUFFER_TAG_NAME <<
        "' tag missing for texture-updater '" << textureUpdater->name() << "'.");
    return false;
  }

  for(; child; child= child->next_sibling(XML_BUFFER_TAG_NAME))
  {
    xml_attribute<>* nameAtt = child->first_attribute(XML_BUFFER_NAME_TAG);
    if(nameAtt==NULL) {
      ERROR_LOG("no '" << XML_BUFFER_NAME_TAG << "' tag defined.");
      return NULL;
    }
    string name = nameAtt->value();
    DEBUG_LOG("  parsing buffer '" << name << "'.");

    // check if a texture file is specified
    xml_attribute<>* fileAtt = child->first_attribute(XML_BUFFER_FILE_TAG);
    if(fileAtt!=NULL) {
      ref_ptr<Texture> tex = ref_ptr<Texture>::manage(new ImageTexture(fileAtt->value()));
      textureUpdater->addBuffer(new TextureBuffer(name, tex));
      continue;
    }

    // check if a special texture was requested
    xml_attribute<>* spectrumAtt = child->first_attribute(XML_BUFFER_SPECTRUM_TAG);
    if(spectrumAtt!=NULL) {
      Vec2f params;
      parseVec2f(spectrumAtt->value(),params);
      GLint numTexels = 256;
      GLenum mimpmapFlag = GL_DONT_CARE;
      GLboolean useMipmap = true;
      ref_ptr<SpectralTexture> spectralTex =
          ref_ptr<SpectralTexture>::manage(new SpectralTexture);
      spectralTex->set_spectrum(
          params.x, params.y,
          numTexels,
          mimpmapFlag,
          useMipmap);
      ref_ptr<Texture> tex = ref_ptr<Texture>::cast(spectralTex);
      textureUpdater->addBuffer(new TextureBuffer(name, tex));
      continue;
    }

    xml_attribute<>* sizeAtt = child->first_attribute(XML_BUFFER_SIZE_TAG);
    if(sizeAtt==NULL) {
      ERROR_LOG("no '" << XML_BUFFER_SIZE_TAG <<
          "' tag defined for texture-updater '" << textureUpdater->name() << "'.");
      return NULL;
    }
    Vec3i size(0);
    parseVec3i(sizeAtt->value(),size);
    if(size.z<1) { size.z=1; }

    xml_attribute<>* dimAtt = child->first_attribute(XML_BUFFER_COMPONENTS_TAG);
    if(dimAtt==NULL) {
      ERROR_LOG("no '" << XML_BUFFER_COMPONENTS_TAG <<
          "' tag defined for texture-updater '" << textureUpdater->name() << "'.");
      return NULL;
    }
    GLuint dim;
    parseVec1ui(dimAtt->value(),dim);

    xml_attribute<>* countAtt = child->first_attribute(XML_BUFFER_COUNT_TAG);
    if(countAtt==NULL) {
      ERROR_LOG("no '" << XML_BUFFER_COUNT_TAG <<
          "' tag defined for texture-updater '" << textureUpdater->name() << "'.");
      return NULL;
    }
    GLuint count;
    parseVec1ui(countAtt->value(),count);

    xml_attribute<>* pixelTypeAtt = child->first_attribute(XML_BUFFER_PIXEL_TYPE_TAG);
    TextureBuffer::PixelType pixelType = TextureBuffer::F16;
    if(pixelTypeAtt!=NULL) {
      pixelType = parsePixelType(pixelTypeAtt->value());
    }

    textureUpdater->addBuffer(new TextureBuffer(name, size, dim, count, pixelType));
  }

  return true;
}

////////////////////////////
////////////  PIPELINE  ////
////////////////////////////
////  Example:
////  <operations>
////      <operation fs=".." vs=".." mode="nextState" blend="add" clear="1" iterations="20" .... />
////      ....
////  </operations>

#define XML_PIPELINE_TAG_NAME "operations"
#define XML_STAGE_TAG_NAME "operation"
#define XML_STAGE_FS_TAG "fs"
#define XML_STAGE_VS_TAG "vs"
#define XML_STAGE_GS_TAG "gs"
#define XML_STAGE_TES_TAG "tes"
#define XML_STAGE_TCS_TAG "tcs"
#define XML_STAGE_MODE_TAG "mode"
#define XML_STAGE_BLEND_TAG "blend"
#define XML_STAGE_CLEAR_TAG "clearColor"
#define XML_STAGE_ITERATIONS_TAG "iterations"
#define XML_STAGE_INPUT_PREFIX "in_"
#define XML_STAGE_OUTPUT_TAG "out"

static TextureUpdateOperation* parseOperation(
    TextureUpdater *textureUpdater,
    TextureUpdateNode *node,
    const map<string,string> &shaderConfig)
{
  map<GLenum,string> shaderNames;

  xml_attribute<>* fsAtt = node->first_attribute(XML_STAGE_FS_TAG);
  xml_attribute<>* vsAtt = node->first_attribute(XML_STAGE_VS_TAG);
  xml_attribute<>* gsAtt = node->first_attribute(XML_STAGE_GS_TAG);
  xml_attribute<>* tesAtt = node->first_attribute(XML_STAGE_TES_TAG);
  xml_attribute<>* tcsAtt = node->first_attribute(XML_STAGE_TCS_TAG);
  xml_attribute<>* outputAtt = node->first_attribute(XML_STAGE_OUTPUT_TAG);

  if(fsAtt==NULL) {
    ERROR_LOG("no '" << XML_STAGE_FS_TAG <<
        "' tag defined for texture-updater '" << textureUpdater->name() << "'.");
    return NULL;
  }
  if(outputAtt==NULL) {
    ERROR_LOG("no '" << XML_STAGE_OUTPUT_TAG <<
        "' tag defined for texture-updater '" << textureUpdater->name() << "'.");
    return NULL;
  }
  DEBUG_LOG("  parsing operation '" << fsAtt->value() << "'.");

  TextureBuffer *buffer = textureUpdater->getBuffer(outputAtt->value());
  if(buffer==NULL) {
    ERROR_LOG("no buffer named '" << outputAtt->value() <<
        "' known for texture-updater '" << textureUpdater->name() << "'.");
    return NULL;
  }

  shaderNames[GL_FRAGMENT_SHADER] = fsAtt->value();
  if(vsAtt!=NULL) {
    shaderNames[GL_VERTEX_SHADER] = vsAtt->value();
  }
  if(gsAtt!=NULL) {
    shaderNames[GL_GEOMETRY_SHADER] = gsAtt->value();
  }
  if(tesAtt!=NULL) {
    shaderNames[GL_TESS_EVALUATION_SHADER] = tesAtt->value();
  }
  if(tcsAtt!=NULL) {
    shaderNames[GL_TESS_CONTROL_SHADER] = tcsAtt->value();
  }

  map<string,string> defines_;
  for(map<string,string>::const_iterator
      it=shaderConfig.begin(); it!=shaderConfig.end(); ++it)
  {
    defines_[it->first] = it->second;
  }
  for (xml_attribute<>* attr=node->first_attribute();
      attr; attr=attr->next_attribute())
  {
    string name = attr->name();
    const char *nameC = name.c_str();
    if(toupper(nameC[0])==nameC[0]) {
      defines_[name] = attr->value();
    }
  }
  TextureUpdateOperation *operation = new TextureUpdateOperation(
      shaderNames, buffer, textureUpdater->textureQuad(), defines_);

  // load operation configuration
  for (xml_attribute<>* attr=node->first_attribute();
      attr; attr=attr->next_attribute())
  {
    string name = attr->name();
    if(name == XML_STAGE_MODE_TAG) {
      operation->set_mode(parseOperationMode(attr->value()));
    }
    else if(name == XML_STAGE_BLEND_TAG) {
      operation->set_blendMode(parseOperationBlendMode(attr->value()));
    }
    else if(name == XML_STAGE_ITERATIONS_TAG) {
      GLuint numObstacles;
      if(parseVec1ui(attr->value(),numObstacles)==0) {
        operation->set_numIterations(numObstacles);
      }
    }
    else if(name == XML_STAGE_CLEAR_TAG) {
      Vec4f clearColor;
      if(parseVec4f(attr->value(),clearColor)==0) {
        operation->set_clearColor(clearColor);
      }
    }
  }

  Shader *operationShader = operation->shader();
  if(operationShader==NULL) {
    ERROR_LOG("no shader was loaded for operation '" <<
        operation->name() << "' for texture-updater '" << textureUpdater->name() << "'.");
    delete operation;
    return NULL;
  }

  ref_ptr<ShaderInput> inverseSize = ref_ptr<ShaderInput>::cast(buffer->inverseSize());
  operationShader->set_input(inverseSize->name(), inverseSize);

  // load uniforms and macros
  for (xml_attribute<>* attr=node->first_attribute();
      attr; attr=attr->next_attribute())
  {
    if(boost::starts_with(attr->name(), "in_")) {
      string uniformName = string(attr->name()).substr(3);

      if(operationShader->isSampler(uniformName)) {
        TextureBuffer *inputBuffer = textureUpdater->getBuffer(attr->value());
        if(inputBuffer==NULL) {
          ERROR_LOG("no buffer named '" << outputAtt->value() <<
              "' known for operation '" << operation->name() <<
              "' for texture-updater '" << textureUpdater->name() << "'.");
        } else {
          operation->addInputBuffer(inputBuffer, operationShader->samplerLocation(uniformName));
        }

      } else if(operationShader->isUniform(uniformName)) {
        ref_ptr<ShaderInput> inRef = operationShader->input(uniformName);
        ShaderInput &in = *inRef.get();
        // parse value
        in << attr->value();
      } else {
        WARN_LOG("'" << uniformName <<
            "' is not an active uniform name for operation '" <<
            operation->name() << "' for texture-updater '" << textureUpdater->name() << "'.");
      }
    }
  }

  return operation;
}

static list<TextureUpdateOperation*> parseOperations(
    TextureUpdater *textureUpdater,
    TextureUpdateNode *parent,
    const map<string,string> &shaderConfig)
{
  list<TextureUpdateOperation*> operations;
  for(TextureUpdateNode *child = parent->first_node(XML_STAGE_TAG_NAME);
      child!=NULL;
      child= child->next_sibling(XML_STAGE_TAG_NAME))
  {
    TextureUpdateOperation *operation = parseOperation(textureUpdater, child, shaderConfig);

    if(operation!=NULL) {
      operations.push_back(operation);
    } else {
      ERROR_LOG(XML_STAGE_TAG_NAME <<
          " failed to parse for texture-updater '" << textureUpdater->name() << "'.");
    }
  }
  return operations;
}

////  Example:
////  <texture-updater
////      name="test" ...
////  >
////      <buffers>....</buffers>
////      <operations>....</operations>
////  </texture-updater>

#define XML_UPDATER_TAG_NAME "texture-updater"
#define XML_UPDATER_NAME_TAG "name"
#define XML_UPDATER_FRAMERATE_TAG "framerate"
#define XML_UPDATER_SHADER_TAG "shader"

static TextureUpdater* parseTextureUpdaterStringXML(
    MeshState *textureQuad, char *xmlString)
{
  // character type defaults to char
  xml_document<> doc;
  doc.parse<0>(xmlString);

  // load fluid root node
  xml_node<> *root = doc.first_node(XML_UPDATER_TAG_NAME);
  if(root==NULL) {
    ERROR_LOG("'" << XML_UPDATER_TAG_NAME <<
        "' tag missing in texture-updater definition file.");
    return NULL;
  }

  xml_attribute<>* nameAtt = root->first_attribute(XML_UPDATER_NAME_TAG);
  if(nameAtt==NULL) {
    ERROR_LOG("no '" << XML_UPDATER_NAME_TAG << "' tag defined.");
    return NULL;
  }
  string name = nameAtt->value();
  DEBUG_LOG("parsing texture-updater '" << name << "'.");

  map<string,string> shaderConfig;
  for (xml_attribute<>* attr=root->first_attribute();
      attr; attr=attr->next_attribute())
  {
    string name = attr->name();
    const char *nameC = name.c_str();
    if(toupper(nameC[0])==nameC[0]) {
      shaderConfig[name] = attr->value();
    }
  }

  TextureUpdater *textureUpdater = new TextureUpdater(name);
  textureUpdater->set_textureQuad(textureQuad);

  xml_attribute<>* framerateAtt = root->first_attribute(XML_UPDATER_FRAMERATE_TAG);
  if(framerateAtt!=NULL) {
    GLuint framerate;
    if(parseVec1ui(framerateAtt->value(),framerate)==0) {
      textureUpdater->set_framerate( framerate );
    }
  }

  DEBUG_LOG("parsing buffers.");
  xml_node<> *buffers = root->first_node(XML_BUFFERS_TAG_NAME);
  if(buffers==NULL) {
    ERROR_LOG("'" << XML_BUFFERS_TAG_NAME <<
        "' tag missing in texture-updater definition file.");
    delete textureUpdater;
    return NULL;
  }
  if(!parseBuffers(textureUpdater, buffers)) {
    delete textureUpdater;
    return NULL;
  }

  DEBUG_LOG("parsing operations.");
  xml_node<> *pipeline = root->first_node(XML_PIPELINE_TAG_NAME);
  if(pipeline==NULL) {
    ERROR_LOG("'" << XML_PIPELINE_TAG_NAME <<
        "' tag missing in texture-updater definition file.");
    delete textureUpdater;
    return NULL;
  }
  list<TextureUpdateOperation*> operations = parseOperations(textureUpdater,pipeline,shaderConfig);
  for(list<TextureUpdateOperation*>::iterator
      it=operations.begin(); it!=operations.end(); ++it)
  {
    textureUpdater->addOperation(*it, GL_FALSE);
  }

  // handle initial operations
  DEBUG_LOG("parsing initial operations.");
  list<TextureUpdateOperation*> initialOperations = parseOperations(textureUpdater,root,shaderConfig);
  for(list<TextureUpdateOperation*>::iterator
      it=initialOperations.begin(); it!=initialOperations.end(); ++it)
  {
    textureUpdater->addOperation(*it, GL_TRUE);
  }

  return textureUpdater;
}

static TextureUpdater* readTextureUpdaterFileXML(MeshState *textureQuad, const string &xmlFile)
{
  DEBUG_LOG("parsing fluid file at '" << xmlFile << "'.");
  ifstream inputfile(xmlFile.c_str());

  vector<char> buffer((istreambuf_iterator<char>(inputfile)),
               istreambuf_iterator<char>( ));
  buffer.push_back('\0');

  return parseTextureUpdaterStringXML(textureQuad, &buffer[0]);
}

#endif // TEXTURE_UPDATER_XML_H_
