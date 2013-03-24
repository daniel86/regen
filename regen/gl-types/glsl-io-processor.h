/*
 * glsl-io-processor.h
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#ifndef GLSL_IO_PROCESSOR_H_
#define GLSL_IO_PROCESSOR_H_

#include <boost/regex.hpp>
#include <regen/gl-types/shader-input.h>

#include <iostream>
#include <list>
#include <string>
using namespace std;

namespace ogle {
/**
 * \brief A GLSL processor that modifies the IO behavior of the Shader code.
 *
 * Specified ShaderInput can change the IO Type of declaration in the Shader.
 * This can be used to change declarations to constants, uniforms, attributes
 * or instanced attributes.
 * The declarations in the Shader supposed to use 'in_' and 'out_' prefix for
 * varyings in all stages, this processor ensures name matching by inserting
 * defines with a matching prefix above the declarations.
 * This processor also can automatically generate Shader code to transfer a
 * varying between stages. For each input of the following stage output
 * is generated if it was missing. Shaders must declare HANDLE_IO
 * somewhere above the main function and call 'HANDLE_IO(0)' in the main function
 * for this wo work.
 */
class GLSLInputOutputProcessor {
public:
  /**
   * \brief IO Varying used in Shader code.
   */
  struct InputOutput {
    string layout; /**< the layout qualifier. */
    string interpolation; /**< interpolation qulifier. */
    string ioType; /**< the IO type (in/out/const/uniform). */
    string dataType; /**< the data type as used in the Shader. */
    string name; /**< the name as used in the Shader. */
    string numElements; /**< number of array elements (name[N]). */
    string value; /**< for constants this defines the value. */

    InputOutput();
    /**
     * Copy constructor.
     */
    InputOutput(const InputOutput&);
    /**
     * @param stage a shader stage.
     * @return declaration of IO variable.
     */
    string declaration(GLenum stage);
  };

  /**
   * Truncate the one of the known prefixes from string
   * if string matches any prefix.
   */
  static string getNameWithoutPrefix(const string &name);

  /**
   * @param in The input stream providing GLSL code
   * @param stage The shader stage to pre process
   * @param nextStage The following Shader stage
   * @param nextStageInputs used to automatically genrate IO varyings
   * @param specifiedInput used to modify declarations
   */
  GLSLInputOutputProcessor(
      istream &in,
      GLenum stage,
      GLenum nextStage,
      const map<string,InputOutput> &nextStageInputs,
      const map<string, ref_ptr<ShaderInput> > &specifiedInput);

  /**
   * Outputs collected while processing the input stream.
   */
  map<string,InputOutput>& outputs();
  /**
   * Inputs collected while processing the input stream.
   */
  map<string,InputOutput>& inputs();

  /**
   * Read a single line from input stream.
   */
  bool getline(string &line);

  /**
   * Read input stream until EOF reached.
   */
  void preProcess(ostream &out);

protected:
  istream &in_;
  list<string> lineQueue_;
  const map<string,InputOutput> &nextStageInputs_;
  map<string,InputOutput> outputs_;
  map<string,InputOutput> inputs_;

  GLboolean wasEmpty_;
  GLenum stage_;
  GLenum nextStage_;
  const map<string, ref_ptr<ShaderInput> > &specifiedInput_;

  void defineHandleIO();
  void parseValue(string &v, string &val);
  void parseArray(string &v, string &numElements);
};

} // end ogle namespace

#endif /* GLSL_IO_PROCESSOR_H_ */
