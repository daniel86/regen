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
  addUniform( GLSLUniform( "float", "deltaT" ) );
  addUniform( GLSLUniform( "float", "far" ) );
  addUniform( GLSLUniform( "float", "near" ) );
  addUniform( GLSLUniform( "mat4", "modelMat" ) );
  addUniform( GLSLUniform( "mat4", "viewMatrix" ) );
  addUniform( GLSLUniform( "mat4", "viewProjectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "projectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "projectionMatrixOrtho" ) );
  addUniform( GLSLUniform( "mat4", "projectionUnitMatrixOrtho" ) );
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
  addUniform( GLSLUniform( "mat4", "modelMat" ) );
  addUniform( GLSLUniform( "mat4", "viewMatrix" ) );
  addUniform( GLSLUniform( "mat4", "viewProjectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "projectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "projectionMatrixOrtho" ) );
  addUniform( GLSLUniform( "mat4", "projectionUnitMatrixOrtho" ) );
  addUniform( GLSLUniform( "mat4", "inverseViewMatrix" ) );
  addUniform( GLSLUniform( "mat4", "inverseProjectionMatrix" ) );
  addUniform( GLSLUniform( "mat4", "inverseViewProjectionMatrix" ) );
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
  uniforms_.insert(u.uniforms().begin(), u.uniforms().end());
  constants_.insert(u.constants().begin(), u.constants().end());
  inputs_.insert(u.inputs().begin(), u.inputs().end());
  outputs_.insert(u.outputs().begin(), u.outputs().end());
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

void ShaderFunctions::addInput(const GLSLTransfer &v)
{
  inputs_.insert(v);
}
const set<GLSLTransfer>& ShaderFunctions::inputs() const
{
  return inputs_;
}

void ShaderFunctions::addOutput(const GLSLTransfer &v)
{
  outputs_.insert(v);
}
const set<GLSLTransfer>& ShaderFunctions::outputs() const
{
  return outputs_;
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

void ShaderFunctions::addUniform(const GLSLUniform &uniform)
{
  uniforms_.insert(uniform);
}
const set<GLSLUniform>& ShaderFunctions::uniforms() const
{
  return uniforms_;
}
Uniform* ShaderFunctions::makeUniform(
    const GLSLUniform &uniform, const string &value) const
{
  if(uniform.type == "float") {
    float val = 0.0f;
    if(value != "") sscanf(value.c_str(), "%f", &val);
    UniformFloat *u = new UniformFloat(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "vec2") {
    Vec2f val(0.0f, 0.0f);
    if(value != "") sscanf(value.c_str(), "vec2(%f,%f)", &val.x, &val.y);
    UniformVec2 *u = new UniformVec2(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "vec3") {
    Vec3f val(0.0f, 0.0f, 0.0f);
    if(value != "") sscanf(value.c_str(), "vec3(%f,%f,%f)", &val.x, &val.y, &val.z);
    UniformVec3 *u = new UniformVec3(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "vec4") {
    Vec4f val(0.0f, 0.0f, 0.0f, 0.0f);
    if(value != "") sscanf(value.c_str(), "vec4(%f,%f,%f,%f)", &val.x, &val.y, &val.z, &val.w);
    UniformVec4 *u = new UniformVec4(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "int") {
    int val = 0.0f;
    if(value != "") sscanf(value.c_str(), "%d", &val);
    UniformInt *u = new UniformInt(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "ivec2") {
    Vec2i val(0.0f, 0.0f);
    if(value != "") sscanf(value.c_str(), "ivec2(%d,%d)", &val.x, &val.y);
    UniformIntV2 *u = new UniformIntV2(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "ivec3") {
    Vec3i val(0.0f, 0.0f, 0.0f);
    if(value != "") sscanf(value.c_str(), "ivec3(%d,%d,%d)", &val.x, &val.y, &val.z);
    UniformIntV3 *u = new UniformIntV3(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "ivec4") {
    Vec4i val(0.0f, 0.0f, 0.0f, 0.0f);
    if(value != "") sscanf(value.c_str(), "ivec4(%d,%d,%d,%d)", &val.x, &val.y, &val.z, &val.w);
    UniformIntV4 *u = new UniformIntV4(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "mat3") {
    Mat3f val(
      1,0,0,
      0,1,0,
      0,0,1
    );
    if(value != "")
      sscanf(value.c_str(), "mat3(%f,%f,%f,%f,%f,%f,%f,%f,%f)",
          &val.x[0], &val.x[1], &val.x[2], &val.x[3],
          &val.x[4], &val.x[5], &val.x[6], &val.x[7],
          &val.x[8]);
    UniformMat3 *u = new UniformMat3(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  } else if(uniform.type == "mat4") {
    Mat4f val(
      1,0,0,0,
      0,1,0,0,
      0,0,1,0,
      0,0,0,1
    );
    if(value != "")
      sscanf(value.c_str(), "mat4(%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f)",
          &val.x[ 0], &val.x[ 1], &val.x[ 2], &val.x[ 3],
          &val.x[ 4], &val.x[ 5], &val.x[ 6], &val.x[ 7],
          &val.x[ 8], &val.x[ 9], &val.x[10], &val.x[11],
          &val.x[12], &val.x[13], &val.x[14], &val.x[15]);
    UniformMat4 *u = new UniformMat4(uniform.name, uniform.numElems);
    u->set_value(val);
    return u;
  }
  return NULL;
}

void ShaderFunctions::addConstant(const GLSLConstant &constant)
{
  constants_.insert(constant);
}
const set<GLSLConstant>& ShaderFunctions::constants() const
{
  return constants_;
}

Uniform* ShaderFunctions::constantToUniform(const string &constantName)
{
  for(set<GLSLConstant>::iterator
      it = constants_.begin(); it != constants_.end(); ++it)
  {
    if(it->name == constantName) {
      GLSLConstant c = *it;
      constants_.erase(*it);

      GLSLUniform uniform(c.type, c.name);
      addUniform(uniform);

      return makeUniform(uniform, c.value);
    }
  }
  WARN_LOG("unable to find constant '" <<
      constantName << "' in shader function " << myName_ << ".");
  return NULL;
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

void ShaderFunctions::addFragmentOutput(const GLSLFragmentOutput &output)
{
  fragmentOutputs_.push_back(output);
}
const list<GLSLFragmentOutput>& ShaderFunctions::fragmentOutputs() const
{
  return fragmentOutputs_;
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
"vec4 getPosWorldSpaceWithUniforms(inout mat4 rotMat) {\n"
"    vec4 posWorldSpace;\n"
"    if(useInstance) {\n"
"        rotMat = v_instanceMat;\n"
"        return v_instanceMat*vec4(v_pos,1.0);\n"
"    } else {\n"
"        return modelMat*vec4(v_pos,1.0);\n"
"    }\n"
"}\n\n";

/**
 * z-Buffer saves depth non linear.
 * For doing comparison in shaders the sampled depth
 * may be converter to a linear depth using far/near clip distances.
 */
const string ShaderFunctions::linearDepth =
"float linearDepth(float nonLiniearDepth, float far, float near) {\n"
"    float z_n = 2.0 * nonLiniearDepth - 1.0;\n"
"    return 2.0 * near * far / (far + near - z_n * (far - near));\n"
"}\n\n";

// FIXME: BONES: handle num weights < 4 !!!
const string worldSpaceBones =
"vec4 worldSpaceBones(vec4 v, vec4 weights) {\n"
"\n"
"  vec4 bonePos = weights.x * boneMatrices[v_boneIndices.x] * v;\n"
"      bonePos += weights.y * boneMatrices[v_boneIndices.y] * v;\n"
"      bonePos += weights.z * boneMatrices[v_boneIndices.z] * v;\n"
"      bonePos += weights.w * boneMatrices[v_boneIndices.w] * v;\n"
"  return bonePos;\n"
"}\n\n";

string ShaderFunctions::posWorldSpace(
    ShaderFunctions &vertexShader,
    const string &posInput,
    bool hasInstanceMat,
    bool hasBones)
{
  string worldPos = FORMAT_STRING("vec4(" << posInput << ",1.0)");

  if(hasBones) {
    worldPos = FORMAT_STRING("worldSpaceBones( " << worldPos << ", _boneWeights )");
    vertexShader.addDependencyCode("worldSpaceBones", worldSpaceBones);
  }

  if(hasInstanceMat) {
    worldPos = FORMAT_STRING("v_instanceMat * " << worldPos);
  } else {
    worldPos = FORMAT_STRING("modelMat * " << worldPos);
  }

  return worldPos;
}

string ShaderFunctions::norWorldSpace(
    ShaderFunctions &vertexShader,
    const string &norInput,
    bool hasInstanceMat,
    bool hasBones)
{
  // multiple with upper left 3x3 matrix
  string worldNor = FORMAT_STRING("vec4(" << norInput << ",0.0)");

  if(hasBones) {
    worldNor = FORMAT_STRING("worldSpaceBones( " << worldNor << ", _boneWeights )");
    vertexShader.addDependencyCode("worldSpaceBones", worldSpaceBones);
  }

  if(hasInstanceMat) {
    worldNor = FORMAT_STRING("v_instanceMat * " << worldNor);
  } else {
    worldNor = FORMAT_STRING("modelMat * " << worldNor);
  }

  return FORMAT_STRING("normalize( (" << worldNor << ").xyz )");
}
