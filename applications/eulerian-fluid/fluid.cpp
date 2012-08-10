
#include "include/fluid.h"

#include "include/stages.h"
#include "include/liquid.h"
#include "include/helper.h"

const string EulerianFluid::VELOCITY = "velocityTex";
const string EulerianFluid::PRESSURE = "pressureTex";
const string EulerianFluid::DIVERGENCE = "divergenceTex";
const string EulerianFluid::VORTICITY = "vorticityTex";
const string EulerianFluid::OBSTACLES = "obstaclesTex";

/**
 * Initially do nothing after each update step.
 */
class VoidPostUpdate : public PostFluidUpdateInterface {
public:
  virtual void postUpdate() {}
};

EulerianFluid::EulerianFluid(
    ref_ptr<EulerianPrimitive> primitive,
    ref_ptr<EulerianObstacles> obstacles)
: RenderPass(),
  primitive_(primitive),
  obstacles_(obstacles),
  dt_(0.0f),
  useVorticity_(false),
  useGravity_(false),
  gravityVec_( (Vec3f) {0.0f, -0.04f, 0.0f} )
{
  const float velocityDecay_ = 1.0f;
  const int numPressureIterations = 60;

  const float cellSize_ = 1.25f;
  const float fluidDensity_ = cellSize_ / 2.25f;
  const float alpha_ = cellSize_*cellSize_*0.75;

  postUpdate_ = ref_ptr<PostFluidUpdateInterface>::manage(new VoidPostUpdate);

  // velocity buffer uses two textures with x/y or x/y/z component
  velocityBuffer_ = createSlabWithName(
      EulerianFluid::VELOCITY, primitive_->is2D() ? 2 : 3, 2);
  // vorticity buffer uses two textures with x/y or x/y/z component
  vorticityBuffer_ = createSlabWithName(
      EulerianFluid::VORTICITY, primitive_->is2D() ? 2 : 3, 2);
  // pressure buffer uses two textures with single scalar component
  pressureBuffer_ = createSlabWithName(
      EulerianFluid::PRESSURE, 1, 2);
  // divergence buffer needs only one texture because no swapping is done
  divergenceBuffer_ = createSlabWithName(
      EulerianFluid::DIVERGENCE, 3, 1);

  { // transport quantities
    advectStage_ = ref_ptr<EulerianAdvection>::manage(
        new EulerianAdvection(primitive_.get()));
    advectStage_->set_obstaclesTexture(obstacles_->tex());
    advectStage_->set_velocityTexture(velocityBuffer_.tex);
    velocityAdvectionTarget_ = ref_ptr<AdvectionTarget>::manage(new AdvectionTarget());
    velocityAdvectionTarget_->buffer = velocityBuffer_;
    velocityAdvectionTarget_->decayAmount = velocityDecay_;
    velocityAdvectionTarget_->quantityLoss = 0.0f;
    velocityAdvectionTarget_->treatAsLiquid = false;
    advectStage_->addAdvectionTarget(velocityAdvectionTarget_);
  }

  { // add quantities
    splatStage_ = ref_ptr<EulerianSplat>::manage(
        new EulerianSplat(primitive_.get()));
    splatStage_->set_obstaclesTexture(obstacles_->tex());
  }

  {  // pressure solve
    pressureStage_ = ref_ptr<EulerianPressure>::manage(
        new EulerianPressure(primitive_.get()));
    pressureStage_->set_pressureBuffer(pressureBuffer_);
    pressureStage_->set_divergenceBuffer(divergenceBuffer_);
    pressureStage_->set_velocityBuffer(velocityBuffer_);
    pressureStage_->set_obstaclesTexture(obstacles_->tex());
    pressureStage_->set_alpha(alpha_);
    pressureStage_->set_inverseBeta((primitive_->is2D() ? (1.0f/4.0f) : (1.0f/6.0f)));
    pressureStage_->set_numPressureIterations(numPressureIterations);
    pressureStage_->set_fluidDensity(fluidDensity_);
    pressureStage_->set_halfInverseCellSize(0.5f / cellSize_);
  }

  {  // vorticity confinement
    vorticityStage_ = ref_ptr<EulerianVorticity>::manage(
        new EulerianVorticity(primitive_.get()));
    vorticityStage_->set_obstaclesTexture(obstacles_->tex());
    vorticityStage_->set_velocityBuffer(velocityBuffer_);
    vorticityStage_->set_vorticityBuffer(vorticityBuffer_);
    vorticityStage_->set_vorticityConfinementScale(0.12f);
  }

  // set default stages...
  // note: this is a call to EulerianFluid::setDefaultStages
  EulerianFluid::setDefaultStages();
}

void EulerianFluid::processStages()
{
  // process stages, transport quantities and update ink textures
  for(list< ref_ptr<EulerianStage> >::iterator it=stages_.begin();
      it!=stages_.end(); ++it)
  {
    (*it)->update();
  }
}

void EulerianFluid::render()
{
  float dt = primitive_->timeStep()->value();

  glDisable(GL_DEPTH_TEST); glDepthMask(0);

  // setup vertex data for textured quad
  primitive_->bind();
  // obstacles may want to update position / velocity
  obstacles_->update(dt);

  static const bool realTime_ = true;
  if(realTime_) {
    processStages();
  } else {
    const float simulationTimestep_ = 1.0;
    const float updateIntevalMiliSeconds_ = 20.0f;
    dt_ += primitive_->timeStep()->value()*1000.0;
    if(dt_ > updateIntevalMiliSeconds_) {
      primitive_->timeStep()->set_value(simulationTimestep_);
      processStages();
      primitive_->timeStep()->set_value(dt);
      dt_ = 0.0f;
    }
  }

  glDepthMask(1); glEnable(GL_DEPTH_TEST);
  handleGLError("after EulerianGrid::render");

  postUpdate_->postUpdate();
  handleGLError("after EulerianGrid::postUpdate");
}

//////////

ref_ptr<AdvectionTarget> EulerianFluid::createPassiveQuantity(
    const string &name, unsigned int numDimensions,
    float lifetime, float loss, bool treatAsLiquid)
{
  ref_ptr<AdvectionTarget> t = ref_ptr<AdvectionTarget>::manage(new AdvectionTarget);
  t->buffer = createSlabWithName(name, numDimensions, 2);
  t->decayAmount = lifetime;
  t->quantityLoss = loss;
  t->treatAsLiquid = false;
  advectStage_->addAdvectionTarget(t);
  return t;
}

void EulerianFluid::setDefaultStages()
{
  stages_.clear();
  addStage(advectStage_);
  if(useVorticity_) addStage(vorticityStage_);
  addStage(splatStage_);
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

void EulerianFluid::reset()
{
  clearSlab(velocityBuffer_);
  clearSlab(vorticityBuffer_);
  clearSlab(pressureBuffer_);
  clearSlab(divergenceBuffer_);
  handleGLError("after EulerianGrid::reset");
  dt_ = 0.0f;
}

//////////

ref_ptr<SplatSource> EulerianFluid::addSplat(
    const string& quantityName,
    const Vec3f &pos,
    const Vec3f &val,
    float radius)
{
  map<string,FluidBuffer>::iterator it = quantityNameToBuffer_.find(quantityName);
  if(it == quantityNameToBuffer_.end()) {
    return ref_ptr<SplatSource>();
  }

  ref_ptr<SplatSource> source = ref_ptr<SplatSource>::manage(new SplatSource);
  source->buffer = it->second;
  source->value = val;
  source->pos = pos;
  source->mode = 0;
  source->radius = radius;
  splatStage_->addSplatSource(source);
  return source;
}

ref_ptr<SplatSource> EulerianFluid::addSplat(
    const string& quantityName,
    const Vec3f &val,
    ref_ptr<Texture> tex)
{
  map<string,FluidBuffer>::iterator it = quantityNameToBuffer_.find(quantityName);
  if(it == quantityNameToBuffer_.end()) {
    return ref_ptr<SplatSource>();
  }

  ref_ptr<SplatSource> source = ref_ptr<SplatSource>::manage(new SplatSource);
  source->buffer = it->second;
  source->value = val;
  source->mode = 1;
  source->tex = tex;
  splatStage_->addSplatSource(source);
  return source;
}

ref_ptr<SplatSource> EulerianFluid::addSplat(
    const string& quantityName,
    const Vec3f &pos,
    const Vec3f &val,
    float width,
    float height)
{
  map<string,FluidBuffer>::iterator it = quantityNameToBuffer_.find(quantityName);
  if(it == quantityNameToBuffer_.end()) {
    return ref_ptr<SplatSource>();
  }

  ref_ptr<SplatSource> source = ref_ptr<SplatSource>::manage(new SplatSource);
  source->buffer = it->second;
  source->value = val;
  source->mode = 2;
  source->pos = pos;
  source->width = width;
  source->height = height;
  splatStage_->addSplatSource(source);
  return source;
}

void EulerianFluid::removeSplat(ref_ptr<SplatSource> source)
{
  splatStage_->removeSplatSource(source);
}

//////////

FluidBuffer EulerianFluid::createSlabWithName(
    const string &name, int numComponents, int numTexs)
{
  FluidBuffer surface = createSlab(primitive_.get(),
      numComponents, numTexs, primitive_->useHalfFloats());
  quantityNameToBuffer_.insert(make_pair( name, surface ));
  return surface;
}
