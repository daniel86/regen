/*
 * fluid.h
 *
 *  Created on: 06.10.2012
 *      Author: daniel
 */

#ifndef FLUID_H_
#define FLUID_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <map>
using namespace std;

#include <ogle/algebra/vector.h>
#include <ogle/states/mesh-state.h>

#include "fluid-operation.h"
#include "fluid-buffer.h"

/**
 * The Fluid class contains global configuration, buffer objects and
 * a sequence of fluid operations for a single simulation.
 */
class Fluid
{
public:
  Fluid(const string &name,
      GLfloat timestep,
      GLboolean isLiquid,
      GLboolean is2D);
  ~Fluid();

  const string& name();

  /**
   * The constant timestep that is used by FluidOperation's.
   */
  GLfloat timestep() const;

  /**
   * Is this fluid a liquid?
   * This has influence on generated shaders.
   */
  GLboolean isLiquid();
  GLboolean is2D();

  /**
   * Equilibrium height for liquid fluids.
   */
  GLfloat liquidHeight();
  /**
   * Equilibrium height for liquid fluids.
   */
  void setLiquidHeight(GLfloat height);

  GLint framerate();
  void set_framerate(GLint framerate);

  /**
   * A quad used for updating textures.
   */
  MeshState *textureQuad();
  /**
   * A quad used for updating textures.
   */
  void set_textureQuad(MeshState*);

  //////////

  /**
   * Add a named buffer to the list of known buffers.
   */
  void addBuffer(FluidBuffer *buffer);
  /**
   * Get buffer by name.
   */
  FluidBuffer* getBuffer(const string &name);

  //////////

  /**
   * Add an operation to this fluid.
   */
  void addOperation(
      FluidOperation *operation,
      GLboolean isInitial=GL_FALSE);

  /**
   * Sequence of FluidOperation's that are supposed to be
   * executed once on initialization.
   */
  const list<FluidOperation*>& initialOperations();
  /**
   * Sequence of FluidOperation's that are supposed to be
   * executed each frame.
   */
  const list<FluidOperation*>& operations();

  /**
   * Execute sequence of operations.
   */
  void executeOperations(const list<FluidOperation*>&);

protected:
  const string &name_;
  MeshState *textureQuad_;

  GLfloat timestep_;
  GLint framerate_;

  GLboolean is2D_;
  GLboolean isLiquid_;
  GLfloat liquidHeight_;
  list<FluidOperation*> operations_;
  list<FluidOperation*> initialOperations_;
  map<string,FluidBuffer*> buffers_;

private:
  Fluid(const Fluid&);
};

#endif /* FLUID_H_ */
