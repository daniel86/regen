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

#include "fluid-operation.h"
#include "fluid-buffer.h"

class Fluid
{
public:
  Fluid(const string &name,
      const Vec3i &size,
      GLboolean isLiquid);
  ~Fluid();

  const Vec3i& size();
  const string& name();
  GLboolean isLiquid();

  GLfloat liquidHeight();
  void setLiquidHeight(GLfloat height);

  //////////

  void addBuffer(FluidBuffer *buffer);
  FluidBuffer* getBuffer(const string &name);

  //////////

  void addOperation(FluidOperation *operation, GLboolean isInitial=GL_FALSE);

  const list<FluidOperation*>& initialOperations();
  const list<FluidOperation*>& operations();

  void executeOperations(list<FluidOperation*>&);

protected:
  const string &name_;
  const Vec3i &size_;
  GLboolean isLiquid_;
  GLfloat liquidHeight_;
  list<FluidOperation*> operations_;
  list<FluidOperation*> initialOperations_;
  map<string,FluidBuffer*> buffers_;

private:
  Fluid(const Fluid&);
};

#endif /* FLUID_H_ */
