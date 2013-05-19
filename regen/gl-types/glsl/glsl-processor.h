/*
 * glsl-preprocessor.h
 *
 *  Created on: 12.05.2013
 *      Author: daniel
 */

#ifndef GLSL_PROCESSOR_H_
#define GLSL_PROCESSOR_H_

#include <map>
#include <sstream>
using namespace std;

#include <regen/gl-types/shader-input.h>

namespace regen {
  /**
   * \brief Specifies input for GLSL pre-processors.
   */
  struct PreProcessorInput {
    /**
     * Default-Constructor.
     */
    PreProcessorInput(
        const string &_header,
        const map<GLenum, string> &_unprocessed,
        const map<string, string> &_externFunctions,
        const map<string, ref_ptr<ShaderInput> > &_specifiedInput)
    : header(_header),
      unprocessed(_unprocessed),
      externFunctions(_externFunctions),
      specifiedInput(_specifiedInput)
    {}
    /**
     * Copy-Constructor.
     */
    PreProcessorInput(const PreProcessorInput &other)
    : header(other.header),
      unprocessed(other.unprocessed),
      externFunctions(other.externFunctions),
      specifiedInput(other.specifiedInput)
    {}
    /** Header is prepended to input stream. */
    const string &header;
    /** Unprocessed GLSL code. */
    const map<GLenum, string> &unprocessed;
    /** Extern function definitions. */
    const map<string, string> &externFunctions;
    /** Input data configuration. */
    const map<string, ref_ptr<ShaderInput> > &specifiedInput;
  };
}

namespace regen {
  /**
   * \brief The current state of pre-processing a GLSL code.
   */
  struct PreProcessorState {
    /**
     * Default constructor.
     * @param _in input for GLSL pre-processors.
     */
    PreProcessorState(const PreProcessorInput &_in)
    : in(_in), nextStage(GL_NONE), currStage(GL_NONE), version(150) {}
    /** Input for GLSL pre-processors. */
    PreProcessorInput in;
    /** Shader stage following the current stage in the shader pipeline. */
    GLenum nextStage;
    /** The currently pre-processed shader stage. */
    GLenum currStage;
    /** The minimum GLSL version */
    GLuint version;
    /** Input stream providing unprocessed GLSL code. */
    stringstream inStream;
  };
}

namespace regen {
  /**
   * \brief Baseclass for GLSL pre-processors.
   *
   * The stream-like interface of GLSLProcessor is used
   * to process a sequence of pre-processors line-by-line.
   */
  class GLSLProcessor
  {
  public:
    GLSLProcessor() {}
    virtual ~GLSLProcessor() {}

    /**
     * Sets the parent pre-processor. Most pre-processors take their input
     * from the parent processor.
     * @param parent the parent processor.
     */
    void setParent(const ref_ptr<GLSLProcessor> &parent)
    { parent_ = parent; }
    /**
     * @return the parent processor.
     */
    const ref_ptr<GLSLProcessor>& getParent()
    { return parent_; }

    /**
     * Produce a single line.
     * @param state the pre-processor state.
     * @param line the line return.
     * @return true on success
     */
    virtual bool getline(PreProcessorState &state, string &line)=0;
    /**
     * Clear is called to reset the processor to initial state.
     */
    virtual void clear() {}
  protected:
    ref_ptr<GLSLProcessor> parent_;

    bool getlineParent(PreProcessorState &state, string &line)
    {
      if(parent_.get()) {
        return parent_->getline(state, line);
      } else {
        return false;
      }
    }
  };
}

namespace regen {
  /**
   * \brief Simple GLSL pre-processor that
   * provides the unprocessed GLSL code to child processors.
   */
  class InputProviderProcessor : public GLSLProcessor
  {
  public:
    InputProviderProcessor() : GLSLProcessor() {}

    // override
    bool getline(PreProcessorState &state, string &line)
    { return std::getline(state.inStream, line); }
  };
}

namespace regen {
  /**
   * \brief Removes empty lines from GLSL code.
   */
  class WhiteSpaceProcessor : public GLSLProcessor
  {
  public:
    WhiteSpaceProcessor() : GLSLProcessor() {}

    // override
    bool getline(PreProcessorState &state, string &line)
    {
      if(!getlineParent(state, line)) return false;

      if(line.find_first_not_of(' ') != std::string::npos) {
        // There's a non-space.
        return true;
      }
      else {
        // empty string
        return getline(state,line);
      }
    }
  };
}

namespace regen {
  /**
   * \brief Simple GLSL pre-processor that
   * provides input from an arbitrary stringstream.
   */
  class StreamProcessor : public GLSLProcessor
  {
  public:
    StreamProcessor() : GLSLProcessor() {}
    /**
     * Constructor that fills the stringstream with initial data.
     * @param v the initial string data.
     */
    StreamProcessor(const string &v) : GLSLProcessor(), ss_(v) {}

    /**
     * @return the attached stringstream.
     */
    stringstream& stream()
    { return ss_; }

    // override
    bool getline(PreProcessorState &state, string &line)
    { return std::getline(ss_, line); }

  protected:
    stringstream ss_;
  };
}

#endif /* GLSL_PREPROCESSOR_H_ */
