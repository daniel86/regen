/*
 * shader-func.cpp
 *
 *  Created on: 01.02.2011
 *      Author: daniel
 */

#include <sstream>
#include <iostream>
#include <stdio.h>

#include "shader-function.h"
#include <ogle/utility/string-util.h>

vector<string> exportArgs(GLSLEquation e) {
  vector<string> args;
  args.push_back(FORMAT_STRING(e.name << " = " << e.value << ";"));
  return args;
}
vector<string> exportArgs(GLSLStatement e) {
  vector<string> args;
  args.push_back(FORMAT_STRING(e.statement));
  return args;
}

class StatementFunction : public ShaderFunctions {
public:
  StatementFunction(GLSLEquation e)
  : ShaderFunctions("STATEMENT", exportArgs(e)) {}
  StatementFunction(GLSLStatement e)
  : ShaderFunctions("STATEMENT", exportArgs(e)) {}
  virtual string code() const { return ""; }
};

ShaderFunctions::ShaderFunctions()
: minVersion_(150),
  funcs_( ),
  funcCodes_( ),
  uniforms_( ),
  constants_( ),
  inputs_( ),
  outputs_( ),
  deps_( ),
  mainVars_( ),
  exports_( ),
  fragmentOutputs_( ),
  enabledExtensions_( ),
  disabledExtensions_( ),
  myName_("")
{
  // add some default uniforms... will be removed if unused
  addUniform( GLSLUniform( "float", "in_deltaT" ) );
  addUniform( GLSLUniform( "float", "in_far" ) );
  addUniform( GLSLUniform( "float", "in_near" ) );
  addUniform( GLSLUniform( "mat4", "in_modelMat" ) );
  addUniform( GLSLUniform( "mat4", "in_viewMatrix" ) );
  addUniform( GLSLUniform( "mat4", "in_viewProjectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "in_projectionMatrix" ) );
}

ShaderFunctions::ShaderFunctions(
    const string &name,
    const vector<string> &args)
: minVersion_(150),
  funcs_( ),
  funcCodes_( ),
  uniforms_( ),
  constants_( ),
  inputs_( ),
  outputs_( ),
  deps_( ),
  mainVars_( ),
  exports_( ),
  fragmentOutputs_( ),
  enabledExtensions_( ),
  disabledExtensions_( ),
  myName_(name)
{
  funcs_.push_back( pair< string, vector<string> >(name,args) );
  // add some default uniforms... will be removed if unused
  addUniform( GLSLUniform( "mat4", "in_modelMat" ) );
  addUniform( GLSLUniform( "mat4", "in_viewMatrix" ) );
  addUniform( GLSLUniform( "mat4", "in_viewProjectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "in_projectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "in_inverseViewMatrix" ) );
  addUniform( GLSLUniform( "mat4", "in_inverseProjectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "in_inverseViewProjectionMatrix" ) );
}

ShaderFunctions::ShaderFunctions(const ShaderFunctions &other)
{
  funcs_ = other.funcs_;
  funcCodes_.insert(
      other.funcCodes_.begin(),
      other.funcCodes_.end());
  if(other.myName_.size() > 0) {
      funcCodes_[other.myName_] = other.code();
  }
  uniforms_ = other.uniforms_;
  constants_ = other.constants_;
  inputs_ = other.inputs_;
  outputs_ = other.outputs_;
  deps_ = other.deps_;
  mainVars_ = other.mainVars_;
  exports_ = other.exports_;
  fragmentOutputs_ = other.fragmentOutputs_;
  enabledExtensions_ = other.enabledExtensions_;
  disabledExtensions_ = other.disabledExtensions_;
  minVersion_ = other.minVersion_;
  tessPrimitive_ = other.tessPrimitive_;
  tessSpacing_ = other.tessSpacing_;
  tessOrdering_ = other.tessOrdering_;
  tessNumVertices_ = other.tessNumVertices_;
}

ShaderFunctions& ShaderFunctions::operator=(const ShaderFunctions &other)
{
  funcs_ = other.funcs_;
  funcCodes_.insert(
      other.funcCodes_.begin(),
      other.funcCodes_.end());
  if(other.myName_.size() > 0) {
      funcCodes_[other.myName_] = other.code();
  }
  uniforms_ = other.uniforms_;
  constants_ = other.constants_;
  inputs_ = other.inputs_;
  outputs_ = other.outputs_;
  deps_ = other.deps_;
  mainVars_ = other.mainVars_;
  exports_ = other.exports_;
  fragmentOutputs_ = other.fragmentOutputs_;
  enabledExtensions_ = other.enabledExtensions_;
  disabledExtensions_ = other.disabledExtensions_;
  minVersion_ = other.minVersion_;
  tessPrimitive_ = other.tessPrimitive_;
  tessSpacing_ = other.tessSpacing_;
  tessOrdering_ = other.tessOrdering_;
  tessNumVertices_ = other.tessNumVertices_;
  return *this;
}

void ShaderFunctions::join(const ShaderFunctions &u,
                const list<string> &followingFunctions)
{
  minVersion_ = max(minVersion_, u.minVersion_);

  unsigned int maxIndex = funcs_.size();
  for(list<string>::const_iterator it = followingFunctions.begin();
              it != followingFunctions.end(); ++it)
  {
    for(unsigned int j=0; j<maxIndex; ++j) {
      if(funcs_[j].first.compare(*it) == 0) {
        maxIndex = j;
        break;
      }
    }
  }
  funcs_.insert(funcs_.begin()+maxIndex, u.funcs_.begin(), u.funcs_.end());

  tessNumVertices_ = max(tessNumVertices_, u.tessNumVertices_);
  enabledExtensions_.insert(u.enabledExtensions().begin(),
              u.enabledExtensions().end());
  disabledExtensions_.insert(u.disabledExtensions().begin(),
              u.disabledExtensions().end());
  for(list<GLSLUniform>::const_iterator
      it=u.uniforms().begin(); it!=u.uniforms().end(); ++it)
  {
    addUniform(*it);
  }
  for(list<GLSLConstant>::const_iterator
      it=u.constants().begin(); it!=u.constants().end(); ++it)
  {
    addConstant(*it);
  }
  for(list<GLSLTransfer>::const_iterator
      it=u.inputs().begin(); it!=u.inputs().end(); ++it)
  {
    addInput(*it);
  }
  for(list<GLSLTransfer>::const_iterator
      it=u.outputs().begin(); it!=u.outputs().end(); ++it)
  {
    addOutput(*it);
  }
  mainVars_.insert(mainVars_.begin(), u.mainVars().begin(), u.mainVars().end());
  exports_.insert(exports_.begin(), u.exports().begin(), u.exports().end());
  fragmentOutputs_.insert(fragmentOutputs_.end(), u.fragmentOutputs().begin(), u.fragmentOutputs().end());
  funcCodes_.insert(u.funcCodes_.begin(), u.funcCodes_.end());
  if(u.myName_.size() > 0) {
      funcCodes_[u.myName_] = u.code();
  }
  for(map<string,string>::const_iterator it = u.deps_.begin();
          it != u.deps_.end(); ++it)
      deps_[it->first] = it->second;
}

void ShaderFunctions::operator+=(const ShaderFunctions &u)
{
  list<string> l;
  join(u, l);
}

void ShaderFunctions::addStatement(GLSLEquation e)
{
  StatementFunction s(e);
  this->operator +=(s);
}
void ShaderFunctions::addStatement(GLSLStatement e)
{
  StatementFunction s(e);
  this->operator +=(s);
}

void ShaderFunctions::setMinVersion(int minVersion)
{
  minVersion_ = max(this->minVersion_,minVersion);
}
int ShaderFunctions::minVersion() const
{
  return minVersion_;
}

void ShaderFunctions::removeShaderInput(const string &name)
{
  if(uniformNames_.count(name)>0) {
    for(list<GLSLUniform>::iterator
        it=uniforms_.begin(); it!=uniforms_.end(); ++it)
    {
      if(name.compare(it->name)==0) {
        uniforms_.erase(it);
        break;
      }
    }
    uniformNames_.erase(name);
  }
  if(constantNames_.count(name)>0) {
    for(list<GLSLConstant>::iterator
        it=constants_.begin(); it!=constants_.end(); ++it)
    {
      if(name.compare(it->name)==0) {
        constants_.erase(it);
        break;
      }
    }
    constantNames_.erase(name);
  }
  if(inputNames_.count(name)>0) {
    for(list<GLSLTransfer>::iterator
        it=inputs_.begin(); it!=inputs_.end(); ++it)
    {
      if(name.compare(it->name)==0) {
        inputs_.erase(it);
        break;
      }
    }
    inputNames_.erase(name);
  }
}
void ShaderFunctions::removeShaderOutput(const string &name)
{
  if(outputNames_.count(name)>0) {
    for(list<GLSLTransfer>::iterator
        it=outputs_.begin(); it!=outputs_.end(); ++it)
    {
      if(name.compare(it->name)==0) {
        outputs_.erase(it);
        break;
      }
    }
    outputNames_.erase(name);
  }
}

void ShaderFunctions::addUniform(const GLSLUniform &uniform)
{
  GLSLUniform namedUni = uniform;
  if(hasPrefix(uniform.name, "in_")) {
    namedUni.name = uniform.name;
  } else if(hasPrefix(uniform.name, "u_")) {
    namedUni.name = FORMAT_STRING("in_" << truncPrefix(uniform.name,"u_"));
  } else {
    namedUni.name = FORMAT_STRING("in_" << uniform.name);
  }
  // remove previously declared input with same name
  removeShaderInput(namedUni.name);
  uniformNames_.insert(namedUni.name);
  uniforms_.push_back(namedUni);
}
const list<GLSLUniform>& ShaderFunctions::uniforms() const
{
  return uniforms_;
}

void ShaderFunctions::addConstant(const GLSLConstant &constant)
{
  GLSLConstant namedConst = constant;
  if(hasPrefix(constant.name, "in_")) {
    namedConst.name = constant.name;
  } else if(hasPrefix(constant.name, "c_")) {
    namedConst.name = FORMAT_STRING("in_" << truncPrefix(constant.name,"c_"));
  } else {
    namedConst.name = FORMAT_STRING("in_" << constant.name);
  }
  // remove previously declared input with same name
  removeShaderInput(namedConst.name);
  constantNames_.insert(namedConst.name);
  constants_.push_back(namedConst);
}
const list<GLSLConstant>& ShaderFunctions::constants() const
{
  return constants_;
}

void ShaderFunctions::addInput(const GLSLTransfer &transfer)
{
  GLSLTransfer namedTransfer = transfer;
  if(hasPrefix(transfer.name, "in_")) {
    namedTransfer.name = transfer.name;
  } else if(hasPrefix(transfer.name, "fs_")) {
    namedTransfer.name = FORMAT_STRING("in_" << truncPrefix(transfer.name,"fs_"));
  } else if(hasPrefix(transfer.name, "gs_")) {
    namedTransfer.name = FORMAT_STRING("in_" << truncPrefix(transfer.name,"gs_"));
  } else if(hasPrefix(transfer.name, "vs_")) {
    namedTransfer.name = FORMAT_STRING("in_" << truncPrefix(transfer.name,"vs_"));
  } else if(hasPrefix(transfer.name, "tes_")) {
    namedTransfer.name = FORMAT_STRING("in_" << truncPrefix(transfer.name,"tes_"));
  } else if(hasPrefix(transfer.name, "tcs_")) {
    namedTransfer.name = FORMAT_STRING("in_" << truncPrefix(transfer.name,"tcs_"));
  } else {
    namedTransfer.name = FORMAT_STRING("in_" << transfer.name);
  }
  // remove previously declared input with same name
  removeShaderInput(namedTransfer.name);
  inputNames_.insert(namedTransfer.name);
  inputs_.push_back(namedTransfer);
}
const list<GLSLTransfer>& ShaderFunctions::inputs() const
{
  return inputs_;
}

void ShaderFunctions::addOutput(const GLSLTransfer &transfer)
{
  GLSLTransfer namedTransfer = transfer;
  if(hasPrefix(transfer.name, "out_")) {
    namedTransfer.name = transfer.name;
  } else if(hasPrefix(transfer.name, "fs_")) {
    namedTransfer.name = FORMAT_STRING("out_" << truncPrefix(transfer.name,"fs_"));
  } else if(hasPrefix(transfer.name, "gs_")) {
    namedTransfer.name = FORMAT_STRING("out_" << truncPrefix(transfer.name,"gs_"));
  } else if(hasPrefix(transfer.name, "vs_")) {
    namedTransfer.name = FORMAT_STRING("out_" << truncPrefix(transfer.name,"vs_"));
  } else if(hasPrefix(transfer.name, "tes_")) {
    namedTransfer.name = FORMAT_STRING("out_" << truncPrefix(transfer.name,"tes_"));
  } else if(hasPrefix(transfer.name, "tcs_")) {
    namedTransfer.name = FORMAT_STRING("out_" << truncPrefix(transfer.name,"tcs_"));
  } else {
    namedTransfer.name = FORMAT_STRING("out_" << transfer.name);
  }
  // remove previously declared input with same name
  removeShaderOutput(namedTransfer.name);
  outputNames_.insert(namedTransfer.name);
  outputs_.push_back(namedTransfer);
}
const list<GLSLTransfer>& ShaderFunctions::outputs() const
{
  return outputs_;
}

void ShaderFunctions::addFragmentOutput(const GLSLFragmentOutput &output)
{
  fragmentOutputs_.push_back(output);
}
const list<GLSLFragmentOutput>& ShaderFunctions::fragmentOutputs() const
{
  return fragmentOutputs_;
}

void ShaderFunctions::set_tessNumVertices(unsigned int tessNumVertices)
{
  tessNumVertices_ = tessNumVertices;
}
unsigned int ShaderFunctions::tessNumVertices() const
{
  return tessNumVertices_;
}

void ShaderFunctions::set_tessPrimitive(TessPrimitive tessPrimitive)
{
  tessPrimitive_ = tessPrimitive;
}
TessPrimitive ShaderFunctions::tessPrimitive() const
{
  return tessPrimitive_;
}

void ShaderFunctions::set_tessSpacing(TessVertexSpacing tessSpacing)
{
  tessSpacing_ = tessSpacing;
}
TessVertexSpacing ShaderFunctions::tessSpacing() const
{
  return tessSpacing_;
}

void ShaderFunctions::set_tessOrdering(TessVertexOrdering tessOrdering)
{
  tessOrdering_ = tessOrdering;
}
TessVertexOrdering ShaderFunctions::tessOrdering() const
{
  return tessOrdering_;
}
void ShaderFunctions::set_gsConfig(GeometryShaderConfig gsConfig)
{
  gsConfig_ = gsConfig;
}
GeometryShaderConfig ShaderFunctions::gsConfig() const
{
  return gsConfig_;
}

void ShaderFunctions::addDependencyCode(
    const string &codeId,
    const string &code)
{
  deps_[codeId] = code;
}
vector< pair<string,string> > ShaderFunctions::deps() const
{
  vector< pair<string,string> > v;
  for(map<string,string>::const_iterator it=deps_.begin();
      it!=deps_.end(); ++it) {
    v.push_back( pair<string,string>(it->first, it->second) );
  }
  return v;
}

void ShaderFunctions::enableExtension(const string &extensionName)
{
  enabledExtensions_.insert(extensionName);
}
const set<string>& ShaderFunctions::enabledExtensions() const
{
  return enabledExtensions_;
}

void ShaderFunctions::disableExtension(const string &extensionName)
{
  disabledExtensions_.insert(extensionName);
}
const set<string>& ShaderFunctions::disabledExtensions() const
{
  return disabledExtensions_;
}

void ShaderFunctions::addMainVar(const GLSLVariable &var)
{
  mainVars_.remove(var);
  mainVars_.push_back(var);
}
const list<GLSLVariable>& ShaderFunctions::mainVars() const
{
  return mainVars_;
}

void ShaderFunctions::addExport(const GLSLExport &e)
{
  exports_.remove(e);
  exports_.push_back(e);
}
const list<GLSLExport>& ShaderFunctions::exports() const
{
  return exports_;
}

string ShaderFunctions::code() const
{
  stringstream s;

  for(map< string,string >::const_iterator it = funcCodes_.begin();
          it != funcCodes_.end(); ++it)
  {
    if(!it->second.empty())
      s << it->second;
  }

  return s.str();
}

vector<string> ShaderFunctions::generateFunctionCalls() const
{
  vector<string> calls;

  for(vector< pair< string,vector<string> > >::const_iterator it = funcs_.begin();
              it != funcs_.end(); ++it)
  {
    if(it->first.compare("STATEMENT")==0) {
      calls.push_back(it->second[0]);
    } else {
      string call = it->first + "( ";
      int numArgs = it->second.size();

      if(numArgs > 0)
      {
        call += it->second[0];
        for(int i=1; i<numArgs; ++i)
        {
          call += ", " + it->second[i];
        }
      }
      call += " );";
      calls.push_back(call);
    }
  }
  return calls;
}

const string ShaderFunctions::worldPositionFromDepth =
"void worldPositionFromDepth(in sampler2D depthTexture, \n"
"    in vec2 texCoord, in mat4 invViewProjection, \n"
"    out vec4 pos0, out vec4 posWorld) \n"
"{\n"
"    // get the depth value at this pixel\n"
"    float depth = texture(depthTexture, texCoord).r;\n"
"    pos0 = vec4(texCoord.x*2 - 1, (1-texCoord.y)*2 - 1, depth, 1);\n"
"    // Transform viewport position by the view-projection inverse.\n"
"    vec4 D = invViewProjection*pos0;\n"
"    // Divide by w to get the world position.\n"
"    posWorld = D/D.w;\n"
"}\n";

const string ShaderFunctions::getRotMat =
"mat4 getRotMat(vec3 rot) {\n"
"    float cx = cos(rot.x), sx = sin(rot.x);\n"
"    float cy = cos(rot.y), sy = sin(rot.y);\n"
"    float cz = cos(rot.z), sz = sin(rot.z);\n"
"    float sxsy = sx*sy;\n"
"    float cxsy = cx*sy;\n"
"    return mat4(\n"
"         cy*cz,  sxsy*cz+cx*sz, -cxsy*cz+sx*sz, 0.0,\n"
"        -cy*sz, -sxsy*sz+cx*cz,  cxsy*sz+sx*cz, 0.0,\n"
"            sy,         -sx*cy,          cx*cy, 0.0,\n"
"           0.0,            0.0,            0.0, 1.0 \n"
"    );\n"
"}\n";
const string ShaderFunctions::textureMS =
    "vec4 textureMS(sampler2DMS tex, vec2 uv, int sampleCount) {\n"
    "    ivec2 iuv = ivec2(uv * textureSize(tex));\n"
    "    vec4 color = vec4 (0.f, 0.f, 0.f, 0.f);\n"
    "    for (int i = 0; i < sampleCount; ++i) {\n"
    "        color += texelFetch(tex, iuv, i);\n"
    "    }\n"
    "    color /= sampleCount;\n"
    "    return color;\n"
    "}\n\n";

const string ShaderFunctions::getCubeUV =
"vec3 getCubeUV(vec3 posScreenSpace, vec3 vertexNormal) {\n"
"    return reflect( -posScreenSpace, vertexNormal );\n"
"}\n\n";
const string ShaderFunctions::getSphereUV =
"vec2 getSphereUV(vec3 posScreenSpace, vec3 vertexNormal) {\n"
"   vec3 r = reflect(normalize(posScreenSpace), vertexNormal);\n"
"   float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );\n"
"   return vec2(r.x/m + 0.5, r.y/m + 0.5);\n"
"}\n\n";
const string ShaderFunctions::getTubeUV =
"vec2 getTubeUV(vec3 posScreenSpace, vec3 vertexNormal) {\n"
"    float PI = 3.14159265358979323846264;\n"
"    vec3 r = reflect(normalize(posScreenSpace), vertexNormal);\n"
"    float u,v;\n"
"    float len = sqrt(r.x*r.x + r.y*r.y);\n"
"    v = (r.z + 1.0f) / 2.0f;\n"
"    if(len > 0.0f) u = ((1.0 - (2.0*atan(r.x/len,r.y/len) / PI)) / 2.0);\n"
"    else u = 0.0f;\n"
"    return vec2(u,v);\n"
"}\n\n";
const string ShaderFunctions::getFlatUV =
"vec2 getFlatUV(vec3 posScreenSpace, vec3 vertexNormal) {\n"
"   vec3 r = reflect(normalize(posScreenSpace), vertexNormal);\n"
"    return vec2( (r.x + 1.0)/2.0, (r.y + 1.0)/2.0);\n"
"}\n\n";

const string ShaderFunctions::posWorldSpaceWithUniforms =
"vec4 getPosWorldSpaceWithUniforms() {\n"
"    return in_modelMat*vec4(in_pos,1.0);\n"
"}\n\n";

/**
 * z-Buffer saves depth non linear.
 * For doing comparison in shaders the sampled depth
 * may be converter to a linear depth using far/near clip distances.
 */
const string ShaderFunctions::linearDepth =
"float linearDepth(float _nonLiniearDepth, float _far, float _near) {\n"
"    float z_n = 2.0 * _nonLiniearDepth - 1.0;\n"
"    return 2.0 * _near * _far / (_far + _near - z_n * (_far - _near));\n"
"}\n\n";

const string worldSpaceBones1 =
"vec4 worldSpaceBones1(vec4 v) {\n"
"\n"
"  return in_boneMatrices[in_boneIndices] * v;\n"
"}\n\n";
const string worldSpaceBones2 =
"vec4 worldSpaceBones2(vec4 v) {\n"
"\n"
"  return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v\n"
"       + in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v;\n"
"}\n\n";
const string worldSpaceBones3 =
"vec4 worldSpaceBones3(vec4 v) {\n"
"\n"
"  return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v\n"
"       + in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v\n"
"       + in_boneWeights.z * in_boneMatrices[in_boneIndices.z] * v;\n"
"}\n\n";
const string worldSpaceBones4 =
"vec4 worldSpaceBones4(vec4 v) {\n"
"\n"
"  return in_boneWeights.x * in_boneMatrices[in_boneIndices.x] * v\n"
"       + in_boneWeights.y * in_boneMatrices[in_boneIndices.y] * v\n"
"       + in_boneWeights.z * in_boneMatrices[in_boneIndices.z] * v\n"
"       + in_boneWeights.w * in_boneMatrices[in_boneIndices.w] * v;\n"
"}\n\n";

string ShaderFunctions::posWorldSpace(
    ShaderFunctions &vertexShader,
    const string &posInput,
    GLuint maxNumBoneWeights)
{
  string worldPos = FORMAT_STRING("vec4(" << posInput << ",1.0)");

  if(maxNumBoneWeights>0) {
    switch(maxNumBoneWeights) {
    case 4:
      worldPos = FORMAT_STRING("worldSpaceBones4( " << worldPos << " )");
      vertexShader.addDependencyCode("worldSpaceBones4", worldSpaceBones4);
      break;
    case 3:
      worldPos = FORMAT_STRING("worldSpaceBones3( " << worldPos << " )");
      vertexShader.addDependencyCode("worldSpaceBones3", worldSpaceBones3);
      break;
    case 2:
      worldPos = FORMAT_STRING("worldSpaceBones2( " << worldPos << " )");
      vertexShader.addDependencyCode("worldSpaceBones2", worldSpaceBones2);
      break;
    case 1:
      worldPos = FORMAT_STRING("worldSpaceBones1( " << worldPos << " )");
      vertexShader.addDependencyCode("worldSpaceBones1", worldSpaceBones1);
      break;
    }
  }

  // FIXME: only if there is a modelMat uniform
  worldPos = FORMAT_STRING("in_modelMat * " << worldPos);

  return worldPos;
}

string ShaderFunctions::norWorldSpace(
    ShaderFunctions &vertexShader,
    const string &norInput,
    GLuint maxNumBoneWeights)
{
  // multiple with upper left 3x3 matrix
  string worldNor = FORMAT_STRING("vec4(" << norInput << ",0.0)");

  if(maxNumBoneWeights>0) {
    switch(maxNumBoneWeights) {
    case 4:
      worldNor = FORMAT_STRING("worldSpaceBones4( " << worldNor << " )");
      break;
    case 3:
      worldNor = FORMAT_STRING("worldSpaceBones3( " << worldNor << " )");
      break;
    case 2:
      worldNor = FORMAT_STRING("worldSpaceBones2( " << worldNor << " )");
      break;
    case 1:
      worldNor = FORMAT_STRING("worldSpaceBones1( " << worldNor << " )");
      break;
    }
  }

  worldNor = FORMAT_STRING("in_modelMat * " << worldNor);

  return FORMAT_STRING("normalize( (" << worldNor << ").xyz )");
}
