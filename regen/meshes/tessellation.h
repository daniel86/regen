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
  struct TriangleVertex {
    TriangleVertex(const Vec3f &_p, const GLuint &_i) : p(_p), i(_i) {}
    TriangleVertex() : i(0) {}
    Vec3f p;
    GLuint i;
  };
  struct TriangleFace {
    TriangleFace(
        const TriangleVertex &_v1,
        const TriangleVertex &_v2,
        const TriangleVertex &_v3)
          : v1(_v1), v2(_v2), v3(_v3) {}
    TriangleFace() {}
    TriangleVertex v1;
    TriangleVertex v2;
    TriangleVertex v3;
  };

  vector<TriangleFace>* tessellate(GLuint lod, vector<TriangleFace> &inputFaces);
  vector<TriangleFace>* tessellate(GLuint lod, TriangleFace &inputFace);
}

#endif /* TESSELATION_H_ */
