/*
 * fluid.h
 *
 *  Created on: 20.02.2012
 *      Author: daniel
 */

#ifndef EULERIAN_GRID_H_
#define EULERIAN_GRID_H_

#include <shader.h>
#include <ref-ptr.h>
#include <render-pass.h>

#include "external-forces.h"
#include "advect.h"
#include "pressure.h"
#include "vorticity.h"
#include "obstacles.h"
#include "helper.h"

using namespace std;
#include <list>
#include <map>

/**
 * Called after each update step.
 * You may want to render the result to texture here.
 */
class PostFluidUpdateInterface {
public:
  virtual void postUpdate() = 0;
};

/**
 * Realizes grid based fluid simulation.
 * Each cell of the grid contains quantities that are transported
 * through the grid using a velocity field.
 */
class EulerianFluid : public RenderPass
{
public:
  static const string VELOCITY;
  static const string PRESSURE;
  static const string DIVERGENCE;
  static const string VORTICITY;
  static const string OBSTACLES;

  EulerianFluid(
      ref_ptr<EulerianPrimitive> primitive,
      ref_ptr<EulerianObstacles> obstacles);

  ////////////

  /**
   * Set the lifetime of the velocity.
   * Range is 0.0 to 1.0, 0.0 means velocity disappears immediately
   * and 1.0 means it lives forever. Anyway due to numerical instability
   * even with 1.0 the velocity may disappear.
   * Usually you want to set a value near 1.0 here, something like 0.999 or so.
   *
   * loss is a constant value subtracted from from the velocity each time step.
   */
  void setLifetimeVelocity(float lifetime=1.0f, float loss=0.0f) {
    velocityAdvectionTarget_->decayAmount = max(0.0f, min(1.0f, lifetime));
    velocityAdvectionTarget_->quantityLoss = loss;
  }

  /**
   * Number of iterations for pressure solve to converge to a good value.
   * A good value for real time application might be 20 to 40.
   */
  void setNumPressureIterations(int numPressureIterations) {
    pressureStage_->set_numPressureIterations(numPressureIterations);
  }

  /**
   * Sets factor for vorticity confinement.
   * Probably best to keep value below one, velocity field may go crazy else.
   */
  void setVorticityConfinement(float scale) {
    vorticityStage_->set_vorticityConfinementScale(scale);
  }

  /**
   * Toggles on/off the vorticity stage.
   */
  void set_useVorticity(bool b) {
    useVorticity_ = b;
    setDefaultStages();
  }
  bool useVorticity() const {
    return useVorticity_;
  }

  /**
   * Toggles on/off the gravity stage.
   */
  void set_useGravity(bool b) {
    useGravity_ = b;
    setDefaultStages();
  }
  bool useGravity() const {
    return useGravity_;
  }

  /**
   * Set the gravity vector. Has only influence if the gravity stage toggled on.
   */
  void set_gravityVec(const Vec3f& v) {
    gravityVec_ = v;
    setDefaultStages();
  }
  const Vec3f& gravityVec() const {
    return gravityVec_;
  }

  /**
   * Creates a quantity that gets advected by the velocity field.
   * This quantity has no influence on the simulation
   * as long as no extern stages are used to modify the velocity field
   * using this quatity.
   * @param name the name of the quantity
   * @param numDimensions dimensions of the texture (1 means only r, 2 means rg, ..)
   * @param lifetime lifetime factor [1,0]
   * @param loss lifetime loss [max,0]
   * @param treatAsLiquid liquids only advect inside the liquid domain
   */
  ref_ptr<AdvectionTarget> createPassiveQuantity(
      const string &name, unsigned int numDimensions,
      float lifetime, float loss, bool treatAsLiquid);

  /**
   * Adds a splat for a buffer created with createSlabWithName
   * @param quantityName name of the buffer (EulerianGrid::VELOCITY,...)
   * @param splat pos on texture (for 2D set only xy components)
   * @param val value (for scalar fields set only r component)
   * @param radius splat radius on texture
   */
  ref_ptr<SplatSource> addSplat(
      const string &quantityName,
      const Vec3f &pos,
      const Vec3f &val,
      float radius);
  ref_ptr<SplatSource> addSplat(
      const string &quantityName,
      const Vec3f &pos,
      const Vec3f &val,
      float width,
      float height);
  ref_ptr<SplatSource> addSplat(
          const string& quantityName,
          const Vec3f &val,
          ref_ptr<Texture> tex);
  /**
   * Remove previously added splat.
   */
  void removeSplat(ref_ptr<SplatSource> source);

  /**
   * Resets simulation to initial state.
   */
  virtual void reset();

  /**
   * Sets the function to call after each update step.
   */
  void setPostUpdate(ref_ptr<PostFluidUpdateInterface> postUpdate) {
    if(postUpdate.get() != NULL) postUpdate_ = postUpdate;
  }

  //////////////

  /**
   * Add a stage to the end of the stages list.
   * All stages are processed when render() called.
   */
  virtual void addStage(ref_ptr<EulerianStage> stage) {
    stages_.push_back(stage);
  }
  /**
   * Remove  previously added stage.
   */
  virtual void removeStage(ref_ptr<EulerianStage> stage) {
    stages_.remove(stage);
  }
  /**
   * Clears the stages list. Afterwards the simulation will do nothing
   * before you re-populate the fluid with stages.
   * You may use this to set your own pipeline of stages.
   */
  void clearStages() {
    stages_.clear();
  }
  /**
   * Sets the default stages for this fluid.
   * Note that you can clear stages and then re-populate
   * using add/removeStage.
   */
  virtual void setDefaultStages();

  /**
   * Creates texture with associated fbo and name.
   * You can add splats for these buffer names using addSplat().
   * @param name buffer name
   * @param numComponents num of components in texture format (1 means single scalar)
   * @param numTexs number of textures to allocate
   */
  FluidBuffer createSlabWithName(
      const string &name,
      int numComponents,
      int numTexs);

  EulerianPrimitive& primitive() {
    return *primitive_.get();
  }

  EulerianAdvection& advectStage() {
    return *advectStage_.get();
  }
  EulerianSplat& splatStage() {
    return *splatStage_.get();
  }
  EulerianPressure& pressureStage() {
    return *pressureStage_.get();
  }
  EulerianVorticity& vorticityStage() {
    return *vorticityStage_.get();
  }

  AdvectionTarget& velocityAdvectionTarget() {
    return *velocityAdvectionTarget_.get();
  }

  FluidBuffer velocityBuffer() {
    return velocityBuffer_;
  }
  FluidBuffer divergenceBuffer() {
    return divergenceBuffer_;
  }
  FluidBuffer pressureBuffer() {
    return pressureBuffer_;
  }
  FluidBuffer vorticityBuffer() {
    return vorticityBuffer_;
  }
  ref_ptr<Texture> obstaclesTexture() {
    return obstacles_->tex();
  }

  EulerianObstacles* obstacles() {
    return obstacles_.get();
  }

  // RenderPass overrides
  virtual bool rendersOnTop() { return false; }
  virtual bool usesDepthTest() { return false; }
  virtual void render();
  virtual void processStages();

protected:
  // contains some simulation properties
  ref_ptr<EulerianPrimitive> primitive_;

  // responsible for providing obstacles to the simulation
  ref_ptr<EulerianObstacles> obstacles_;

  // velocity field
  FluidBuffer velocityBuffer_;
  // divergence texture for pressure calculation
  FluidBuffer divergenceBuffer_;
  // pressure calculated by velocity and divergence
  FluidBuffer pressureBuffer_;
  // vorticity buffer if enabled
  FluidBuffer vorticityBuffer_;
  // remember name-buffer association
  map<string,FluidBuffer> quantityNameToBuffer_;

  list< ref_ptr<EulerianStage> > stages_;
  // transports quantities in the grid
  ref_ptr<EulerianAdvection> advectStage_;
  // splats values into the grid
  ref_ptr<EulerianSplat> splatStage_;
  // calculate pressure influence on velocity
  ref_ptr<EulerianPressure> pressureStage_;
  // calculate vorticity velocity confinment
  ref_ptr<EulerianVorticity> vorticityStage_;

  // properties for velocity advection
  ref_ptr<AdvectionTarget> velocityAdvectionTarget_;

  // flag enabling vorticity
  bool useVorticity_;
  // flag enabling gravity
  bool useGravity_;
  // gravity value
  Vec3f gravityVec_;

  // function to call after each fluid update
  ref_ptr<PostFluidUpdateInterface> postUpdate_;

  float dt_;
};

#endif /* EULERIAN_GRID_H_ */
