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
  class Sky;

  class SkyLayer : public StateNode {
  public:
    SkyLayer(const ref_ptr<Sky> &sky) {
      sky_ = sky;
      updateInterval_ = 4000.0;
      dt_ = updateInterval_;
    }
    virtual ~SkyLayer() {}

    void updateSky(RenderState *rs, GLdouble dt) {
      dt_ += dt;
      if(dt_<updateInterval_) { return; }

      GL_ERROR_LOG();
      updateSkyLayer(rs,dt_);
      GL_ERROR_LOG();

      dt_ = 0.0;
    }

    void set_updateInterval(GLdouble interval_ms)
    { updateInterval_ = interval_ms; }

    GLdouble updateInterval() const
    { return updateInterval_; }

    virtual void updateSkyLayer(RenderState *rs, GLdouble dt) = 0;
    virtual ref_ptr<Mesh> getMeshState() = 0;
    virtual ref_ptr<HasShader> getShaderState() = 0;

  protected:
    ref_ptr<Sky> sky_;

    GLdouble updateInterval_;
    GLdouble dt_;
  };
}

#endif /* SKY_LAYER_H_ */
