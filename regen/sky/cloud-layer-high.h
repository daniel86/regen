/*
 * cloud-layer-high.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef HIGH_CLOUDS_H_
#define HIGH_CLOUDS_H_

#include <regen/sky/cloud-layer.h>

namespace regen {
  /**
   * \brief High Cloud Layer.
   * @see https://code.google.com/p/osghimmel/
   */
  class HighCloudLayer : public CloudLayer {
  public:
    HighCloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize=2048);

    const float defaultAltitude();

    const Vec2f defaultScale();

    GLdouble defaultChange();

    void set_color(const Vec3f &color);

    const ref_ptr<ShaderInput3f>& color() const;

  protected:
    ref_ptr<ShaderInput3f> color_;
  };
}
#endif /* HIGH_CLOUDS_H_ */
