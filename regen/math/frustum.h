/*
 * frustum.h
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#ifndef FRUSTUM_H_
#define FRUSTUM_H_

using namespace std;
#include <vector>

#include <GL/glew.h>
#include <regen/math/vector.h>

namespace regen {
  struct Frustum {
    GLdouble fov;
    GLdouble aspect;
    GLdouble near;
    GLdouble far;
    Vec2f nearPlane;
    Vec2f farPlane;
    Vec3f points[8];

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
    vector<Frustum*> split(GLuint count, GLdouble splitWeight) const;
  };
} // namespace

#endif /* FRUSTUM_H_ */
