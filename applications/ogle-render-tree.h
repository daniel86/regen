/*
 * ogle-render-tree.h
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#ifndef OGLE_RENDER_TREE_H_
#define OGLE_RENDER_TREE_H_

#include <ogle/render-tree/render-tree.h>

class OGLERenderTree : public RenderTree
{
public:
  virtual void render(GLdouble dt) = 0;
  virtual void postRender(GLdouble dt) = 0;
  virtual void setWindowSize(GLuint w, GLuint h) = 0;
  virtual void setMousePosition(GLuint x, GLuint y) = 0;
  virtual void initTree() = 0;
};

#endif /* OGLE_RENDER_TREE_H_ */
