/*
 * tesselation-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef TESSELATION_STATE_H_
#define TESSELATION_STATE_H_

#include <regen/states/state.h>

#include <GL/glew.h>
#include <regen/math/vector.h>

namespace regen {
  /**
   * \brief Encapsulates Tesselation states.
   */
  class TesselationState : public State
  {
  public:
    /**
     * \brief Defines the metric to compute LoD factor for faces.
     */
    enum LoDMetric {
      /** do not use a custom tess control */
      FIXED_FUNCTION,
      /** LoD as function of screen space edge size */
      EDGE_SCREEN_DISTANCE,
      /** LoD as function of device space edge size */
      EDGE_DEVICE_DISTANCE,
      /** LoD as function of distance to the camera */
      CAMERA_DISTANCE_INVERSE
    };

    /**
     * @param numPatchVertices Specifies the number of vertices that
     * will be used to make up a single patch primitive.
     */
    TesselationState(GLuint numPatchVertices);

    /**
     * @param metric the metric to compute LoD factor for faces.
     */
    void set_lodMetric(LoDMetric metric);
    /**
     * @return the metric to compute LoD factor for faces.
     */
    LoDMetric lodMetric() const;

    /**
     * Tesselation has a range for its levels, maxLevel is currently 64.0.
     * If you set the factor to 0.5 the range will be clamped to [1,maxLevel*0.5]
     * If you set the factor to 32.0 the range will be clamped to [32,maxLevel].
     * @return the LoD factor.
     */
    const ref_ptr<ShaderInput1f>& lodFactor() const;
    /**
     * @note only used if !isAdaptive
     */
    const ref_ptr<ShaderInput4f>& outerLevel() const;
    /**
     * @note only used if !isAdaptive
     */
    const ref_ptr<ShaderInput4f>& innerLevel() const;

  protected:
    LoDMetric lodMetric_;
    GLuint numPatchVertices_;

    ref_ptr<ShaderInput1f> lodFactor_;
    ref_ptr<ShaderInput4f> outerLevel_;
    ref_ptr<ShaderInput4f> innerLevel_;

    ref_ptr<State> tessLevelSetter_;
  };
} // namespace

#endif /* TESSELATION_STATE_H_ */
