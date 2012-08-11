/*
 * shader-manager.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <config.h>

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

static string mainFunction(const ShaderFunctions &f)
{
  stringstream main;
  main << "void main()" << endl;
  main << "{" << endl;
  // add vars to the top of main function
  for(list<GLSLVariable>::const_iterator it = f.mainVars().begin(); it != f.mainVars().end(); ++it)
  {
    main << "    " << *it << ";" << endl;
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
    main << "    " << *it << ";" << endl;
  }
  main << "}" << endl;
  return main.str();
}

static bool containsInputVar_(
    const string &var, const string &codeStr)
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

bool ShaderManager::containsInputVar(
    const string &var, const ShaderFunctions &f)
{
  if(containsInputVar_( var, f.code() )) return true;
  if(containsInputVar_( var, mainFunction(f) )) return true;
  return false;
}

void ShaderManager::replaceInputVar(const string &var, const string &prefix, string *code)
{
  // replace every term '$var' with '$prefix_$var' in code

  size_t inSize = var.size();
  size_t start = 0;
  size_t pos;
  list<size_t> replaced;
  string &codeStr = *code;
  string prefixVar = FORMAT_STRING(prefix << "_" << var);

  while( (pos = codeStr.find(var,start)) != string::npos )
  {
    start += pos + var.size();
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
}

list<string> ShaderManager::getValidTransformFeedbackNames(
    const map<GLenum, ShaderFunctions> shaderStages,
    const map<string,VertexAttribute*> &tfAttributes)
{
  static const string prefixes[] = {"gl", "f", "tcs", "tes", "g"};

  list<string> tfNames;
  map<GLenum, ShaderFunctions>::const_iterator
    vsIt = shaderStages.find(GL_VERTEX_SHADER);
  if(vsIt == shaderStages.end()) { return tfNames; }
  const ShaderFunctions &vs = vsIt->second;

  // setup transform feedback varyings,
  // must be done before linking
  // sum attribute element sizes
  for(map<string,VertexAttribute*>::const_iterator
      it = tfAttributes.begin(); it != tfAttributes.end(); ++it)
  {
    const VertexAttribute *att = it->second;
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

string ShaderManager::generateSource(
    const ShaderFunctions &functions, GLenum shaderType)
{
  stringstream code, functionCode;
  set<GLSLUniform> uniforms = functions.uniforms();
  set<string> inputNames;
  string inputPrefix = "";

  // collect input names without prefix and extract the shader input prefix
  for(set<GLSLTransfer>::iterator
      it = functions.inputs().begin();
      it != functions.inputs().end(); ++it)
  {
    const string &name = it->name;
    unsigned int underscoreIndex;
    for(underscoreIndex=0; underscoreIndex<name.size(); ++underscoreIndex) {
      if(name[underscoreIndex] == '_') break;
    }
    if(underscoreIndex == name.size()) {
      WARN_LOG("no underscore found in input var " << *it);
      inputNames.insert(name);
    } else {
      inputNames.insert(name.substr( underscoreIndex+1, name.size()-underscoreIndex ));
      string inputPrefix_ = name.substr( 0, underscoreIndex );
      if(inputPrefix.size() == 0) {
        inputPrefix = inputPrefix_;
      } else if(inputPrefix.compare(inputPrefix_) != 0) {
        WARN_LOG("not matching input var prefixes " << inputPrefix << " and " << inputPrefix_);
      }
    }
  }

  // erase uniforms with conflicting input vars
  for(set<GLSLUniform>::iterator it = uniforms.begin(); it != uniforms.end(); ++it)
  {
    const GLSLUniform &uni = *it;
    if( inputNames.count( uni.name )>0 ) {
      // replace uniform by input var
      uniforms.erase( it );
      it = uniforms.begin();
    }
  }

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

  // input vars have a prefix %s_$name uniformas are just calles $name
  // instancing may transforms the uniform to IO var with prefix.
  // make shaders able wo use input vars without prefix....
  for(set<string>::iterator it = inputNames.begin(); it != inputNames.end(); ++it)
  {
    replaceInputVar(*it, inputPrefix, &codeStr);
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

  // set layout
  switch(shaderType) {
  case GL_TESS_CONTROL_SHADER:
    code << "layout("; {
      code << "vertices = " << functions.tessNumVertices();
    } code << ") out;" << endl;
    break;
  case GL_TESS_EVALUATION_SHADER:
    code << "layout("; {
      switch(functions.tessPrimitive()) {
      case TESS_PRIMITVE_TRIANGLES:
        code << "triangles"; break;
      case TESS_PRIMITVE_QUADS:
        code << "quads"; break;
      case TESS_PRIMITVE_ISOLINES:
        code << "isolines"; break;
      }
      code << ", ";
      switch(functions.tessSpacing()) {
      case TESS_SPACING_EQUAL:
        code << "equal_spacing"; break;
      case TESS_SPACING_FRACTIONAL_EVEN:
        code << "fractional_even_spacing"; break;
      case TESS_SPACING_FRACTIONAL_ODD:
        code << "fractional_odd_spacing"; break;
      }
      code << ", ";
      switch(functions.tessOrdering()) {
      case TESS_ORDERING_CCW:
        code << "ccw"; break;
      case TESS_ORDERING_CW:
        code << "cw"; break;
      case TESS_ORDERING_POINT_MODE:
        code << "point_mode"; break;
      }
    } code << ") in;" << endl;
    break;
  case GL_GEOMETRY_SHADER:
    code << "layout("; {
      switch(functions.gsConfig().input) {
      case GS_INPUT_POINTS:
        code << "points"; break;
      case GS_INPUT_LINES:
        code << "lines"; break;
      case GS_INPUT_LINES_ADJACENCY:
        code << "lines_adjacency"; break;
      case GS_INPUT_TRIANGLES:
        code << "triangles"; break;
      case GS_INPUT_TRIANGLES_ADJACENCY:
        code << "triangles_adjacency"; break;
      }
      if(functions.gsConfig().invocations > 1) {
        code << ", ";
        code << "invocations = " << functions.gsConfig().invocations;
      }
    } code << ") in;" << endl;

    code << "layout("; {
      switch(functions.gsConfig().input) {
      case GS_OUTPUT_POINTS:
        code << "points"; break;
      case GS_OUTPUT_LINE_STRIP:
        code << "line_strip"; break;
      case GS_OUTPUT_TRIANGLE_STRIP:
        code << "triangle_strip"; break;
      }
      code << ", ";
      code << "max_vertices = " << functions.gsConfig().maxVertices;
    } code << ") out;" << endl;

    break;

  case GL_VERTEX_SHADER:
  case GL_FRAGMENT_SHADER:
  default:
    break;
  }
  code << endl;

  // add input/output vars for communication between shader stages

  switch(shaderType) {
  case GL_TESS_EVALUATION_SHADER:
    if(functions.inputs().size() > 0) {
      code << "in Tess" << endl;
      code << "{" << endl;
      for(set<GLSLTransfer>::iterator
          it = functions.inputs().begin();
          it != functions.inputs().end(); ++it)
      {
        if(it->interpolation.size() > 0) {
          code << "    " << it->interpolation << " " << *it << ";" << endl;
        } else {
          code << "    " << *it << ";" << endl;
        }
      }
      code << "} In[];" << endl;
    }
    break;
  case GL_TESS_CONTROL_SHADER:
    // TODO SHADERGEN: i guess [] will make problems for array attributes!
    for(set<GLSLTransfer>::iterator
        it = functions.inputs().begin();
        it != functions.inputs().end(); ++it)
    {
      if(it->interpolation.size() > 0) {
        code << it->interpolation << " in " << *it << "[];" << endl;
      } else {
        code << "in " << *it << "[];" << endl;
      }
    }
    break;
  default:
    for(set<GLSLTransfer>::iterator
        it = functions.inputs().begin();
        it != functions.inputs().end(); ++it)
    {
      if(it->interpolation.size() > 0) {
        code << it->interpolation << " in " << *it << ";" << endl;
      } else {
        code << "in " << *it << ";" << endl;
      }
    }
    break;
  }
  switch(shaderType) {
  case GL_TESS_CONTROL_SHADER:
    if(functions.outputs().size() > 0) {
      code << "out Tess" << endl;
      code << "{" << endl;
      for(set<GLSLTransfer>::iterator
          it = functions.outputs().begin();
          it != functions.outputs().end(); ++it)
      {
        if(it->interpolation.size() > 0) {
          code << "    " << it->interpolation << " " << *it << ";" << endl;
        } else {
          code << "    " << *it << ";" << endl;
        }
      }
      code << "} Out[];" << endl;
    }
    break;
  case GL_FRAGMENT_SHADER:
    for(list<GLSLFragmentOutput>::const_iterator
        it = functions.fragmentOutputs().begin(); it != functions.fragmentOutputs().end(); ++it)
    {
      code << "out " << it->type << " " << it->name << ";" << endl;
    }
    break;
  default:
    for(set<GLSLTransfer>::iterator
        it = functions.outputs().begin();
        it != functions.outputs().end(); ++it)
    {
      if(it->interpolation.size() > 0) {
        code << it->interpolation << " out " << *it << ";" << endl;
      } else {
        code << "out " << *it << ";" << endl;
      }
    }
    break;
  }
  code << endl;

  for(set<GLSLUniform>::iterator
      it = functions.uniforms().begin();
      it != functions.uniforms().end(); ++it)
  {
    if(codeStr.find(it->name) != codeStr.npos)
      code << "uniform " << *it << ";" << endl;
  }
  for(set<GLSLConstant>::iterator
      it = functions.constants().begin();
      it != functions.constants().end(); ++it)
  {
    if(codeStr.find(it->name) != codeStr.npos)
      code << "const " << *it << ";" << endl;
  }

  // add dependency functions
  code << codeStr;

  return code.str();
}
