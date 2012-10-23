/*
 * shader-manager.h
 *
 *  Created on: 01.02.2011
 *      Author: daniel
 */

#ifndef _SHADER_MANAGER_H_
#define _SHADER_MANAGER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <list>
using namespace std;

#include <ogle/shader/shader-function.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/gl-types/tbo.h>
#include <ogle/gl-types/shader.h>

/**
 * Helps compiling ShaderFunctions.
 * Singleton class.
 */
class ShaderManager
{
public:
  static string inputType(ShaderInput *input);
  static string inputValue(ShaderInput *input);

  static string loadShaderFromKey(const string &effectKey);
  static string loadShaderCode(const string &code);

  static ref_ptr<Shader> createShaderWithSignarure(
      const string &signature,
      const string &shaderHeader,
      const map<GLenum,string> &shaderNames);
  static string getShaderSignature(
      const map<GLenum,string> &shaderNames,
      const map<string,string> &shaderConfig);
  static string getShaderHeader(
      const map<string,string> &shaderConfig);
  static void setShaderWithSignarure(ref_ptr<Shader> &shader, const string &signature);

  static string generateSource(
      const ShaderFunctions &functions,
      GLenum shaderStage,
      GLenum nextShaderStage,
      const string &forcedPrefix="");

  /**
   * Display the shader log.
   */
  static void printLog(
      GLuint shader,
      GLenum shaderType,
      const char *shaderCode,
      GLboolean success);

  static GLboolean containsInputVar(
      const string &var,
      const string &code);
  static GLboolean containsInputVar(
      const string &var,
      const ShaderFunctions &f);

  static list<string> getValidTransformFeedbackNames(
      const map<GLenum, string> &shaderStages,
      const list< ref_ptr<VertexAttribute> > &tfAttributes);
  static list<string> getValidTransformFeedbackNames(
      const map<GLenum, string> &shaderStages,
      const list< string > &tfAttributes);

  static void addUniform(const GLSLUniform &v,
      map< GLenum, ShaderFunctions* > stages);
  static void addConstant(const GLSLConstant &v,
      map< GLenum, ShaderFunctions* > stages);

  static void setupInputs(
      map< string, ref_ptr<ShaderInput> > &inputs,
      map< GLenum, ShaderFunctions* > stages);

private:
  static map<string, ref_ptr<Shader> > shaderCache_;

  static GLboolean replaceVariable(
          const string &varName,
          const string &varPrefix,
          const string &desiredPrefix,
          string *code);
};

#endif /* _SHADER_MANAGER_H_ */
