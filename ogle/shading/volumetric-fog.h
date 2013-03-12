/*
 * volumetric-fog.h
 *
 *  Created on: 07.02.2013
 *      Author: daniel
 */

#ifndef VOLUMETRIC_FOG_H_
#define VOLUMETRIC_FOG_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/texture-state.h>

namespace ogle {

class VolumetricSpotFog;
class VolumetricPointFog;

class VolumetricFog : public State
{
public:
  VolumetricFog();

  void createShader(ShaderConfig &cfg);

  void set_gDepthTexture(const ref_ptr<Texture> &t);
  void set_tBuffer(const ref_ptr<Texture> &color, const ref_ptr<Texture> &depth);

  void addLight(
      const ref_ptr<SpotLight> &l,
      const ref_ptr<ShaderInput1f> &exposure,
      const ref_ptr<ShaderInput2f> &radiusScale,
      const ref_ptr<ShaderInput2f> &coneScale);
  void addLight(
      const ref_ptr<PointLight> &l,
      const ref_ptr<ShaderInput1f> &exposure,
      const ref_ptr<ShaderInput2f> &radiusScale);

  void removeLight(SpotLight *l);
  void removeLight(PointLight *l);

  const ref_ptr<ShaderInput1f>& fogStart() const;
  const ref_ptr<ShaderInput1f>& fogEnd() const;

protected:
  ref_ptr<TextureState> tDepthTexture_;
  ref_ptr<TextureState> tColorTexture_;
  ref_ptr<TextureState> gDepthTexture_;

  ref_ptr<StateSequence> fogSequence_;
  ref_ptr<VolumetricSpotFog> spotFog_;
  ref_ptr<VolumetricPointFog> pointFog_;

  ref_ptr<ShaderInput1f> fogStart_;
  ref_ptr<ShaderInput1f> fogEnd_;
};

class VolumetricSpotFog : public State
{
public:
  VolumetricSpotFog();

  void createShader(ShaderConfig &cfg);

  void addLight(
      const ref_ptr<SpotLight> &l,
      const ref_ptr<ShaderInput1f> &exposure,
      const ref_ptr<ShaderInput2f> &radiusScale,
      const ref_ptr<ShaderInput2f> &coneScale);
  void removeLight(Light *l);

  virtual void enable(RenderState *rs);

protected:
  friend class VolumetricFog;

  struct FogLight {
    ref_ptr<SpotLight> l;
    ref_ptr<ShaderInput1f> exposure;
    ref_ptr<ShaderInput2f> radiusScale;
    ref_ptr<ShaderInput2f> coneScale;
  };
  list<FogLight> lights_;
  map< Light*, list<FogLight>::iterator > lightIterators_;

  ref_ptr<MeshState> mesh_;

  ref_ptr<ShaderState> fogShader_;
  GLint posLoc_;
  GLint dirLoc_;
  GLint coneLoc_;
  GLint radiusLoc_;
  GLint diffuseLoc_;
  GLint exposureLoc_;
  GLint radiusScaleLoc_;
  GLint coneScaleLoc_;
  GLint coneMatLoc_;
};

class VolumetricPointFog : public State
{
public:
  VolumetricPointFog();

  void createShader(ShaderConfig &cfg);

  void addLight(
      const ref_ptr<PointLight> &l,
      const ref_ptr<ShaderInput1f> &exposure,
      const ref_ptr<ShaderInput2f> &radiusScale);
  void removeLight(Light *l);

  virtual void enable(RenderState *rs);

protected:
  friend class VolumetricFog;

  struct FogLight {
    ref_ptr<PointLight> l;
    ref_ptr<ShaderInput1f> exposure;
    ref_ptr<ShaderInput2f> radiusScale;
  };
  list<FogLight> lights_;
  map< Light*, list<FogLight>::iterator > lightIterators_;

  ref_ptr<MeshState> mesh_;

  ref_ptr<ShaderState> fogShader_;
  GLint posLoc_;
  GLint radiusLoc_;
  GLint diffuseLoc_;
  GLint exposureLoc_;
  GLint radiusScaleLoc_;
};

} // end ogle namespace

#endif /* VOLUMETRIC_FOG_H_ */
