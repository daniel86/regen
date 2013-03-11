/*
 * post-processing.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_POST_PROCESSING_H_
#define __SHADING_POST_PROCESSING_H_

#include <ogle/shading/ambient-occlusion.h>

namespace ogle {

/**
 * Encapsulates some frequently used post pocessing operations
 * done after but related to shading.
 */
class ShadingPostProcessing : public State, public Resizable
{
public:
  ShadingPostProcessing();
  void createShader(ShaderState::Config &cfg);
  virtual void resize();

  void setUseAmbientOcclusion();
  const ref_ptr<AmbientOcclusion>& ambientOcclusionState() const;

  void set_gBuffer(
      const ref_ptr<Texture> &depthTexture,
      const ref_ptr<Texture> &norWorldTexture,
      const ref_ptr<Texture> &diffuseTexture);
  void set_tBuffer(const ref_ptr<Texture> &colorTexture);
  void set_aoBuffer(const ref_ptr<Texture> &aoTexture);

protected:
  ref_ptr<TextureState> gDiffuseTexture_;
  ref_ptr<TextureState> gDepthTexture_;
  ref_ptr<TextureState> gNorWorldTexture_;
  ref_ptr<TextureState> tColorTexture_;
  ref_ptr<TextureState> aoTexture_;

  ref_ptr<StateSequence> stateSequence_;
  ref_ptr<ShaderState> shader_;
  ref_ptr<AmbientOcclusion> updateAOState_;
  GLboolean hasAO_;
};

} // end ogle namespace

#endif /* __SHADING_POST_PROCESSING_H_ */
