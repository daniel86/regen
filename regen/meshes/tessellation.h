/*
 * tesselation.h
 *
 *  Created on: Dec 27, 2013
 *      Author: daniel
 */

#ifndef TESSELLATION_H_
#define TESSELLATION_H_

#include <vector>
using namespace std;

#include <regen/math/vector.h>

namespace regen {
  struct TriangleFace {
    TriangleFace(const Vec3f &_p1, const Vec3f &_p2, const Vec3f &_p3)
          : p1(_p1), p2(_p2), p3(_p3) {}
    TriangleFace() {}
    Vec3f p1;
    Vec3f p2;
    Vec3f p3;
  };

  vector<TriangleFace>* tessellate(GLuint lod, vector<TriangleFace> &inputFaces);
  vector<TriangleFace>* tessellate(GLuint lod, TriangleFace &inputFace);
}

#endif /* TESSELATION_H_ */
