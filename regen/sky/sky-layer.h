/*
 * sky-layer.h
 *
 *  Created on: Jan 4, 2014
 *      Author: daniel
 */

#ifndef SKY_LAYER_H_
#define SKY_LAYER_H_

#include <regen/meshes/mesh-state.h>
#include <regen/states/state.h>
#include <regen/states/state-node.h>
#include <regen/states/shader-state.h>

namespace regen {
  class SkyLayer : public StateNode {
  public:
    virtual ~SkyLayer() {}

    virtual void updateSky(RenderState *rs, GLdouble dt) = 0;
    virtual ref_ptr<Mesh> getMeshState() = 0;
    virtual ref_ptr<HasShader> getShaderState() = 0;
  };
}

#endif /* SKY_LAYER_H_ */
