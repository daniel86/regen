/*
 * frustum.h
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#ifndef FRUSTUM_H_
#define FRUSTUM_H_

#include <vector>

#include <GL/glew.h>
#include <regen/math/vector.h>
#include <regen/math/plane.h>

namespace regen {
  /**
   * A portion of a solid pyramid that lies between two parallel planes cutting it.
   */
  struct Frustum {
    /** The field of view angle. */
    GLdouble fov;
    /** The aspect ratio. */
    GLdouble aspect;
    /** The near plane distance. */
    GLdouble near;
    /** The far plane distance. */
    GLdouble far;
    /** Near plane size. */
    Vec2f nearPlane;
    /** Far plane size. */
    Vec2f farPlane;
    /** The 8 frustum points. */
    Vec3f points[8];
    /** The 6 frustum planes. */
    Plane planes[6];

    /**
     * Set projection parameters and compute near- and far-plane.
     */
    void set(GLfloat aspect, GLfloat fov, GLfloat near, GLfloat far);
    /**
     * Update frustum points based on view point and direction.
     */
    void update(const Vec3f &pos, const Vec3f &dir);
    /**
     * Split this frustum along the view ray.
     */
    std::vector<Frustum*> split(GLuint count, GLdouble splitWeight) const;
    /**
     * @return true if the sphere intersects with this frustum.
     */
    GLboolean hasIntersectionWithSphere(const Vec3f &center, GLfloat radius);
    /**
     * @return true if the box intersects with this frustum.
     */
    GLboolean hasIntersectionWithBox(const Vec3f &center, const Vec3f *points);
  };
} // namespace

#endif /* FRUSTUM_H_ */
