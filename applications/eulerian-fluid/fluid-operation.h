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

#include "fluid-buffer.h"

class FluidOperation : public State
{
public:
  enum Mode {
    MODIFY, NEXT, NEW
  };

  FluidOperation(
      const string &name,
      FluidBuffer *outputBuffer,
      GLboolean is2D,
      GLboolean useObstacles,
      GLboolean isLiquid);

  const string& name() const;

  Shader* shader();

  void set_mode(Mode mode);
  Mode mode() const;

  void set_blendMode(TextureBlendMode blendMode);
  TextureBlendMode blendMode() const;

  void set_numIterations(GLuint numIterations);
  GLuint numIterations() const;

  void set_clear(GLboolean clear);
  GLboolean clear() const;

  void addInputBuffer(FluidBuffer *buffer);

  void execute(RenderState *rs, GLint lastShaderID);

protected:
  string name_;
  ref_ptr<Shader> shader_;

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
