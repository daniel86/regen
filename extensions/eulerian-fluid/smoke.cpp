
#include "include/smoke.h"

#define IS_LIQUID false

const string EulerianSmoke::DENSITY = "densityTex";
const string EulerianSmoke::TEMPERATURE = "temperatureTex";

EulerianSmoke::EulerianSmoke(
    ref_ptr<EulerianPrimitive> primitive,
    ref_ptr<EulerianObstacles> obstacles)
: EulerianFluid(primitive, obstacles)
{
  // temperature lifetime [0,1]
  const float temperatureDecay_ = 0.997f;
  // lifetimeFactor. factor for density in advection [0,1]
  const float densityDecay_ = 0.999993f;
  // temperature threshold for smoke raising
  const float ambientTemperature_ = 0.0f;

  // velocity lifetime [0,1]
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
  const float vorticityConfinement = 0.0f;

  useVorticity_ = (vorticityConfinement>0.0f);
  useGravity_ = false;

  // density is a single scalar, we need two textures for advection
  densityBuffer_ = createSlabWithName(EulerianSmoke::DENSITY, 1, 2);
  // same as for density
  temperatureBuffer_ = createSlabWithName(EulerianSmoke::TEMPERATURE, 1, 2);

  // set pressure stage parameters
  pressureStage_->set_inverseBeta(primitive_->is2D() ? (1.0f/4.0f) : (1.0f/6.0f));
  pressureStage_->set_halfInverseCellSize(0.5f/cellSize_);
  pressureStage_->set_fluidDensity(fluidDensity);
  pressureStage_->set_alpha(alpha);
  pressureStage_->set_numPressureIterations(numPressureIterations);

  velocityAdvectionTarget_->decayAmount = velocityDecay_;
  velocityAdvectionTarget_->quantityLoss = 0.0f;

  // set the vorticity confinement factor
  vorticityStage_->set_vorticityConfinementScale(vorticityConfinement);

  { // add temperature and density as advection target
    temperatureAdvectionTarget_ = ref_ptr<AdvectionTarget>::manage(new AdvectionTarget);
    temperatureAdvectionTarget_->buffer = temperatureBuffer_;
    temperatureAdvectionTarget_->decayAmount = temperatureDecay_;
    temperatureAdvectionTarget_->quantityLoss = 0.0f;
    temperatureAdvectionTarget_->treatAsLiquid = false;
    advectStage_->addAdvectionTarget(temperatureAdvectionTarget_);
    densityAdvectionTarget_ = ref_ptr<AdvectionTarget>::manage(new AdvectionTarget);
    densityAdvectionTarget_->buffer = densityBuffer_;
    densityAdvectionTarget_->decayAmount = densityDecay_;
    densityAdvectionTarget_->decayAmountUnnormalized = densityDecay_;
    densityAdvectionTarget_->quantityLoss = 0.0f;
    densityAdvectionTarget_->treatAsLiquid = false;
    advectStage_->addAdvectionTarget(densityAdvectionTarget_);
  }

  { // make smoke raising at high temperature by changing the velocity field
    buoyancyStage_ = ref_ptr<EulerianBuoyancy>::manage(
        new EulerianBuoyancy(primitive_.get()));
    buoyancyStage_->set_velocityBuffer(velocityBuffer_);
    buoyancyStage_->set_temperatureTexture(temperatureBuffer_.tex);
    buoyancyStage_->set_ambientTemperature(ambientTemperature_);
    buoyancyStage_->set_smokeBuoyancy(smokeBuoyancy);
  }

  // set default stages...
  // note: this is a call to EulerianSmoke::setDefaultStages
  EulerianSmoke::setDefaultStages();
}

void EulerianSmoke::reset()
{
  EulerianFluid::reset();
  clearSlab(densityBuffer_);
  clearSlab(temperatureBuffer_);
}

void EulerianSmoke::processStages()
{
  EulerianFluid::processStages();
}

void EulerianSmoke::setDefaultStages()
{
  stages_.clear();
  addStage(advectStage_);
  if(useVorticity_) addStage(vorticityStage_);
  addStage(splatStage_);
  addStage(buoyancyStage_);
  addStage(pressureStage_);
  if(useGravity_) {
    ref_ptr<EulerianGravity> gravityStage_ = ref_ptr<EulerianGravity>::manage(
        new EulerianGravity(primitive_.get()));
    gravityStage_->set_obstaclesTexture(obstacles_->tex());
    gravityStage_->set_velocityBuffer(velocityBuffer_);
    gravityStage_->addGravitySource(gravityVec_);
    addStage(gravityStage_);
  }
}
