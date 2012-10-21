/*
 * shader.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _SHADER_H_
#define _SHADER_H_

#include <map>
#include <set>
using namespace std;

#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/shader-input.h>

/**
 * Tuple of texture and unit the
 * texture is bound to.
 */
struct ShaderTexture
{
  const Texture *tex;
  GLuint texUnit;
  ShaderTexture(const Texture *_tex, GLuint _texUnit)
  : tex(_tex), texUnit(_texUnit)
  {
  }
};
/**
 * Tuple of FBO color attachment number
 * and output variable name used in the program.
 */
struct ShaderOutput
{
  GLenum colorAttachment;
  const string name;
  ShaderOutput(GLenum _colorAttachment, const string &_name)
  : colorAttachment(_colorAttachment),
    name(_name)
  {
  }
};
struct ShaderInputLocation
{
  ref_ptr<ShaderInput> input;
  GLint location;
  ShaderInputLocation(
      const ref_ptr<ShaderInput> &_input,
      GLint _location)
  : input(_input),
    location(_location)
  {
  }
};

/**
 * Encapsulates a GLSL program, helps
 * compiling and linking together the
 * shader stages.
 */
class Shader
{
public:
  Shader(const map<GLenum, string> &shaderCodes);
  ~Shader();

  GLboolean isPointShader() const;
  void set_isPointShader(GLboolean);

  GLboolean isLineShader() const;
  void set_isLineShader(GLboolean);

  bool hasShader(GLenum stage) const;

  /**
   * Returns the GL program object.
   */
  GLint id() const;

  /**
   * Attach shader stages to the program.
   * Note that you have to call link before
   * the program can be used.
   */
  bool compile();
  /**
   * Link added stages.
   */
  bool link();

  const GLuint& shader(GLenum stage) const;
  void setShaders(const map<GLenum, GLuint> &shaders);

  const map<string, ref_ptr<ShaderInput> >& inputs() const;
  GLboolean isUniform(const string &name) const;
  ref_ptr<ShaderInput> input(const string &name);
  void set_input(const string &name, ref_ptr<ShaderInput> &in);

  GLboolean isSampler(const string &name) const;
  GLint samplerLocation(const string &name);

  /**
   * Creates ShaderInput's for each active uniform.
   */
  void setupUniforms();

  /**
   * Bind user-defined varying out variables
   * to a fragment shader color number.
   * Note that this must be done only for MRT
   * and that it must be done before linking
   * but after compiling.
   */
  void setupOutputs(
      const list<ShaderOutput> &outputs);

  /**
   * Specify values to record in transform feedback buffers.
   * Note that this must be done before linking
   * but after compiling.
   * Also linking will fail if any of the provided variable
   * names could not be found in the program.
   */
  void setupTransformFeedback(
      const list<string> &tfAtts,
      GLenum attributeLayout);

  void applyInputs();
  /**
   * Bind texture unit with shader uniform.
   */
  void applyTexture(const ShaderTexture &d);
  /**
   * Bind attribute to shader.
   */
  void applyAttribute(const ShaderInput *in);
  /**
   * Bind uniform value to shader.
   */
  void applyUniform(const ShaderInput *in);

  GLuint numInstances() const;

  static void printLog(
      GLuint shader,
      GLenum shaderType,
      const char *shaderCode,
      GLboolean success);

  // TODO: DEPRECATED below

  void setupInputs(const map<string, ref_ptr<ShaderInput> > &inputs);

  /**
   * Looks up attribute and uniform locations
   * in the linked program.
   * Shader::apply* functions are using the locations
   * obtained in this call.
   */
  void setupLocations(
      const set<string> &attributeNames,
      const set<string> &uniformNames);

  const string& shaderCode(GLenum stage) const;

protected:
  GLuint id_;

  GLboolean isPointShader_;
  GLboolean isLineShader_;

  map<GLenum, GLuint> shaders_;

  map<string, GLint> samplerLocations_;
  map<string, GLint> uniformLocations_;
  map<string, GLint> attributeLocations_;

  list<ShaderInputLocation> attributes_;
  list<ShaderInputLocation> uniforms_;

  map<string, ref_ptr<ShaderInput> > inputs_;

  GLuint numInstances_;

  // TODO: DEPRECATED below
  map<GLenum, string> shaderCodes_;

private:
  Shader();
  Shader(const Shader&);
};

#endif /* _SHADER_H_ */
