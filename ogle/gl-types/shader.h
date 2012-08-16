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
  Shader();
  ~Shader();

  /**
   * Returns the GL program object.
   */
  GLint id() const;

  /**
   * Attach shader stages to the program.
   * Note that you have to call link before
   * the program can be used.
   */
  bool compile(const map<GLenum, string> &stages);
  /**
   * Link added stages.
   */
  bool link();

  /**
   * Looks up attribute and uniform locations
   * in the linked program.
   * Shader::apply* functions are using the locations
   * obtained in this call.
   */
  void setupLocations(
      const set<string> &attributeNames,
      const set<string> &uniformNames);

  void setupInputs(map<string, ref_ptr<ShaderInput> > &inputs);

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
  void setupTransformFeedback(const list<string> &tfAtts);

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

protected:
  GLuint id_;

  map<string, GLint> uniformLocations_;
  map<string, GLint> attributeLocations_;

  list<ShaderInputLocation> attributes_;
  list<ShaderInputLocation> uniforms_;

  GLuint numInstances_;

  void printLog(
      GLuint shader,
      GLenum shaderType,
      const char *shaderCode,
      GLboolean success);
};

#endif /* _SHADER_H_ */
