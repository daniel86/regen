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

/**
 * Helps compiling ShaderFunctions.
 * Singleton class.
 */
class ShaderManager
{
public:
  /**
   * Compiles a set of ShaderFunctions
   */
  static bool compileShader(
      const map< GLenum, ShaderFunctions > &functions,
      GLint id,
      bool linkShader=true);

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
      bool success);

  static bool containsInputVar(
      const string &var,
      const string &code);
  static bool containsInputVar(
      const string &var,
      const ShaderFunctions &f);

  static list<string> getValidTransformFeedbackNames(
      const map<GLenum, string> &shaderStages,
      const map<string,VertexAttribute*> &tfAttributes);

private:
  static bool compileShader(
      const char *code,
      GLenum target,
      GLuint *id_ret);

  static void replaceVariable(
          const string &varName,
          const string &varPrefix,
          const string &desiredPrefix,
          string *code);
};

#endif /* _SHADER_MANAGER_H_ */
