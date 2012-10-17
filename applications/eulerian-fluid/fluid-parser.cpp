/*
 * fluid-parser.cpp
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#include "fluid-parser.h"

#include <boost/algorithm/string.hpp>

#include <ogle/utility/logging.h>
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

typedef xml_node<> FluidNode;

static GLint parseValueb(const string &val)
{
  return (
      val == "1" ||
      val == "true" ||
      val == "TRUE" ||
      val == "y" ||
      val == "yes");
}
template <typename T>
static T parseValueVecb(const string &val)
{
  list<string> v;
  boost::split(v, val, boost::is_any_of(","));
  T vec(0.0f);
  int *data = &vec.x;
  int i=0;
  for(list<string>::iterator it=v.begin(); it!=v.end(); ++it)
  {
    if(i==v.size()) { break; }
    data[i] = parseValueb(*it);
    ++i;
  }
  return vec;
}
static Vec2i parseValue2b(const string &val)
{
  return parseValueVecb<Vec2i>(val);
}
static Vec3i parseValue3b(const string &val)
{
  return parseValueVecb<Vec3i>(val);
}
static Vec4i parseValue4b(const string &val)
{
  return parseValueVecb<Vec4i>(val);
}

static GLfloat parseValuef(const string &val)
{
  char *pEnd;
  return strtof(val.c_str(), &pEnd);
}
template <typename T>
static T parseValueVecf(const string &val)
{
  list<string> v;
  boost::split(v, val, boost::is_any_of(","));
  T vec(0.0f);
  float *data = &vec.x;
  int i=0;
  for(list<string>::iterator it=v.begin(); it!=v.end(); ++it)
  {
    if(i==v.size()) { break; }
    data[i] = parseValuef(*it);
    ++i;
  }
  return vec;
}
static Vec2f parseValue2f(const string &val)
{
  return parseValueVecf<Vec2f>(val);
}
static Vec3f parseValue3f(const string &val)
{
  return parseValueVecf<Vec3f>(val);
}
static Vec4f parseValue4f(const string &val)
{
  return parseValueVecf<Vec4f>(val);
}

static GLint parseValuei(const string &val)
{
  char *pEnd;
  return strtoul(val.c_str(), &pEnd, 0);
}
template <typename T>
static T parseValueVeci(const string &val)
{
  list<string> v;
  boost::split(v, val, boost::is_any_of(","));
  T vec(0.0f);
  int *data = &vec.x;
  int i=0;
  for(list<string>::iterator it=v.begin(); it!=v.end(); ++it)
  {
    if(i==v.size()) { break; }
    data[i] = parseValuei(*it);
    ++i;
  }
  return vec;
}
static Vec2i parseValue2i(const string &val)
{
  return parseValueVeci<Vec2i>(val);
}
static Vec3i parseValue3i(const string &val)
{
  return parseValueVeci<Vec3i>(val);
}
static Vec4i parseValue4i(const string &val)
{
  return parseValueVeci<Vec4i>(val);
}

static FluidBuffer::PixelType parsePixelType(const string &val)
{
  if(val == "f16" ||
      val=="16f" ||
      val == "F16" ||
      val == "16F") {
    return FluidBuffer::F16;
  } else if(val == "f32" ||
      val=="32f" ||
      val == "F32" ||
      val == "32F") {
    return FluidBuffer::F32;
  } else if(val == "byte") {
    return FluidBuffer::BYTE;
  } else {
    WARN_LOG("unknown pixel type '" << val << "'.");
    return FluidBuffer::F16;
  }
}
static FluidOperation::Mode parseOperationMode(const string &val)
{
  if(val == "modifyState" || val == "modify") {
    return FluidOperation::MODIFY_STATE;
  } else if(val == "newState" || val == "new") {
    return FluidOperation::NEW_STATE;
  } else {
    WARN_LOG("unknown mode '" << val << "'.");
    return FluidOperation::NEW_STATE;
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

static bool parseBuffers(Fluid *fluid, FluidNode *parent)
{
  FluidNode *child = parent->first_node(XML_BUFFER_TAG_NAME);
  if(child==NULL) {
    ERROR_LOG("'" << XML_BUFFER_TAG_NAME <<
        "' tag missing for fluid '" << fluid->name() << "'.");
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
      fluid->addBuffer(new FluidBuffer(name, tex));
      continue;
    }

    // check if a special texture was requested
    xml_attribute<>* spectrumAtt = child->first_attribute(XML_BUFFER_SPECTRUM_TAG);
    if(spectrumAtt!=NULL) {
      Vec2f params = parseValue2f(spectrumAtt->value());
      cout << "SPECTRUM " << params << " " << name << endl;
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
      fluid->addBuffer(new FluidBuffer(name, tex));
      continue;
    }

    xml_attribute<>* sizeAtt = child->first_attribute(XML_BUFFER_SIZE_TAG);
    if(sizeAtt==NULL) {
      ERROR_LOG("no '" << XML_BUFFER_SIZE_TAG <<
          "' for fluid '" << fluid->name() << "' tag defined.");
      return NULL;
    }
    Vec3i size = parseValue3i(sizeAtt->value());
    if(size.z<1) { size.z=1; }

    xml_attribute<>* dimAtt = child->first_attribute(XML_BUFFER_COMPONENTS_TAG);
    if(dimAtt==NULL) {
      ERROR_LOG("no '" << XML_BUFFER_COMPONENTS_TAG <<
          "' for fluid '" << fluid->name() << "' tag defined.");
      return NULL;
    }
    GLuint dim = parseValuei(dimAtt->value());

    xml_attribute<>* countAtt = child->first_attribute(XML_BUFFER_COUNT_TAG);
    if(countAtt==NULL) {
      ERROR_LOG("no '" << XML_BUFFER_COUNT_TAG <<
          "' for fluid '" << fluid->name() << "' tag defined.");
      return NULL;
    }
    GLuint count = parseValuei(countAtt->value());

    xml_attribute<>* pixelTypeAtt = child->first_attribute(XML_BUFFER_PIXEL_TYPE_TAG);
    FluidBuffer::PixelType pixelType = FluidBuffer::F16;
    if(pixelTypeAtt!=NULL) {
      pixelType = parsePixelType(pixelTypeAtt->value());
    }

    fluid->addBuffer(new FluidBuffer(name, size, dim, count, pixelType));
  }

  return true;
}

////////////////////////////
////////////  PIPELINE  ////
////////////////////////////
////  Example:
////  <operations>
////      <operation name="foo" mode="nextState" blend="add" clear="1" iterations="20" .... />
////      ....
////  </operations>

#define XML_PIPELINE_TAG_NAME "operations"
#define XML_STAGE_TAG_NAME "operation"
#define XML_STAGE_NAME_TAG "name"
#define XML_STAGE_MODE_TAG "mode"
#define XML_STAGE_BLEND_TAG "blend"
#define XML_STAGE_CLEAR_TAG "clearColor"
#define XML_STAGE_ITERATIONS_TAG "iterations"
#define XML_STAGE_INPUT_PREFIX "in_"
#define XML_STAGE_OUTPUT_TAG "out"
#define XML_STAGE_USE_OBSTACLES_TAG "useObstacles"

static FluidOperation* parseOperation(
    Fluid *fluid,
    FluidNode *node)
{
  xml_attribute<>* nameAtt = node->first_attribute(XML_STAGE_NAME_TAG);
  if(nameAtt==NULL) {
    ERROR_LOG("no '" << XML_STAGE_NAME_TAG <<
        "' tag defined for fluid '" << fluid->name() << "'.");
    return NULL;
  }
  xml_attribute<>* outputAtt = node->first_attribute(XML_STAGE_OUTPUT_TAG);
  if(outputAtt==NULL) {
    ERROR_LOG("no '" << XML_STAGE_OUTPUT_TAG <<
        "' tag defined for fluid '" << fluid->name() << "'.");
    return NULL;
  }
  FluidBuffer *buffer = fluid->getBuffer(outputAtt->value());
  if(buffer==NULL) {
    ERROR_LOG("no buffer named '" << outputAtt->value() <<
        "' known for fluid '" << fluid->name() << "'.");
    return NULL;
  }
  DEBUG_LOG("  parsing operation '" << nameAtt->value() << "'.");

  GLboolean is2D = (fluid->is2D());
  GLboolean useObstacles = (fluid->getBuffer("obstacles")!=NULL);
  GLboolean isLiquid = (fluid->isLiquid());
  for (xml_attribute<>* attr=node->first_attribute();
      attr; attr=attr->next_attribute())
  {
    string name = attr->name();
    if(name == XML_STAGE_USE_OBSTACLES_TAG) {
      useObstacles = parseValueb(attr->value());
    }
  }
  FluidOperation *operation = new FluidOperation(
      nameAtt->value(),
      buffer,
      is2D,
      useObstacles,
      isLiquid,
      fluid->timestep(),
      fluid->textureQuad());

  // load operation configuration
  for (xml_attribute<>* attr=node->first_attribute();
      attr; attr=attr->next_attribute())
  {
    string name = attr->name();
    if(name == XML_STAGE_MODE_TAG) {
      operation->set_mode(parseOperationMode(attr->value()));
    } else if(name == XML_STAGE_BLEND_TAG) {
      operation->set_blendMode(parseOperationBlendMode(attr->value()));
    } else if(name == XML_STAGE_ITERATIONS_TAG) {
      operation->set_numIterations(parseValuei(attr->value()));
    } else if(name == XML_STAGE_CLEAR_TAG) {
      operation->set_clearColor(parseValue4f(attr->value()));
    }
  }

  Shader *operationShader = operation->shader();
  if(operationShader==NULL) {
    ERROR_LOG("no shader was loaded for operation '" <<
        operation->name() << "' for fluid '" << fluid->name() << "'.");
    delete operation;
    return NULL;
  }
  map<string, ref_ptr<ShaderInput> > shaderInputs;
  set<string> attributeNames;
  set<string> uniformNames;

  shaderInputs[buffer->inverseSize()->name()] =
      ref_ptr<ShaderInput>::cast(buffer->inverseSize());
  uniformNames.insert(buffer->inverseSize()->name());
  // TODO FLUID PARSER: allow loading const and instanced input
  // TODO FLUID PARSER: better configuration (macro) handling

  glUseProgram(operationShader->id());

  // load uniforms
  for (xml_attribute<>* attr=node->first_attribute();
      attr; attr=attr->next_attribute())
  {
    if(!boost::starts_with(attr->name(), "in_")) { continue; }

    string uniformName = string(attr->name()).substr(3);
    uniformNames.insert(uniformName);

    GLint loc = glGetUniformLocation(operationShader->id(), uniformName.c_str());
    if(loc<0) {
      WARN_LOG("uniform '" << uniformName <<
          "' is not an active uniform name for operation '" <<
          operation->name() << "' for fluid '" << fluid->name() << "'.");
      continue;
    }

    GLint arraySize;
    GLenum type;
    glGetActiveUniform(operationShader->id(), loc, 0, NULL, &arraySize, &type, NULL);
    switch(type) {
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE: {
      FluidBuffer *inputBuffer = fluid->getBuffer(attr->value());
      if(inputBuffer==NULL) {
        ERROR_LOG("no buffer named '" << outputAtt->value() <<
            "' known for operation '" << operation->name() <<
            "' for fluid '" << fluid->name() << "'.");
      } else {
        operation->addInputBuffer(inputBuffer, loc);
      }
      break;
    }
    case GL_FLOAT: {
      ref_ptr<ShaderInput1f> uniform =
          ref_ptr<ShaderInput1f>::manage(new ShaderInput1f(uniformName));
      uniform->setUniformData(parseValuef(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_FLOAT_VEC2: {
      ref_ptr<ShaderInput2f> uniform =
          ref_ptr<ShaderInput2f>::manage(new ShaderInput2f(uniformName));
      uniform->setUniformData(parseValue2f(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_FLOAT_VEC3: {
      ref_ptr<ShaderInput3f> uniform =
          ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(uniformName));
      uniform->setUniformData(parseValue3f(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_FLOAT_VEC4: {
      ref_ptr<ShaderInput4f> uniform =
          ref_ptr<ShaderInput4f>::manage(new ShaderInput4f(uniformName));
      uniform->setUniformData(parseValue4f(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_INT: {
      ref_ptr<ShaderInput1i> uniform =
          ref_ptr<ShaderInput1i>::manage(new ShaderInput1i(uniformName));
      uniform->setUniformData(parseValuei(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_INT_VEC2: {
      ref_ptr<ShaderInput2i> uniform =
          ref_ptr<ShaderInput2i>::manage(new ShaderInput2i(uniformName));
      uniform->setUniformData(parseValue2i(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_INT_VEC3: {
      ref_ptr<ShaderInput3i> uniform =
          ref_ptr<ShaderInput3i>::manage(new ShaderInput3i(uniformName));
      uniform->setUniformData(parseValue3i(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_INT_VEC4: {
      ref_ptr<ShaderInput4i> uniform =
          ref_ptr<ShaderInput4i>::manage(new ShaderInput4i(uniformName));
      uniform->setUniformData(parseValue4i(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_BOOL: {
      ref_ptr<ShaderInput1i> uniform =
          ref_ptr<ShaderInput1i>::manage(new ShaderInput1i(uniformName));
      uniform->setUniformData(parseValueb(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_BOOL_VEC2: {
      ref_ptr<ShaderInput2i> uniform =
          ref_ptr<ShaderInput2i>::manage(new ShaderInput2i(uniformName));
      uniform->setUniformData(parseValue2b(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_BOOL_VEC3: {
      ref_ptr<ShaderInput3i> uniform =
          ref_ptr<ShaderInput3i>::manage(new ShaderInput3i(uniformName));
      uniform->setUniformData(parseValue3b(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    case GL_BOOL_VEC4: {
      ref_ptr<ShaderInput4i> uniform =
          ref_ptr<ShaderInput4i>::manage(new ShaderInput4i(uniformName));
      uniform->setUniformData(parseValue4b(attr->value()));
      shaderInputs[uniformName] = ref_ptr<ShaderInput>::cast(uniform);
      break;
    }
    // TODO FLUID PARSER: LOW: allow matrix types ?
    /*
    case GL_FLOAT_MAT2:
      break;
    case GL_FLOAT_MAT3:
      break;
    case GL_FLOAT_MAT4:
      break;
    */
    default:
      WARN_LOG("uniform type '" << type <<
          "' is unknown for operation '" << operation->name() <<
          "' for fluid '" << fluid->name() << "'.");
      break;
    }
  }

  // check for unhandled uniforms
  GLint count;
  glGetProgramiv(operationShader->id(), GL_ACTIVE_UNIFORMS, &count);
  for(GLint i=0; i<count; ++i) {
    GLint arraySize;
    GLenum type;
    char name[32];
    glGetActiveUniform(operationShader->id(), i, 32, NULL, &arraySize, &type, name);
    if(uniformNames.count(string(name))==0) {
      WARN_LOG("unhandled input '" << name  <<
          "' for operation '" << operation->name() <<
          "' for fluid '" << fluid->name() << "'.");
    }
  }

  operationShader->setupLocations(attributeNames, uniformNames);
  operationShader->setupInputs(shaderInputs);

  return operation;
}

static list<FluidOperation*> parseOperations(Fluid *fluid, FluidNode *parent)
{
  list<FluidOperation*> operations;
  for(FluidNode *child = parent->first_node(XML_STAGE_TAG_NAME);
      child!=NULL;
      child= child->next_sibling(XML_STAGE_TAG_NAME))
  {
    FluidOperation *operation = parseOperation(fluid, child);

    if(operation!=NULL) {
      operations.push_back(operation);
    } else {
      ERROR_LOG(XML_STAGE_TAG_NAME <<
          " failed to parse for fluid '" << fluid->name() << "'.");
    }
  }
  return operations;
}

////////////////////////////
///////////////  FLUID  ////
////////////////////////////
////  Example:
////  <fluid
////      name="fluid-test"
////      size="256,256"
////      isLiquid="false"
////  >
////      <buffers>....</buffers>
////      <operations>....</operations>
////  </fluid>

#define XML_FLUID_TAG_NAME "fluid"
#define XML_FLUID_NAME_TAG "name"
#define XML_FLUID_IS2D_TAG "is2D"
#define XML_FLUID_IS_LIQUID_TAG "isLiquid"
#define XML_FLUID_TIMESTEP_TAG "timestep"
#define XML_FLUID_FRAMERATE_TAG "framerate"
#define XML_FLUID_LIQUID_HEIGHT_TAG "liquidHeight"

Fluid* FluidParser::readFluidFileXML(MeshState *textureQuad, const string &xmlFile)
{
  DEBUG_LOG("parsing fluid file at '" << xmlFile << "'.");
  ifstream inputfile(xmlFile.c_str());

  vector<char> buffer((istreambuf_iterator<char>(inputfile)),
               istreambuf_iterator<char>( ));
  buffer.push_back('\0');

  return parseFluidStringXML(textureQuad, &buffer[0]);
}

Fluid* FluidParser::parseFluidStringXML(MeshState *textureQuad, char *xmlString)
{
  // character type defaults to char
  xml_document<> doc;
  doc.parse<0>(xmlString);

  // load fluid root node
  xml_node<> *root = doc.first_node(XML_FLUID_TAG_NAME);
  if(root==NULL) {
    ERROR_LOG("'" << XML_FLUID_TAG_NAME <<
        "' tag missing in fluid definition file.");
    return NULL;
  }

  xml_attribute<>* nameAtt = root->first_attribute(XML_FLUID_NAME_TAG);
  if(nameAtt==NULL) {
    ERROR_LOG("no '" << XML_FLUID_NAME_TAG << "' tag defined.");
    return NULL;
  }
  string name = nameAtt->value();
  DEBUG_LOG("parsing fluid '" << name << "'.");

  xml_attribute<>* is2DAtt = root->first_attribute(XML_FLUID_IS2D_TAG);
  GLboolean is2D = GL_TRUE;
  if(is2DAtt!=NULL) {
    is2D = parseValueb(is2DAtt->value());
  }

  xml_attribute<>* isLiquidAtt = root->first_attribute(XML_FLUID_IS_LIQUID_TAG);
  GLboolean isLiquid = GL_FALSE;
  if(isLiquidAtt!=NULL) {
    isLiquid = parseValueb(isLiquidAtt->value());
  }

  xml_attribute<>* timestepAtt = root->first_attribute(XML_FLUID_TIMESTEP_TAG);
  GLfloat timestep = 0.015;
  if(timestepAtt!=NULL) {
    timestep = parseValuef(timestepAtt->value());
  }

  Fluid *fluid = new Fluid(name,timestep,isLiquid,is2D);
  fluid->set_textureQuad(textureQuad);

  xml_attribute<>* framerateAtt = root->first_attribute(XML_FLUID_FRAMERATE_TAG);
  if(framerateAtt!=NULL) {
    fluid->set_framerate( parseValuei(framerateAtt->value()) );
  }

  xml_attribute<>* liquidHeightAtt = root->first_attribute(XML_FLUID_LIQUID_HEIGHT_TAG);
  if(liquidHeightAtt!=NULL) {
    GLfloat liquidHeight = parseValuef(liquidHeightAtt->value());
    fluid->setLiquidHeight(liquidHeight);
  }

  DEBUG_LOG("parsing buffers.");
  xml_node<> *buffers = root->first_node(XML_BUFFERS_TAG_NAME);
  if(buffers==NULL) {
    ERROR_LOG("'" << XML_BUFFERS_TAG_NAME <<
        "' tag missing in fluid definition file.");
    delete fluid;
    return NULL;
  }
  if(!parseBuffers(fluid, buffers)) {
    delete fluid;
    return NULL;
  }

  DEBUG_LOG("parsing operations.");
  xml_node<> *pipeline = root->first_node(XML_PIPELINE_TAG_NAME);
  if(pipeline==NULL) {
    ERROR_LOG("'" << XML_PIPELINE_TAG_NAME <<
        "' tag missing in fluid definition file.");
    delete fluid;
    return NULL;
  }
  list<FluidOperation*> operations = parseOperations(fluid, pipeline);
  for(list<FluidOperation*>::iterator
      it=operations.begin(); it!=operations.end(); ++it)
  {
    fluid->addOperation(*it, GL_FALSE);
  }

  // handle initial operations
  DEBUG_LOG("parsing initial operations.");
  list<FluidOperation*> initialOperations = parseOperations(fluid, root);
  for(list<FluidOperation*>::iterator
      it=initialOperations.begin(); it!=initialOperations.end(); ++it)
  {
    fluid->addOperation(*it, GL_TRUE);
  }

  return fluid;
}
