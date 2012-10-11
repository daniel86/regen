/*
 * fluid-operation.h
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#ifndef FLUID_OPERATION_H_
#define FLUID_OPERATION_H_

#include <string>
using namespace std;

#include <ogle/gl-types/shader.h>
#include <ogle/states/state.h>
#include <ogle/states/mesh-state.h>

#include "fluid-buffer.h"

/**
 * FluidOperation's are using an associated shader program
 * to compute fluid simulations on the GPU.
 * Each operation has a set of associated shader inputs, an output buffer
 * and a set of general configurations that are used by each individual
 * operation.
 */
class FluidOperation : public State
{
public:
  /**
   * Defines how to handle the output buffer.
   */
  enum Mode {
    // modify the latest render result, usually combined with a blend mode
    MODIFY,
    // use latest render result to create a new result
    NEXT,
    // just render to the currently active target
    NEW
  };

  /**
   * Sets up operation and tries to load a shader
   * program from the default shader resource file.
   */
  FluidOperation(
      const string &name,
      FluidBuffer *outputBuffer,
      GLboolean is2D,
      GLboolean useObstacles,
      GLboolean isLiquid,
      GLfloat timestep,
      MeshState *textureQuad);

  const string& name() const;

  /**
   * The operation shader program.
   * NULL if no shader loaded yet.
   */
  Shader* shader();

  /**
   * Defines how to handle the output buffer.
   */
  void set_mode(Mode mode);
  /**
   * Defines how to handle the output buffer.
   */
  Mode mode() const;

  /**
   * Activates blending before execution.
   */
  void set_blendMode(TextureBlendMode blendMode);
  /**
   * Activates blending before execution.
   */
  TextureBlendMode blendMode() const;

  /**
   * Number of executions per frame.
   */
  void set_numIterations(GLuint numIterations);
  /**
   * Number of executions per frame.
   */
  GLuint numIterations() const;

  /**
   * Clear the buffer texture before execution.
   */
  void set_clear(GLboolean clear);
  /**
   * Clear the buffer texture before execution.
   */
  GLboolean clear() const;

  void addInputBuffer(FluidBuffer *buffer);

  void execute(RenderState *rs, GLint lastShaderID);

protected:
  string name_;

  MeshState *textureQuad_;
  ref_ptr<ShaderInput> posInput_;

  ref_ptr<Shader> shader_;
  GLuint posLoc_;

  Mode mode_;
  TextureBlendMode blendMode_;
  GLuint numIterations_;
  GLboolean clear_;

  FluidBuffer *outputBuffer_;
  Texture *outputTexture_;
  struct PositionedFluidBuffer {
    GLint loc;
    FluidBuffer *buffer;
  };
  list<PositionedFluidBuffer> inputBuffer_;

  ref_ptr<State> blendState_;
  ref_ptr<State> operationModeState_;
};

#endif /* FLUID_OPERATION_H_ */
