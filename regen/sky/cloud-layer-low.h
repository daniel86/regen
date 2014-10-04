/*
 * low-clouds.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef LOW_CLOUDS_H_
#define LOW_CLOUDS_H_

#include <regen/sky/cloud-layer.h>
#include <regen/gl-types/fbo.h>

namespace regen {
  /**
   * \brief High Cloud Layer.
   * @see https://code.google.com/p/osghimmel/
   */
  class LowCloudLayer : public CloudLayer {
  public:
    LowCloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize);

    const float defaultAltitude();

    const Vec2f defaultScale();

    GLdouble defaultChange();

    void set_bottomColor(const Vec3f &color);

    const ref_ptr<ShaderInput3f>& bottomColor() const;

    void set_topColor(const Vec3f &color);

    const ref_ptr<ShaderInput3f>& topColor() const;

    void set_thickness(GLdouble thickness);

    const ref_ptr<ShaderInput1f>& thickness() const;

    void set_offset(GLdouble offset);

    const ref_ptr<ShaderInput1f>& offset() const;

    // Override
    void updateSkyLayer(RenderState *rs, GLdouble dt);

  protected:
    ref_ptr<ShaderInput1f> q_;
    ref_ptr<ShaderInput1f> offset_;
    ref_ptr<ShaderInput1f> thickness_;
    ref_ptr<ShaderInput3f> topColor_;
    ref_ptr<ShaderInput3f> bottomColor_;
  };
}
#endif /* LOW_CLOUDS_H_ */
