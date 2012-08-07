
#include "include/fire.h"

#define IS_LIQUID false

const string EulerianFire::REACTION_COORDINATE = "reactionCoordinateTex";

EulerianFire::EulerianFire(
    ref_ptr<EulerianPrimitive> primitive,
    ref_ptr<EulerianObstacles> obstacles)
: EulerianSmoke(primitive, obstacles)
{
  // temperature lifetime [0,1]
  const float temperatureDecay_ = 0.995f;
  // lifetimeFactor. factor for density in advection [0,1]
  const float densityDecay_ = 0.99f;
  const float densityLoss_ = 0.003f;
  // temperature threshold for smoke raising
  const float ambientTemperature_ = 0.0f;

  // velocity lifetime [0,1]
  //const float velocityDecay_ = 0.93f;
  const float velocityDecay_ = 0.999f;
  // try to converge to a good pressure value, higher numbers should be always better
  const int numPressureIterations = 20;

  // could be used to get same temperature ranges in multiple simulations
  // with temperature decay and temperature value in splat stage
  // all properties can be configured i think
  const float smokeBuoyancy = 1.0f * 50.0;

  const float cellSize_ = 1.25;
  const float fluidDensity = cellSize_ / 2.25f;
  const float alpha = cellSize_*cellSize_;

  const float vorticityConfinement = 0.75f;

  useVorticity_ = (vorticityConfinement > 0.0f);
  useGravity_ = false;

  // set pressure stage parameters
  pressureStage_->set_alpha(alpha);
  pressureStage_->set_fluidDensity(fluidDensity);
  pressureStage_->set_numPressureIterations(numPressureIterations);
  pressureStage_->set_inverseBeta(primitive_->is2D() ? (1.0f/4.0f) : (1.0f/6.0f));
  pressureStage_->set_halfInverseCellSize(0.5f/cellSize_);

  // setup advection targets
  temperatureAdvectionTarget_->decayAmount = temperatureDecay_;
  densityAdvectionTarget_->decayAmount = densityDecay_;
  densityAdvectionTarget_->quantityLoss = densityLoss_;
  velocityAdvectionTarget_->decayAmount = velocityDecay_;

  // swirl factor
  vorticityStage_->set_vorticityConfinementScale(vorticityConfinement);

  // setup temperature influence stage
  buoyancyStage_->set_smokeBuoyancy(smokeBuoyancy);
  buoyancyStage_->set_ambientTemperature(ambientTemperature_);

  // set default stages...
  // note: this is a call to EulerianFire::setDefaultStages
  EulerianFire::setDefaultStages();
}

void EulerianFire::reset()
{
  EulerianSmoke::reset();
}

void EulerianFire::setDefaultStages()
{
  stages_.clear();
  addStage(advectStage_);
  if(useVorticity_) addStage(vorticityStage_);
  addStage(splatStage_);
  addStage(pressureStage_);
  addStage(buoyancyStage_);
  if(useGravity_) {
    ref_ptr<EulerianGravity> gravityStage_ = ref_ptr<EulerianGravity>::manage(
        new EulerianGravity(primitive_.get()));
    gravityStage_->set_obstaclesTexture(obstacles_->tex());
    gravityStage_->set_velocityBuffer(velocityBuffer_);
    gravityStage_->addGravitySource(gravityVec_);
    addStage(gravityStage_);
  }
}
