/*
 * shader-manager.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <iostream>
#include <string>
using namespace std;

#include <boost/algorithm/string.hpp>
#include <stdio.h>

#include <ogle/shader/shader-manager.h>
#include <ogle/shader/normal-mapping.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/utility/logging.h>
#include <ogle/utility/string-util.h>

static bool isVarCharacter(char c)
{
  return
      isalnum(c) ||
      c=='_';
}

static string tessPrimitiveStr(TessPrimitive val)
{
  switch(val) {
  case TESS_PRIMITVE_TRIANGLES:
    return "triangles";
  case TESS_PRIMITVE_QUADS:
    return "quads";
  case TESS_PRIMITVE_ISOLINES:
    return "isolines";
  default: return "unknownValue";
  }
}
static string tessSpacingStr(TessVertexSpacing val)
{
  switch(val) {
  case TESS_SPACING_EQUAL:
    return "equal_spacing";
  case TESS_SPACING_FRACTIONAL_EVEN:
    return "fractional_even_spacing";
  case TESS_SPACING_FRACTIONAL_ODD:
    return "fractional_odd_spacing";
  default: return "unknownValue";
  }
}
static string tessOrderingStr(TessVertexOrdering val)
{
  switch(val) {
  case TESS_ORDERING_CCW:
    return "ccw";
  case TESS_ORDERING_CW:
    return "cw";
  case TESS_ORDERING_POINT_MODE:
    return "point_mode";
  default: return "unknownValue";
  }
}
static string geometryInputStr(GeometryShaderInput val)
{
  switch(val) {
  case GS_INPUT_POINTS:
    return "points";
  case GS_INPUT_LINES:
    return "lines";
  case GS_INPUT_LINES_ADJACENCY:
    return "lines_adjacency";
  case GS_INPUT_TRIANGLES:
    return "triangles";
  case GS_INPUT_TRIANGLES_ADJACENCY:
    return "triangles_adjacency";
  default: return "unknownValue";
  }
}
static string geometryOutputStr(GeometryShaderOutput val)
{
  switch(val) {
  case GS_OUTPUT_POINTS:
    return "points";
  case GS_OUTPUT_LINE_STRIP:
    return "line_strip";
  case GS_OUTPUT_TRIANGLE_STRIP:
    return "triangle_strip";
  default: return "unknownValue";
  }
}
static string stageToPrefix(GLenum val)
{
  switch(val) {
  case GL_VERTEX_SHADER:
    return "vs";
  case GL_FRAGMENT_SHADER:
    return "fs";
  case GL_GEOMETRY_SHADER:
    return "gs";
  case GL_TESS_EVALUATION_SHADER:
    return "tes";
  case GL_TESS_CONTROL_SHADER:
    return "tcs";
  default: return "unknownValue";
  }
}
static string interpolationStr(FragmentInterpolation val)
{
  switch(val) {
  case FRAGMENT_INTERPOLATION_FLAT:
    return "flat";
  case FRAGMENT_INTERPOLATION_NOPERSPECTIVE:
    return "noperspective";
  case FRAGMENT_INTERPOLATION_SMOOTH:
    return "smooth";
  case FRAGMENT_INTERPOLATION_CENTROID:
    return "centroid";
  case FRAGMENT_INTERPOLATION_DEFAULT:
    return "";
  default: return "";
  }
}

static string mainFunction(const ShaderFunctions &f)
{
  stringstream main;
  main << "void main()" << endl;
  main << "{" << endl;
  // add vars to the top of main function
  for(list<GLSLVariable>::const_iterator it = f.mainVars().begin(); it != f.mainVars().end(); ++it)
  {
    main << "    " << it->type << " " << it->name;
    if(!it->value.empty()) {
      main << " = " << it->value;
    }
    main << ";" << endl;
  }
  main << endl;
  // call configured shader functions
  vector<string> calls = f.generateFunctionCalls();
  for(vector<string>::iterator it = calls.begin(); it != calls.end(); ++it)
  {
    main << "    " << *it << endl;
  }
  main << endl;
  // and set the output vars
  for(list<GLSLExport>::const_iterator it = f.exports().begin(); it != f.exports().end(); ++it)
  {
    main << "    " << it->name << " = " << it->value << ";" << endl;
  }
  main << "}" << endl;
  return main.str();
}

GLboolean ShaderManager::containsInputVar(
    const string &var,
    const string &codeStr)
{
  size_t inSize = var.size();
  size_t start = 0;
  size_t pos;

  while( (pos = codeStr.find(var,start)) != string::npos )
  {
    start += pos + var.size();
    size_t end = pos + inSize;
    // check if character before and after are not part of the var
    if(pos>0 && isVarCharacter(codeStr[pos-1])) continue;
    if(end<codeStr.size() && isVarCharacter(codeStr[end])) continue;
    return true;
  }
  return false;
}

GLboolean ShaderManager::containsInputVar(
    const string &var,
    const ShaderFunctions &f)
{
  return containsInputVar( var, f.code() ) ||
      containsInputVar( var, mainFunction(f) );
}

list<string> ShaderManager::getValidTransformFeedbackNames(
    const map<GLenum, string> &shaderStages,
    const list< ref_ptr<VertexAttribute> > &tfAttributes)
{
  static const string prefixes[] = {"gl", "fs", "tcs", "tes", "gs"};

  list<string> tfNames;
  map<GLenum, string>::const_iterator
    vsIt = shaderStages.find(GL_VERTEX_SHADER);
  if(vsIt == shaderStages.end()) { return tfNames; }
  const string &vs = vsIt->second;

  // setup transform feedback varyings,
  // must be done before linking
  // sum attribute element sizes
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      it = tfAttributes.begin(); it != tfAttributes.end(); ++it)
  {
    const VertexAttribute *att = it->get();
    if(att->name().size()==0) { continue; }
    // find attribute in vertex shader
    string name = "";
    for(int i=0; i<4; ++i) {
      string attExport = FORMAT_STRING(prefixes[i] << "_" << att->name());
      if(containsInputVar(attExport, vs)) {
        name = attExport;
        break;
      }
    }
    if(name.size()==0) {
      WARN_LOG("transform feedback: attribute '" <<
          att->name() << "' not found in shader stages.");
    } else {
      tfNames.push_back( name );
    }
  }
  return tfNames;
}

static string getNameWithoutPrefix(
    const string &varName,
    const string &varPrefix)
{
  if(!hasPrefix(varName, varPrefix))
  {
    WARN_LOG("variable " << varName << " has no " << varPrefix << " prefix.");
    return varName;
  }
  else
  {
    return truncPrefix(varName, varPrefix);
  }
}

GLboolean ShaderManager::replaceVariable(
    const string &varName,
    const string &varPrefix,
    const string &desiredPrefix,
    string *code)
{
  string nameWithoutPrefix = getNameWithoutPrefix(varName, varPrefix);

  size_t inSize = varName.size();
  size_t start = 0;
  size_t pos;
  list<size_t> replaced;
  string &codeStr = *code;
  string prefixVar = FORMAT_STRING(desiredPrefix << nameWithoutPrefix);

  while( (pos = codeStr.find(varName,start)) != string::npos )
  {
    start = pos+1;
    size_t end = pos + inSize;
    // check if character before and after are not part of the var
    if(pos>0 && isVarCharacter(codeStr[pos-1])) continue;
    if(end<codeStr.size() && isVarCharacter(codeStr[end])) continue;
    // remember replacement
    replaced.push_front( pos );
  }

  // adding last replacement first, this way indices stay valid
  for(list<size_t>::iterator it=replaced.begin(); it!=replaced.end(); ++it)
  {
    codeStr.replace( *it, inSize, prefixVar );
  }

  return !replaced.empty();
}

string ShaderManager::generateSource(
    const ShaderFunctions &functions,
    GLenum shaderStage,
    GLenum nextShaderStage)
{
  stringstream code, functionCode;

  string inputPrefix = stageToPrefix(shaderStage);
  string outputPrefix = stageToPrefix(nextShaderStage);

  // setup other functions
  vector< pair<string,string> > deps = functions.deps();
  for(vector< pair<string,string> >::iterator
      it = deps.begin(); it != deps.end(); ++it)
  {
    functionCode << it->second;
  }
  functionCode << functions.code() << endl;
  functionCode << endl;
  string codeStr = functionCode.str() + "\n\n" + mainFunction(functions);

  list<GLSLUniform> uniforms = functions.uniforms();
  list<GLSLConstant> constants = functions.constants();
  list<GLSLTransfer> inputs = functions.inputs();
  list<GLSLTransfer> outputs = functions.outputs();

  // uniforms, constants and input attributes are prefixed with 'in_'
  // output attributes with 'out_'. Here we replace 'in_' for uniforms
  // with 'u_', for constants with 'c_' and for attributes 'in_' is replaced with the stage
  // prefix and 'out_' with the prefix of the following stage.
  // This allows implementing functions that do not know if a input var is a uniform,constant
  // or an attribute. The 'in_' 'out_' prefix also allows that stages can be implemented
  // without knowing the following stage (avoiding name collisions)
  {
    // replace every occurrence of variables 'in_$name' with 'u_$name'
    for(list<GLSLUniform>::iterator it=uniforms.begin(); it!=uniforms.end();)
    {
      if(!replaceVariable(it->name, "in_", "u_", &codeStr)) {
        uniforms.erase(it++);
      } else {
        ++it;
      }
    }
    // replace every occurrence of variables 'in_$name' with 'c_$name'
    for(list<GLSLConstant>::iterator it=constants.begin(); it!=constants.end();)
    {
      if(!replaceVariable(it->name, "in_", "c_", &codeStr)) {
        constants.erase(it++);
      } else {
        ++it;
      }
    }
    // replace every occurrence of variables 'in_$name' with '$stagePrefix$name'
    for(list<GLSLTransfer>::iterator it=inputs.begin(); it!=inputs.end();)
    {
      if(!replaceVariable(it->name, "in_", FORMAT_STRING(inputPrefix<<"_"), &codeStr)) {
        inputs.erase(it++);
      } else {
        ++it;
      }
    }
    // replace every occurrence of variables 'out_$name' with '$nextStagePrefix$name'
    for(list<GLSLTransfer>::iterator it=outputs.begin(); it!=outputs.end();)
    {
      if(!replaceVariable(it->name, "out_", FORMAT_STRING(outputPrefix<<"_"), &codeStr)) {
        outputs.erase(it++);
      } else {
        ++it;
      }
    }
  }

  // set version
  if(functions.minVersion() != 0) {
    code << "#version " << functions.minVersion() << endl;
  }
  // enable/disable extensions
  for(set<string>::iterator
      it = functions.enabledExtensions().begin();
      it != functions.enabledExtensions().end(); ++it)
  {
    code << "#extension " << *it << " : enable" << endl;
  }
  for(set<string>::iterator
      it = functions.disabledExtensions().begin();
      it != functions.disabledExtensions().end(); ++it)
  {
    code << "#extension " << *it << " : disable" << endl;
  }
  code << endl;

  // setup layout
  switch(shaderStage) {
  case GL_TESS_CONTROL_SHADER:
    code << "layout("; {
      code << "vertices = " << functions.tessNumVertices();
    } code << ") out;" << endl;
    break;
  case GL_TESS_EVALUATION_SHADER:
    code << "layout("; {
      code << tessPrimitiveStr(functions.tessPrimitive()) << ", ";
      code << tessSpacingStr(functions.tessSpacing()) << ", ";
      code << tessOrderingStr(functions.tessOrdering());
    } code << ") in;" << endl;
    break;
  case GL_GEOMETRY_SHADER:
    code << "layout("; {
      code << geometryInputStr(functions.gsConfig().input);
      if(functions.gsConfig().invocations > 1) {
        code << ", invocations = " << functions.gsConfig().invocations;
      }
    } code << ") in;" << endl;

    code << "layout("; {
      code << geometryOutputStr(functions.gsConfig().output);
      code << ", max_vertices = " << functions.gsConfig().maxVertices;
    } code << ") out;" << endl;
    break;
  default:
    break;
  }
  code << endl;

  // setup constant/uniform inputs
  for(list<GLSLUniform>::const_iterator it=uniforms.begin(); it!=uniforms.end(); ++it)
  {
    string nameWithoutPrefix = getNameWithoutPrefix(it->name, "in_");
    code << "uniform " << it->type << " u_" << nameWithoutPrefix;
    if(it->numElems>1 || it->forceArray) { code << "[" << it->numElems << "]"; }
    code << ";" << endl;
  }
  for(list<GLSLConstant>::const_iterator it=constants.begin(); it!=constants.end(); ++it)
  {
    string nameWithoutPrefix = getNameWithoutPrefix(it->name, "in_");
    code << "const " << it->type << " c_" << nameWithoutPrefix;
    if(it->numElems>1 || it->forceArray) { code << "[" << it->numElems << "]"; }
    code << " = " << it->value << ";" << endl;
  }

  // setup attribute/varying inputs
  switch(shaderStage) {
  case GL_TESS_EVALUATION_SHADER:
    code << "in Tess" << endl;
    code << "{" << endl;
    for(list<GLSLTransfer>::const_iterator it=inputs.begin(); it!=inputs.end(); ++it)
    {
      string nameWithoutPrefix = getNameWithoutPrefix(it->name, "in_");
      code << "    ";
      code << interpolationStr(it->interpolation) << " ";
      code << it->type << " " << inputPrefix << "_" << nameWithoutPrefix;
      if(it->numElems>1 || it->forceArray) { code << "[" << it->numElems << "]"; }
      code << ";" << endl;
    }
    code << "} In[];" << endl;
    break;
  case GL_TESS_CONTROL_SHADER:
    for(list<GLSLTransfer>::const_iterator it=inputs.begin(); it!=inputs.end(); ++it)
    {
      string nameWithoutPrefix = getNameWithoutPrefix(it->name, "in_");
      code << interpolationStr(it->interpolation) << " ";
      code << "in " << it->type << " " <<
          inputPrefix << "_" << nameWithoutPrefix << "[];" << endl;
    }
    break;
  default:
    for(list<GLSLTransfer>::const_iterator it=inputs.begin(); it!=inputs.end(); ++it)
    {
      string nameWithoutPrefix = getNameWithoutPrefix(it->name, "in_");
      code << interpolationStr(it->interpolation) << " ";
      code << "in " << it->type << " " << inputPrefix << "_" << nameWithoutPrefix;
      if(it->numElems>1 || it->forceArray) { code << "[" << it->numElems << "]"; }
      code << ";" << endl;
    }
    break;
  }

  // setup outputs
  switch(shaderStage) {
  case GL_FRAGMENT_SHADER:
    for(list<GLSLFragmentOutput>::const_iterator
        it=functions.fragmentOutputs().begin(); it!=functions.fragmentOutputs().end(); ++it)
    {
      code << "out " << it->type << " " << it->name << ";" << endl;
    }
    break;
  case GL_TESS_CONTROL_SHADER:
    if(functions.outputs().size() > 0) {
      code << "out Tess" << endl;
      code << "{" << endl;
      for(list<GLSLTransfer>::const_iterator it=outputs.begin(); it!=outputs.end(); ++it)
      {
        string nameWithoutPrefix = getNameWithoutPrefix(it->name, "out_");
        code << "    ";
        code << interpolationStr(it->interpolation) << " ";
        code << it->type << " " << outputPrefix << "_" << nameWithoutPrefix;
        if(it->numElems>1 || it->forceArray) { code << "[" << it->numElems << "]"; }
        code << ";" << endl;
      }
      code << "} Out[];" << endl;
    }
    break;
  default:
    for(list<GLSLTransfer>::const_iterator it=outputs.begin(); it!=outputs.end(); ++it)
    {
      string nameWithoutPrefix = getNameWithoutPrefix(it->name, "out_");
      code << interpolationStr(it->interpolation) << " ";
      code << "out " << it->type << " " << outputPrefix << "_" << nameWithoutPrefix;
      if(it->numElems>1 || it->forceArray) { code << "[" << it->numElems << "]"; }
      code << ";" << endl;
    }
    break;
  }
  code << endl;

  // add dependency functions
  code << codeStr;

  return code.str();
}
