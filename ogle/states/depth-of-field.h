/*
 * depth-of-field.h
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#ifndef DEPTH_OF_FIELD_H_
#define DEPTH_OF_FIELD_H_

#include <ogle/states/fullscreen-pass.h>
#include <ogle/states/texture-state.h>

namespace ogle {
/**
 * \brief Depth-of-Field implementation.
 */
class DepthOfField : public FullscreenPass
{
public:
  DepthOfField(
      const ref_ptr<Texture> &input,
      const ref_ptr<Texture> &blurInput,
      const ref_ptr<Texture> &depthTexture);

  const ref_ptr<ShaderInput1f>& focalDistance() const;
  const ref_ptr<ShaderInput1f>& focalWidth() const;
  const ref_ptr<ShaderInput1f>& blurRange() const;

protected:
  ref_ptr<ShaderInput1f> focalDistance_;
  ref_ptr<ShaderInput1f> focalWidth_;
  ref_ptr<ShaderInput1f> blurRange_;
};
} // namespace

#endif /* DEPTH_OF_FIELD_H_ */
