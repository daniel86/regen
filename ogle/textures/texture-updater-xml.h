
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
using namespace ogle;

static ref_ptr<TextureUpdateOperation> readTextureUpdateOperationXML(
    TextureUpdater *textureUpdater,
    xml_node<> *node,
    const ShaderState::Config &globalShaderConfig)
{
  string outputName = XMLLoader::readAttribute<string>(node, "out");
  string shaderKey = XMLLoader::readAttribute<string>(node, "shader");

  ref_ptr<FrameBufferObject> buffer = textureUpdater->getBuffer(outputName);
  if(buffer.get()==NULL) {
    ERROR_LOG("no buffer named '" << outputName << "' known.");
    return ref_ptr<TextureUpdateOperation>();
  }

  // load operation
  ref_ptr<TextureUpdateOperation> operation = ref_ptr<TextureUpdateOperation>::manage(new TextureUpdateOperation(buffer));
  // read XML configuration
  operation->operator >>(node);

  // compile shader
  ShaderState::Config shaderConfig(globalShaderConfig);
  XMLLoader::loadShaderConfig(node, shaderConfig);
  operation->createShader(shaderConfig, shaderKey);

  // load uniforms
  for(xml_attribute<>* attr=node->first_attribute(); attr; attr=attr->next_attribute())
  {
    if(!boost::starts_with(attr->name(), "in_")) continue;

    string uniformName = string(attr->name()).substr(3);

    if(operation->shader()->isSampler(uniformName)) {
      ref_ptr<FrameBufferObject> inputBuffer = textureUpdater->getBuffer(attr->value());
      if(inputBuffer.get()==NULL) {
        ERROR_LOG("no buffer named '" << outputName << "' known for operation.");
      } else {
        operation->addInputBuffer(inputBuffer, uniformName);
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

  return operation;
}

#endif // TEXTURE_UPDATER_XML_H_
