/*
 * io-processor.h
 *
 *  Created on: 29.10.2012
 *      Author: daniel
 */

#ifndef __GLSL_IO_PROCESSOR_H_
#define __GLSL_IO_PROCESSOR_H_

#include <boost/regex.hpp>
#include <regen/gl-types/shader-input.h>
#include <regen/gl-types/glsl/glsl-processor.h>

#include <iostream>
#include <list>
#include <string>
using namespace std;

namespace regen {
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
  class IOProcessor : public GLSLProcessor {
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
     * Default constructor.
     */
    IOProcessor();

    // Override
    bool process(PreProcessorState &state, string &line);
    void clear();

  protected:
    list<string> lineQueue_;
    map<GLenum, map<string,InputOutput> > inputs_;
    map<GLenum, map<string,InputOutput> > outputs_;
    map<GLenum, map<string,InputOutput> > uniforms_;
    set<string> inputNames_;
    GLboolean isInputSpecified_;
    GLenum currStage_;

    void declareSpecifiedInput(PreProcessorState &state);
    void defineHandleIO(PreProcessorState &state);
    void parseValue(string &v, string &val);
    void parseArray(string &v, string &numElements);
  };
} // namespace

#endif /* GLSL_IO_PROCESSOR_H_ */
