/*
 * shader-fragment-output.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef SHADER_FRAGMENT_OUTPUT_H_
#define SHADER_FRAGMENT_OUTPUT_H_

#include <ogle/shader/shader-function.h>
#include <ogle/shader/glsl-types.h>

/**
 * Defines output for the fragment shader.
 * Output variables are associated to FBO color attachments.
 */
class ShaderFragmentOutput {
public:
  /**
   * Default constructor.
   */
  ShaderFragmentOutput()
  : colorAttachment_(GL_COLOR_ATTACHMENT0)
  {}
  /**
   * Add the output to a shader function for generating a shader
   * that uses this output.
   */
  virtual void addOutput(ShaderFunctions &fragmentShader) = 0;

  void set_colorAttachment(GLenum colorAttachment) {
    colorAttachment_ = colorAttachment;
  }
  GLenum colorAttachment() const {
    return colorAttachment_;
  }

  virtual string variableName() const = 0;

protected:
  GLenum colorAttachment_;
};

/**
 * Outputs the fragment color to a color attachment.
 */
class DefaultFragmentOutput : public ShaderFragmentOutput {
public:
  DefaultFragmentOutput()
  : ShaderFragmentOutput()
  {
  }

  virtual string variableName() const
  {
    return "defaultColorOutput";
  }
  // override
  virtual void addOutput(ShaderFunctions &fragmentShader)
  {
    fragmentShader.addFragmentOutput( GLSLFragmentOutput(
      "vec4", variableName(), colorAttachment_ ) );
    fragmentShader.addExport( GLSLExport( variableName(), "_color" ));
  }
};

#endif /* SHADER_FRAGMENT_OUTPUT_H_ */
