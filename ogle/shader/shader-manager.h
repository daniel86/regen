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

  /**
   * Compiles a set of ShaderFunctions
   */
  static bool compileShader(
      const map< GLenum, ShaderFunctions > &functions,
      GLint id,
      GLboolean linkShader=true);

  static string generateSource(
      const ShaderFunctions &functions,
      GLenum shaderStage,
      GLenum nextShaderStage);

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

  static void addUniform(const GLSLUniform &v,
      map< GLenum, ShaderFunctions* > stages);
  static void addConstant(const GLSLConstant &v,
      map< GLenum, ShaderFunctions* > stages);

  static void setupInputs(
      map< string, ref_ptr<ShaderInput> > &inputs,
      map< GLenum, ShaderFunctions* > stages);

  static void setupLocations(
      ref_ptr<Shader> &shader,
      map< GLenum, ShaderFunctions* > stages);

private:
  static GLboolean compileShader(
      const char *code,
      GLenum target,
      GLuint *id_ret);

  static GLboolean replaceVariable(
          const string &varName,
          const string &varPrefix,
          const string &desiredPrefix,
          string *code);
};

#endif /* _SHADER_MANAGER_H_ */
