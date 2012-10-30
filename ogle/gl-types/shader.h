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
 * Tuple of FBO color attachment number
 * and output variable name used in the program.
 */
struct ShaderOutput
{
  GLenum colorAttachment;
  string name;
  ShaderOutput(GLenum _colorAttachment, const string &_name)
  : colorAttachment(_colorAttachment), name(_name) {}
  ShaderOutput(const ShaderOutput &other)
  : colorAttachment(other.colorAttachment), name(other.name) {}
};
struct ShaderInputLocation
{
  ref_ptr<ShaderInput> input;
  GLint location;
  ShaderInputLocation(const ref_ptr<ShaderInput> &_input, GLint _location)
  : input(_input), location(_location) {}
};
struct ShaderTextureLocation
{
  ref_ptr<Texture> tex;
  GLint location;
  ShaderTextureLocation(const ref_ptr<Texture> &_input, GLint _location)
  : tex(_input), location(_location) {}
};

/**
 * Encapsulates a GLSL program, helps
 * compiling and linking together the
 * shader stages.
 */
class Shader
{
public:
  /**
   * Create a new shader or return an identical shader that
   * was loaded before.
   */
  static ref_ptr<Shader> create(
      const map<string, string> &shaderConfig,
      const map<string, ref_ptr<ShaderInput> > &specifiedInput,
      map<GLenum, string> &code);
  static ref_ptr<Shader> create(
      const map<string, string> &shaderConfig,
      map<GLenum, string> &code);
  /**
   * Loads stages and prepends a header and a body to the code.
   * #include directives are resolved and preProcessCode() is called.
   */
  static void load(
      const string &shaderHeader,
      map<GLenum,string> &shaderCode,
      const map<string, ref_ptr<ShaderInput> > &specifiedInput);
  /**
   * Loads stage and prepends a header and a body to the code.
   * #include directives are resolved.
   */
  static string load(const string &shaderCode);

  static void printLog(
      GLuint shader,
      GLenum shaderType,
      const char *shaderCode,
      GLboolean success);

  /////////////

  /**
   * Share GL resource with other shader.
   * Each shader has an individual configuration only GL resources
   * are shared.
   */
  Shader(Shader&);
  /**
   * Construct pre-compiled shader.
   * link() must be called to use this shader.
   * Note: make sure IO names in stages match each other.
   */
  Shader(
      const map<GLenum, string> &shaderNames,
      const map<GLenum, GLuint> &shaderStages);
  /**
   * Create a new shader with given stage map.
   * compile() and link() must be called to use this shader.
   */
  Shader(
      const map<GLenum, string> &shaderNames);
  ~Shader();

  GLboolean compile();
  /**
   * Link together previous compiled stages.
   * Note: For MRT you must call setOutputs before and for
   * transform feedback you must call setTransformFeedback before.
   */
  GLboolean link();

  GLint id() const;

  GLuint numInstances() const;

  GLboolean isAttribute(const string &name) const;
  GLboolean isUniform(const string &name) const;
  GLboolean isSampler(const string &name) const;

  GLboolean hasUniformData(const string &name) const;

  GLint samplerLocation(const string &name);
  GLint attributeLocation(const string &name);

  GLuint stage(GLenum stage) const;
  const string& stageCode(GLenum stage) const;
  bool hasStage(GLenum stage) const;

  const map<string, ref_ptr<ShaderInput> >& inputs() const;
  ref_ptr<ShaderInput> input(const string &name);

  void setInput(const ref_ptr<ShaderInput> &in);
  void setTexture(const ref_ptr<Texture> &in);
  void setInputs(const map<string, ref_ptr<ShaderInput> > &inputs);

  /**
   * Must be done before linking for MRT.
   */
  void setOutput(const ShaderOutput &out);
  /**
   * Must be done before linking for MRT.
   */
  void setOutputs(const list<ShaderOutput> &outputs);

  /**
   * Must be done before linking for transform feedback.
   */
  void setTransformFeedback(
      const list<string> &transformFeedback,
      GLenum attributeLayout=GL_SEPARATE_ATTRIBS);

  /**
   * Upload inputs that was added by setInput() or setInputs().
   */
  void uploadInputs();
  /**
   * Upload given texture channel.
   */
  void uploadTexture(const Texture *tex);
  /**
   * Upload given attribute access information.
   */
  void uploadAttribute(const ShaderInput *in);
  /**
   * Upload given uniform value.
   */
  void uploadUniform(const ShaderInput *in);

protected:
  // the GL shader handle that can be shared by multiple Shader's
  ref_ptr<GLuint> id_;
  GLuint numInstances_;

  // shader codes without replaced input prefix
  map<GLenum, string> shaderCodes_;
  // compiled shader objects
  map<GLenum, GLuint> shaders_;

  // location maps
  map<string, GLint> samplerLocations_;
  map<string, GLint> uniformLocations_;
  map<string, GLint> attributeLocations_;

  // setup uniforms and attributes
  list<ShaderInputLocation> attributes_;
  list<ShaderInputLocation> uniforms_;
  list<ShaderTextureLocation> textures_;
  // available inputs
  map<string, ref_ptr<ShaderInput> > inputs_;
  // available outputs (only needed for MRT)
  list<ShaderOutput> outputs_;

  list<string> transformFeedback_;
  GLenum transformfeedbackLayout_;

  void setupInputLocations();
};

#endif /* _SHADER_H_ */
