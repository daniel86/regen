/*
 * debug-normal.h
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#ifndef DEBUG_NORMAL_H_
#define DEBUG_NORMAL_H_

#include <render-pass.h>
#include <scene.h>
#include <shader.h>

/**
 * A post rendering pass that processes all primitive sets of the scene
 * and adds normal vectors instead of the vertices.
 */
class DebugNormalPass : public RenderPass {
public:
  DebugNormalPass(Scene* scene, Mesh *mesh);
  Shader& shader();

  // override
  virtual void render();
  virtual bool rendersOnTop() { return true; }
  virtual bool usesDepthTest() { return true; }
protected:
  Mesh *mesh_;
  ref_ptr<Shader> normalPassShader_;
  ref_ptr<VertexAttribute> posAtt_;
  ref_ptr<VertexAttribute> norAtt_;
  GLuint posAttLoc_;
  GLuint norAttLoc_;
};


#endif /* DEBUG_NORMAL_H_ */
