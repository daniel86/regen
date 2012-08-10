
#ifndef EULERIAN_GRID_SMOKE_H_
#define EULERIAN_GRID_SMOKE_H_

#include "fluid.h"
#include "stages.h"

/**
 * From GPU Gems3:
 * Although the velocity field describes the fluid's motion, it does not look much like a
 * fluid when visualized directly. To get interesting visual effects, we must keep track of
 * additional quantities that are pushed around by the fluid. For instance, we can keep
 * track of density and temperature to obtain the appearance of smoke (Fedkiw et al.
 * 2001). For each additional quantity phi, we must allocate an additional texture with the
 * same dimensions as our grid. The evolution of values in this texture is governed by the
 * same advection equation used for velocity:
 *
 *      d_phi/d_t = -(u * NABLA) * phi
 *
 * In other words, we can use the same MacCormack advection routine we used to evolve
 * the velocity.
 * To achieve the particular effect we inject a three-dimensional Gaussian "splat" into a color texture
 * each frame to provide a source of "smoke." These color values have no real physical significance,
 * but they create attractive swirling patterns as they are advected throughout the volume by the fluid velocity.
 * To get a more physically plausible appearance, we must make sure that hot smoke rises
 * and cool smoke falls. To do so, we need to keep track of the fluid temperature T (which
 * again is advected by u). Unlike color, temperature values have an influence on the dynamics of the fluid.
 * This influence is described by the buoyant force:
 *
 *      f_buoyancy = ( P*m*g / R ) * [1/T0 - 1/T] * z
 *
 * where P is pressure, m is the molar mass of the gas, g is the acceleration due to gravity,
 * and R is the universal gas constant. In practice, all of these physical constants can be
 * treated as a single value and can be tweaked to achieve the desired visual appearance.
 * The value T0 is the ambient or "room" temperature, and T represents the temperature
 * values being advected through the flow. z is the normalized upward-direction vector.
 * The buoyant force should be thought of as an "external" force and should be added to
 * the velocity field immediately following velocity advection.
 */
class EulerianSmoke : public EulerianFluid
{
public:
  static const string DENSITY;
  static const string TEMPERATURE;

  EulerianSmoke(
      ref_ptr<EulerianPrimitive> primitive,
      ref_ptr<EulerianObstacles> obstacles);

  ////////////

  /**
   * Set the lifetime of the smoke density.
   * Range is 0.0 to 1.0, 0.0 means density disappears immediately
   * and 1.0 means it lives forever. Anyway due to numerical instability
   * even with 1.0 the density may disappear.
   * Usually you want to set a value near 1.0 here, something like 0.999 or so.
   */
  void setLifetimeDensity(float lifetime=0.99f, float loss=0.0f) {
    densityAdvectionTarget_->decayAmount = max(0.0f, min(1.0f, lifetime));
    velocityAdvectionTarget_->quantityLoss = loss;
  }

  /**
   * Set the lifetime of the smoke temperature.
   * Range is 0.0 to 1.0, 0.0 means temperature disappears immediately
   * and 1.0 means it lives forever. Anyway due to numerical instability
   * even with 1.0 the temperature may disappear.
   * Usually you want to set a value near 1.0 here, something like 0.999 or so.
   */
  void setLifetimeTemperature(float lifetime=0.999f, float loss=0.0f) {
    temperatureAdvectionTarget_->decayAmount = max(0.0f, min(1.0f, lifetime));
    velocityAdvectionTarget_->quantityLoss = loss;
  }

  /**
   * Set the environment temperature.
   * Smoke will raise at regions with a higher temperature.
   */
  void setAmbientTemperature(float temperature=0.0f) {
    buoyancyStage_->set_ambientTemperature(temperature);
  }

  virtual void reset();

  ////////////

  EulerianBuoyancy& buoyancyStage() {
    return *buoyancyStage_.get();
  }

  AdvectionTarget& densityAdvectionTarget() {
    return *densityAdvectionTarget_.get();
  }
  AdvectionTarget& temperatureAdvectionTarget() {
    return *temperatureAdvectionTarget_.get();
  }

  FluidBuffer densityBuffer() {
    return densityBuffer_;
  }
  FluidBuffer temperatureBuffer() {
    return temperatureBuffer_;
  }

  virtual void processStages();

protected:
  FluidBuffer temperatureBuffer_;
  FluidBuffer densityBuffer_;

  ref_ptr<AdvectionTarget> temperatureAdvectionTarget_;
  ref_ptr<AdvectionTarget> densityAdvectionTarget_;

  ref_ptr<EulerianBuoyancy> buoyancyStage_;

  virtual void setDefaultStages();
};

#endif /* EULERIAN_GRID_SMOKE_H_ */
