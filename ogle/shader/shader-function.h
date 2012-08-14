/*
 * shader-func.h
 *
 *  Created on: 01.02.2011
 *      Author: daniel
 */

#ifndef _SHADER_FUNC_H_
#define _SHADER_FUNC_H_

#include <set>
#include <list>
#include <map>
#include <string>
#include <vector>
using namespace std;

#include <ogle/gl-types/uniform.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/tesselation-config.h>
#include <ogle/gl-types/geometry-shader-config.h>
#include <ogle/shader/glsl-types.h>

/**
 * Models GLSL programs.
 * Plain string representations are used.
 */
class ShaderFunctions
{
public:
  static const string depthTestLEQ;
  static const string getRotMat;
  static const string textureMS;
  static const string getCubeUV;
  static const string getTubeUV;
  static const string getFlatUV;
  static const string getSphereUV;
  static const string posWorldSpaceWithUniforms;
  static const string linearDepth;
  static const string worldPositionFromDepth;
  static string posWorldSpace(
      ShaderFunctions &shader,
      const string &posInput,
      bool hasInstanceMat,
      GLuint maxNumBoneWeights);
  static string norWorldSpace(
      ShaderFunctions &shader,
      const string &norInput,
      bool hasInstanceMat,
      GLuint maxNumBoneWeights);

  ShaderFunctions();
  ShaderFunctions(
      const string &name,
      const vector<string> &args);
  /**
   * Copy constructor.
   */
  ShaderFunctions(const ShaderFunctions &other);

  /**
   * Join another shader function into this function.
   */
  void join(const ShaderFunctions &u,
      const list<string> &followingFunctions);

  /**
   * Copy from other.
   */
  ShaderFunctions& operator=(const ShaderFunctions&);
  void operator+=(const ShaderFunctions&);

  /**
   * Sets the minimal GLSL version this shader function can handle.
   */
  void setMinVersion(int minVersion);
  int minVersion() const;

  void addInput(const GLSLTransfer &in);
  const set<GLSLTransfer>& inputs() const;

  void addOutput(const GLSLTransfer &out);
  const set<GLSLTransfer>& outputs() const;

  void set_tessNumVertices(unsigned int tessNumVertices);
  unsigned int tessNumVertices() const;

  void set_tessPrimitive(TessPrimitive tessPrimitive);
  TessPrimitive tessPrimitive() const;

  void set_tessSpacing(TessVertexSpacing tessSpacing);
  TessVertexSpacing tessSpacing() const;

  void set_tessOrdering(TessVertexOrdering tessOrdering);
  TessVertexOrdering tessOrdering() const;

  void set_gsConfig(GeometryShaderConfig tessOrdering);
  GeometryShaderConfig gsConfig() const;

  /**
   * Adds function this function depends on.
   */
  void addDependencyCode(const string &codeId, const string &code);
  vector< pair<string,string> > deps() const;

  /**
   * Adds a uniform.
   */
  void addUniform(const GLSLUniform &uniform);
  const set<GLSLUniform>& uniforms() const;

  /**
   * Adds a constant.
   */
  void addConstant(const GLSLConstant &constant);
  const set<GLSLConstant>& constants() const;

  /**
   * Enable the specified extension in this shader.
   */
  void enableExtension(const string &extensionName);
  const set<string>& enabledExtensions() const;

  /**
   * Disable the specified extension in this shader.
   */
  void disableExtension(const string &extensionName);
  const set<string>& disabledExtensions() const;

  /**
   * Adds a variable to the main function.
   */
  void addMainVar(const GLSLVariable &var);
  const list<GLSLVariable>& mainVars() const;

  /**
   * Add a statement to the main function
   */
  void addStatement(GLSLEquation);
  void addStatement(GLSLStatement);

  /**
   * Adds an export to the main function.
   */
  void addExport(const GLSLExport &e);
  const list<GLSLExport>& exports() const;

  /**
   * Adds fragment output.
   */
  void addFragmentOutput(const GLSLFragmentOutput &output);
  const list<GLSLFragmentOutput>& fragmentOutputs() const;

  virtual string code() const;

  vector<string> generateFunctionCalls() const;

protected:
  vector< pair< string,vector<string> > > funcs_;
  map< string,string > funcCodes_;
  // user variables
  set<GLSLUniform> uniforms_;
  //
  set<GLSLConstant> constants_;
  set<GLSLTransfer> inputs_;
  set<GLSLTransfer> outputs_;
  // needed functions (tuple of name and code)
  map<string,string> deps_;
  list<GLSLVariable> mainVars_;
  list<GLSLExport> exports_;
  list<GLSLFragmentOutput> fragmentOutputs_;
  set<string> enabledExtensions_;
  set<string> disabledExtensions_;
  // minimum gl version
  GLint minVersion_;
  string myName_;

  TessPrimitive tessPrimitive_;
  TessVertexSpacing tessSpacing_;
  TessVertexOrdering tessOrdering_;
  unsigned int tessNumVertices_;

  GeometryShaderConfig gsConfig_;

};

#endif /* _SHADER_FUNC_H_ */
