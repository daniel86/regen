
#ifndef EULERIAN_GRID_FIRE_H_
#define EULERIAN_GRID_FIRE_H_

#include <fire-texture.h>
#include "fluid.h"
#include "stages.h"
#include "smoke.h"

/**
 * From GPU Gems3:
 * Fire is not very different from smoke except that we must store an additional quantity,
 * called the reaction coordinate, that keeps track of the time elapsed since gas was ignited.
 * A reaction coordinate of one indicates that the gas was just ignited, and a coordinate of
 * less than zero indicates that the fuel has been completely exhausted. The evolution of
 * these values is described by the following equation (from Nguyen et al. 2002):
 *
 *      d/dt * Y = -(u * NABLA) * Y - k
 *
 * In other words, the reaction coordinate is advected through the flow and decremented
 * by a constant amount (k) at each time step. In practice, this integration is performed by
 * passing a value for k to the advection routine, which is
 * added to the result of the advection. (This value should be nonzero only when advecting
 * the reaction coordinate.) Reaction coordinates do not have an effect on the dynamics
 * of the fluid but are later used for rendering (see Section 30.3).
 * one possible fire effect: a ball of fuel is
 * continuously generated near the bottom of the volume by setting the reaction coordinate
 * in a spherical region to one. For a more advanced treatment of flames, see Nguyen
 * et al. 2002.
 *
 * Rendering fire is similar to rendering smoke except that instead of blending values as we
 * march, we accumulate values that are determined by the reaction coordinate Y rather than
 * the smoke density (see Section 30.2.6). In particular, we use an artist-defined 1D texture
 * that maps reaction coordinates to colors in a way that gives the appearance of fire. A more
 * physically based discussion of fire rendering can be found in Nguyen et al. 2002.
 * The fire volume can also be used as a light source if desired. The simplest approach is to
 * sample the volume at several locations and treat each sample as a point light source.
 * The reaction coordinate and the 1D color texture can be used to determine the inten-
 * sity and color of the light. However, this approach can lead to severe flickering if not
 * enough point samples are used, and it may not capture the overall behavior of the light.
 * A different approach is to downsample the texture of reaction coordinates to an ex-
 * tremely low resolution and then use every voxel as a light source. The latter approach
 * will be less prone to flickering, but it won't capture any high-frequency lighting effects
 * (such as local lighting due to sparks).
 *
 */
class EulerianFire : public EulerianSmoke
{
public:
  static const string REACTION_COORDINATE;

  EulerianFire(
      ref_ptr<EulerianPrimitive> primitive,
      ref_ptr<EulerianObstacles> obstacles);

  virtual void reset();

protected:
  virtual void setDefaultStages();
};

#endif /* EULERIAN_GRID_SMOKE_H_ */
