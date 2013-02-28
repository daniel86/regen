/*
 * tesselation-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef TESSELATION_STATE_H_
#define TESSELATION_STATE_H_

#include <ogle/states/state.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <ogle/algebra/vector.h>

class SetPatchVertices : public State
{
public:
  SetPatchVertices(GLuint numPatchVertices);
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
protected:
  GLuint numPatchVertices_;
};

class SetTessLevel : public State
{
public:
  SetTessLevel(
      const ref_ptr<ShaderInput4f> &outerLevel,
      const ref_ptr<ShaderInput4f> &innerLevel);
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
protected:
  ref_ptr<ShaderInput4f> innerLevel_;
  ref_ptr<ShaderInput4f> outerLevel_;
};

/**
 * Provides tesselation configuration and LoD uniform.
 */
class TesselationState : public State
{
public:
  typedef enum {
    // do not use a custom tess control
    FIXED_FUNCTION,
    // LoD as function of screen space edge size
    EDGE_SCREEN_DISTANCE,
    // LoD as function of device space edge size
    EDGE_DEVICE_DISTANCE,
    // LoD as function of distance to the camera
    CAMERA_DISTANCE_INVERSE
  }LoDMetric;

  TesselationState(GLuint numPatchVertices);
  virtual ~TesselationState() {};

  void set_lodMetric(LoDMetric metric);
  LoDMetric lodMetric() const;

  /**
   * Tesselation has a range for its levels, maxLevel is currently 64.0.
   * If you set the factor to 0.5 the range will be clamped to [1,maxLevel*0.5]
   * If you set the factor to 32.0 the range will be clamped to [32,maxLevel].
   */
  const ref_ptr<ShaderInput1f>& lodFactor() const;
  /**
   * only used if !isAdaptive
   */
  const ref_ptr<ShaderInput4f>& outerLevel() const;
  /**
   * only used if !isAdaptive
   */
  const ref_ptr<ShaderInput4f>& innerLevel() const;

protected:
  LoDMetric lodMetric_;
  GLuint numPatchVertices_;

  ref_ptr<ShaderInput1f> lodFactor_;
  ref_ptr<ShaderInput4f> outerLevel_;
  ref_ptr<ShaderInput4f> innerLevel_;

  ref_ptr<State> tessLevelSetter_;

  GLboolean usedTess_;
};

#endif /* TESSELATION_STATE_H_ */
