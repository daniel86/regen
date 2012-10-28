
#ifndef TEXTURE_UPDATER_XML_H_
#define TEXTURE_UPDATER_XML_H_

#include <boost/algorithm/string.hpp>

#include <ogle/utility/logging.h>
#include <ogle/utility/string-util.h>
#include <ogle/gl-types/shader-input.h>
#include <ogle/gl-types/volume-texture.h>
#include <ogle/textures/image-texture.h>
#include <ogle/textures/spectral-texture.h>

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

static bool readTextureUpdateBuffersXML(TextureUpdater *textureUpdater, TextureUpdateNode *parent)
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
      ref_ptr<Texture> tex = ref_ptr<Texture>::manage(new ImageTexture(fileAtt->value()));
      textureUpdater->addBuffer(new TextureBuffer(name, tex));
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
      TextureBuffer::PixelType pixelType = TextureBuffer::F16;
      if(pixelTypeAtt!=NULL) {
        pixelType = parsePixelType(pixelTypeAtt->value());
      }
      textureUpdater->addBuffer(new TextureBuffer(name, size, dim, count, pixelType));
    }
  }

  return true;
}

static TextureUpdateOperation* readTextureUpdateOperationXML(
    TextureUpdater *textureUpdater,
    TextureUpdateNode *node,
    const map<string,string> &shaderConfig,
    const map<string,string> &updaterConfig)
{
  xml_attribute<>* outputAtt = node->first_attribute("out");
  if(outputAtt==NULL) {
    ERROR_LOG("no 'out' tag defined for texture-updater '" << textureUpdater->name() << "'.");
    return NULL;
  }
  TextureBuffer *buffer = textureUpdater->getBuffer(outputAtt->value());
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
  for(int i=0; i<sizeof(shaderNames)/sizeof(char*); ++i) {
    xml_node<> *shaderNode = node->first_node(shaderNames[i]);
    if(shaderNode!=NULL) {
      operationConfig[shaderNames[i]] = shaderNode->value();
    }
  }

  TextureUpdateOperation *operation = new TextureUpdateOperation(
      buffer, textureUpdater->textureQuad(), operationConfig, opShaderConfig);

  Shader *operationShader = operation->shader();
  if(operationShader==NULL) {
    ERROR_LOG("no shader was loaded for operation '" <<
        operation->name() << "' for texture-updater '" << textureUpdater->name() << "'.");
    delete operation;
    return NULL;
  }

  ref_ptr<ShaderInput> inverseSize = ref_ptr<ShaderInput>::cast(buffer->inverseSize());
  operationShader->setInput(inverseSize);

  // load uniforms
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
            "' is not an active uniform name for operation '" <<
            operation->name() << "' for texture-updater '" << textureUpdater->name() << "'.");
      }
    }
  }

  return operation;
}

static list<TextureUpdateOperation*> readTextureUpdateOperationsXML(
    TextureUpdater *textureUpdater,
    TextureUpdateNode *parent,
    const map<string,string> &shaderConfig,
    const map<string,string> &updaterConfig)
{
  list<TextureUpdateOperation*> operations;
  for(TextureUpdateNode *child = parent->first_node("operation");
      child!=NULL;
      child= child->next_sibling("operation"))
  {
    TextureUpdateOperation *operation =
        readTextureUpdateOperationXML(textureUpdater, child, shaderConfig, updaterConfig);

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

  map<string,string> shaderConfig, updaterConfig;
  shaderConfig["GLSL_VERSION"] = "150";
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
        readTextureUpdateOperationsXML(textureUpdater,operationsNode,shaderConfig,updaterConfig);
    for(list<TextureUpdateOperation*>::iterator
        it=initialOperations.begin(); it!=initialOperations.end(); ++it)
    {
      textureUpdater->addOperation(*it, GL_TRUE);
    }
  }
  operationsNode = root->first_node("loop");
  if(operationsNode!=NULL) {
    list<TextureUpdateOperation*> operations =
        readTextureUpdateOperationsXML(textureUpdater,operationsNode,shaderConfig,updaterConfig);
    for(list<TextureUpdateOperation*>::iterator
        it=operations.begin(); it!=operations.end(); ++it)
    {
      textureUpdater->addOperation(*it, GL_FALSE);
    }
  }
}


static void writeTextureUpdaterBufferXML(
    TextureUpdater *updater, TextureBuffer *buffer, ostream& out)
{
  Texture *tex = buffer->texture().get();
  out << "        <buffer" << endl;
  out << "            name=\"" << buffer->name() << "\"" << endl;
  if(dynamic_cast<SpectralTexture*>(tex)!=NULL) {
    SpectralTexture *spectrum = (SpectralTexture*)tex;
    out << "            spectrum=\"" <<
        spectrum->t1() << "," << spectrum->t2() <<"\"" << endl;
  }
  else if(dynamic_cast<ImageTexture*>(tex)!=NULL) {
    ImageTexture *image = (ImageTexture*)tex;
    out << "            file=\"" << image->imageFile() <<"\"" << endl;
  }
  else {
    out << "            count=\"" << tex->numBuffers() << "\"" << endl;
    switch (tex->format()) {
    case GL_RED:
      out << "            components=\"1\"" << endl;
      break;
    case GL_RG:
      out << "            components=\"2\"" << endl;
      break;
    case GL_RGB:
      out << "            components=\"3\"" << endl;
      break;
    case GL_RGBA:
    default:
      out << "            components=\"4\"" << endl;
      break;
    }
    switch (tex->pixelType()) {
    case GL_FLOAT:
      out << "            pixelType=\"32F\"" << endl;
      break;
    case GL_HALF_FLOAT:
      out << "            pixelType=\"16F\"" << endl;
      break;
    case GL_BYTE:
    default:
      out << "            pixelType=\"byte\"" << endl;
      break;
    }

    if(dynamic_cast<Texture3D*>(tex)!=NULL) {
      Texture3D *tex3D = (Texture3D*)tex;
      Vec3i size(tex->width(),tex->height(),tex3D->numTextures());
      out << "            size=\"" << size << "\"" << endl;
    } else {
      Vec2i size(tex->width(),tex->height());
      out << "            size=\"" << size << "\"" << endl;
    }
  }

  out << "        />" << endl;
}
static void writeTextureUpdaterOperationXML(
    TextureUpdater *updater,
    TextureUpdateOperation *op,
    map<string,string> &globalShaderConfig,
    ostream& out)
{
  out << "        <operation" << endl;

  // shader stages
  map<string,string> inlineShaders;
  for(map<GLenum,string>::iterator
      it=op->shaderNames().begin(); it!=op->shaderNames().end(); ++it)
  {
    if(boost::contains(it->second, "\n")) {
      switch(it->first) {
      case GL_FRAGMENT_SHADER:
        inlineShaders["fs"] = it->second;
        break;
      case GL_VERTEX_SHADER:
        inlineShaders["vs"] = it->second;
        break;
      case GL_GEOMETRY_SHADER:
        inlineShaders["gs"] = it->second;
        break;
      case GL_TESS_EVALUATION_SHADER:
        inlineShaders["tes"] = it->second;
        break;
      case GL_TESS_CONTROL_SHADER:
        inlineShaders["tcs"] = it->second;
        break;
      }
    } else {
      switch(it->first) {
      case GL_FRAGMENT_SHADER:
        out << "            fs=\"" << it->second << "\"" << endl;
        break;
      case GL_VERTEX_SHADER:
        out << "            vs=\"" << it->second << "\"" << endl;
        break;
      case GL_GEOMETRY_SHADER:
        out << "            gs=\"" << it->second << "\"" << endl;
        break;
      case GL_TESS_EVALUATION_SHADER:
        out << "            tes=\"" << it->second << "\"" << endl;
        break;
      case GL_TESS_CONTROL_SHADER:
        out << "            tcs=\"" << it->second << "\"" << endl;
        break;
      }
    }
  }

  if(op->blendMode()!=BLEND_MODE_SRC) {
    out << "            blend=\"" << op->blendMode() << "\"" << endl;
  }
  if(op->clear()==GL_TRUE) {
    out << "            clearColor=\"" << op->clearColor() << "\"" << endl;
  }
  if(op->numIterations()>1) {
    out << "            iterations=\"" << op->numIterations() << "\"" << endl;
  }

  map<string,string> &shaderCfg = op->shaderConfig();
  for(map<string,string>::iterator
      it=shaderCfg.begin(); it!=shaderCfg.end(); ++it)
  {
    map<string,string>::iterator needle = globalShaderConfig.find(it->first);
    if(needle==globalShaderConfig.end() || needle->second!=it->second) {
      out << "            " << it->first << "=\"" << it->second << "\"" << endl;
    }
  }

  if(op->shader()!=NULL) {
    const map<string, ref_ptr<ShaderInput> > &input = op->shader()->inputs();

    // FIXME: global inputs added here (for example inverse grid size)
    for(map<string, ref_ptr<ShaderInput> >::const_iterator
        it=input.begin(); it!=input.end(); ++it)
    {
      const ref_ptr<ShaderInput> &inRef = it->second;
      if(!inRef->hasData()) continue;
      const ShaderInput &in = *(inRef.get());
      out << "            in_" << inRef->name() << "=\"";
      in >> out;
      out << "\"" << endl;
    }
  }

  for(list<TextureUpdateOperation::PositionedTextureBuffer>::iterator
      it=op->inputBuffer().begin(); it!=op->inputBuffer().end(); ++it)
  {
    TextureUpdateOperation::PositionedTextureBuffer &buffer = *it;
    out << "            in_" << buffer.nameInShader << "=\"" << buffer.buffer->name() << "\"" << endl;
  }

  if(op->outputBuffer()!=NULL) {
    out << "            out=\"" << op->outputBuffer()->name() << "\"" << endl;
  }

  if(inlineShaders.empty()) {
    out << "        />" << endl;
  } else {
    out << "        >" << endl;
    for(map<string,string>::iterator
        it=inlineShaders.begin(); it!=inlineShaders.end(); ++it)
    {
      out << "            <" << it->first << ">";
      out << it->second;
      out << "            </" << it->first << ">" << endl;
    }
    out << "        </operation>" << endl;
  }
}
static void findShaderCfgSubset(
    map<string,string> &globalShaderConfig, TextureUpdateOperation *op)
{
  map<string,string> &opShaderCfg = op->shaderConfig();
  if(globalShaderConfig.empty()) {
    globalShaderConfig = opShaderCfg;
  } else {
    for(map<string,string>::iterator
        it=globalShaderConfig.begin(); it!=globalShaderConfig.end(); ++it)
    {
      if(it->second != opShaderCfg[it->first]) {
        globalShaderConfig.erase(it);
      }
    }
  }
}
static void writeTextureUpdaterXML(TextureUpdater *updater, ostream& os)
{
  os << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;
  os << "<texture-updater" << endl;
  os << "    name=\"" << updater->name() << "\"" << endl;
  os << "    framerate=\"" << updater->framerate() << "\"" << endl;

  map<string,string> globalShaderConfig;
  for(list<TextureUpdateOperation*>::iterator
      it=updater->initialOperations().begin(); it!=updater->initialOperations().end(); ++it)
  {
    findShaderCfgSubset(globalShaderConfig,*it);
  }
  for(list<TextureUpdateOperation*>::iterator
      it=updater->operations().begin(); it!=updater->operations().end(); ++it)
  {
    findShaderCfgSubset(globalShaderConfig,*it);
  }
  for(map<string,string>::iterator
      it=globalShaderConfig.begin(); it!=globalShaderConfig.end(); ++it)
  {
    os << "    " << it->first << "=\"" << it->second << "\"" << endl;
  }

  os << ">" << endl;

  os << "    <buffers>" << endl;
  for(map<string,TextureBuffer*>::iterator
      it=updater->buffers().begin(); it!=updater->buffers().end(); ++it)
  {
    TextureBuffer *buffer = it->second;
    writeTextureUpdaterBufferXML(updater, buffer, os);
  }
  os << "    </buffers>" << endl;

  if(!updater->initialOperations().empty()) {
    os << "    <init>" << endl;
    for(list<TextureUpdateOperation*>::iterator
        it=updater->initialOperations().begin(); it!=updater->initialOperations().end(); ++it)
    {
      writeTextureUpdaterOperationXML(updater,*it,globalShaderConfig,os);
    }
    os << "    </init>" << endl;
  }

  if(!updater->operations().empty()) {
    os << "    <loop>" << endl;
    for(list<TextureUpdateOperation*>::iterator
        it=updater->operations().begin(); it!=updater->operations().end(); ++it)
    {
      writeTextureUpdaterOperationXML(updater,*it,globalShaderConfig,os);
    }
    os << "    </loop>" << endl;
  }
  os << "</texture-updater>" << endl;
}

#endif // TEXTURE_UPDATER_XML_H_
