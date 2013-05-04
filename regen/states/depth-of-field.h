/*
 * depth-of-field.h
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#ifndef DEPTH_OF_FIELD_H_
#define DEPTH_OF_FIELD_H_

#include <regen/states/fullscreen-pass.h>
#include <regen/states/texture-state.h>

namespace regen {
  /**
   * \brief Depth-of-Field implementation.
   */
  class DepthOfField : public FullscreenPass
  {
  public:
    /**
     * @param input input texture.
     * @param blurInput blurred texture.
     * @param depthTexture input depth texture.
     */
    DepthOfField(
        const ref_ptr<Texture> &input,
        const ref_ptr<Texture> &blurInput,
        const ref_ptr<Texture> &depthTexture);

    /**
     * @return distance to point with max sharpness in NDC space.
     */
    const ref_ptr<ShaderInput1f>& focalDistance() const;
    /**
     * Inner and outer focal width. Between the original and
     * the blurred image are linear combined.
     * @return the focal width in NDC space.
     */
    const ref_ptr<ShaderInput2f>& focalWidth() const;

  protected:
    ref_ptr<ShaderInput1f> focalDistance_;
    ref_ptr<ShaderInput2f> focalWidth_;
  };
} // namespace

#endif /* DEPTH_OF_FIELD_H_ */
