/*
 * shading-interface.h
 *
 *  Created on: 09.11.2012
 *      Author: daniel
 */

#ifndef SHADING_INTERFACE_H_
#define SHADING_INTERFACE_H_

#include <ogle/render-tree/state-node.h>
#include <ogle/states/fbo-state.h>

class ShadingInterface : public StateNode {
public:
  ShadingInterface() : StateNode() {}

  virtual ref_ptr<StateNode>& geometryStage() = 0;
  virtual ref_ptr<FBOState>& framebuffer() = 0;
  virtual ref_ptr<Texture>& depthTexture() = 0;
  virtual ref_ptr<Texture>& colorTexture() = 0;
  virtual void resize(GLuint w, GLuint h) = 0;
};

#endif /* SHADING_INTERFACE_H_ */
