
#include "include/liquid.h"
#include "include/helper.h"

#define IS_LIQUID true

class DistanceToLiquidHeightShader : public ShaderFunctions {
public:
  DistanceToLiquidHeightShader(EulerianFluid *grid)
  : ShaderFunctions("fluidDistanceToLiquidHeight", makeShaderArgs())
  {
    addUniform( GLSLUniform("float", "liquidHeight") );
  }
  virtual string code() const {
    stringstream s;
    s << "void " << myName_ << "(out float outCol) {" << endl;
    s << "     outCol = gl_FragCoord.y - liquidHeight;" << endl;
    s << "}" << endl;
    return s.str();
  }
};

const string EulerianLiquid::LEVEL_SET = "levelSetTex";

// TODO FLUID: LIQUID: artifacts below equilibrium
// TODO FLUID: LIQUID: water level way below equilibrium
EulerianLiquid::EulerianLiquid(
    ref_ptr<EulerianPrimitive> primitive,
    ref_ptr<EulerianObstacles> obstacles)
: EulerianFluid(primitive, obstacles),
  liquidHeight_( ref_ptr<UniformFloat>::manage(
      new UniformFloat("liquidHeight") ))
{
  /*
  const float cellSize_ = 1.25f;
  const float velocityDecay_ = 1.0f;
  const float levelSetDecay_ = 1.0f;
  const float fluidDensity = 0.365f;
  const float pressureAlpha = 1.0f;
  const int numPressureIterations = 50;
  const int redistanceIterations = 11;
  const int extrapolationIterations = 20;
   */
  const float cellSize_ = 1.25f;
  const float velocityDecay_ = 1.0f;
  const float levelSetDecay_ = 1.0f;
  const float fluidDensity = 0.37489f;
  const float pressureAlpha = 1.0f;
  const int numPressureIterations = 40;
  const int redistanceIterations = 11;
  const int extrapolationIterations = 10;

  const bool advectEverything = false;
  useVorticity_ = false;
  useGravity_ = true;

  gravityVec_ = (Vec3f) {0.0f, -120.0f, 0.0f};

  liquidHeight_->set_value(40.0f);
  distanceToLiquidHeightShader_ = primitive_->makeShader(
      DistanceToLiquidHeightShader(this), "float" );
  distanceToLiquidHeightShader_->addUniform(liquidHeight_.get());

  levelSetBuffer_ = createSlabWithName(EulerianLiquid::LEVEL_SET, 1, 2);
  initialLevelSetTexture_ = createTexture(
      primitive_->width(), primitive_->height(), primitive_->depth(), 1, 1, true);
  initialLevelSetAttachment_ = levelSetBuffer_.fbo->addColorAttachment(
      *( (Texture2D*) initialLevelSetTexture_.get() )) - GL_COLOR_ATTACHMENT0;
  resetLevelSet();

  vorticityStage_->set_vorticityConfinementScale(0.12f);

  {
    pressureStage_->set_alpha(pressureAlpha);
    pressureStage_->set_inverseBeta(primitive_->is2D() ? (1.0f/4.0f) : (1.0f/6.0f));
    pressureStage_->set_halfInverseCellSize(0.5f/cellSize_);
    pressureStage_->set_fluidDensity(fluidDensity);
    pressureStage_->set_numPressureIterations(numPressureIterations);
    pressureStage_->set_levelSetTexture(levelSetBuffer_.tex);
  }

  {
    advectStage_->set_levelSetTexture(levelSetBuffer_.tex);

    // Use no decay for Level Set advection
    velocityAdvectionTarget_->decayAmount = velocityDecay_;
    velocityAdvectionTarget_->quantityLoss = 0.0f;
    velocityAdvectionTarget_->treatAsLiquid = !advectEverything;

    levelSetAdvectionTarget_ = ref_ptr<AdvectionTarget>::manage(
        new AdvectionTarget());
    levelSetAdvectionTarget_->buffer = levelSetBuffer_;
    levelSetAdvectionTarget_->decayAmount = levelSetDecay_;
    levelSetAdvectionTarget_->quantityLoss = 0.0f;
    levelSetAdvectionTarget_->treatAsLiquid = false;
    advectStage_->addAdvectionTarget(levelSetAdvectionTarget_);
  }

  {
    liquidStreamStage_ = ref_ptr<EulerianLiquidStream>::manage(
        new EulerianLiquidStream(primitive_.get()));
    liquidStreamStage_->set_levelSetBuffer( levelSetBuffer_ );
    liquidStreamStage_->set_obstaclesTexture( obstacles_->tex() );
    liquidStreamStage_->set_velocityBuffer( velocityBuffer_ );
  }

  {
    ref_ptr<InjectLiquidStage> injectLiquidStage = ref_ptr<InjectLiquidStage>::manage(
        new InjectLiquidStage(primitive_.get(), liquidHeight_.get()));
    injectLiquidStage->set_levelSetBuffer( levelSetBuffer_ );
    injectLiquidStage_ = injectLiquidStage;
  }

  {
    redistancelevelSetStage_ = ref_ptr<EulerianRedistance>::manage(
        new EulerianRedistance(primitive_.get()));
    redistancelevelSetStage_->set_levelSetBuffer(levelSetBuffer_);
    redistancelevelSetStage_->set_initialLevelSetTexture(initialLevelSetTexture_);
    redistancelevelSetStage_->set_numRedistanceIterations(redistanceIterations);
  }

  {
    extrapolateVelocityStage_ = ref_ptr<EulerianExtrapolateVelocity>::manage(
        new EulerianExtrapolateVelocity(primitive_.get()));
    extrapolateVelocityStage_->set_levelSetTexture(levelSetBuffer_.tex);
    extrapolateVelocityStage_->set_obstaclesTexture(obstacles_->tex());
    extrapolateVelocityStage_->set_velocityBuffer(velocityBuffer_);
    extrapolateVelocityStage_->set_numOfExtrapolationIterations(extrapolationIterations);
  }

  // set default stages...
  // note: this is a call to EulerianLiquid::setDefaultStages
  EulerianLiquid::setDefaultStages();
}

void EulerianLiquid::setDefaultStages()
{
  stages_.clear();
  addStage(advectStage_);
  addStage(liquidStreamStage_);
  //addStage(splatStage_);
  //addStage(injectLiquidStage_);
  addStage(pressureStage_);
  //addStage(redistancelevelSetStage_);
  addStage(extrapolateVelocityStage_);
  if(useGravity_) {
    ref_ptr<EulerianGravity> gravityStage_ = ref_ptr<EulerianGravity>::manage(
        new EulerianGravity(primitive_.get()));
    gravityStage_->set_obstaclesTexture(obstacles_->tex());
    gravityStage_->set_levelSetTexture(levelSetBuffer_.tex);
    gravityStage_->set_velocityBuffer(velocityBuffer_);
    gravityStage_->addGravitySource(gravityVec_);
    addStage(gravityStage_);
  }
}

void EulerianLiquid::resetLevelSet()
{
  primitive_->bind();
  levelSetBuffer_.fbo->setAttachmentActive(initialLevelSetAttachment_, true);
  levelSetBuffer_.fbo->bind();
  levelSetBuffer_.fbo->drawBufferMRT();
  primitive_->enableShader(distanceToLiquidHeightShader_);
  primitive_->draw();
  swapBuffer(levelSetBuffer_);
}

void EulerianLiquid::reset()
{
  EulerianFluid::reset();
  resetLevelSet();
}
