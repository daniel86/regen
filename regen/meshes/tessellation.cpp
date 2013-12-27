/*
 * tessellation.cpp
 *
 *  Created on: Dec 27, 2013
 *      Author: daniel
 */

#include "tessellation.h"

namespace regen {
  vector<TriangleFace>* tessellate(GLuint lod, TriangleFace &in)
  {
    vector<TriangleFace> in_(1);
    in_[0] = in;
    return tessellate(lod,in_);
  }

  vector<TriangleFace>* tessellate(GLuint lod, vector<TriangleFace> &in)
  {
    GLuint inFaces  = in.size();
    // Each triangle is divided in 4 smaller triangles
    // for each LoD level.
    GLuint outFaces = pow(4.0,lod)*inFaces;
    GLuint counter = inFaces;
    Vec3f pa,pb,pc;
    GLuint i, j;

    vector<TriangleFace> *out_ = new vector<TriangleFace>(outFaces);
    vector<TriangleFace> &out = *out_;

    for(i=0; i<inFaces; ++i) out[i] = in[i];

    for (j=0; j<lod; ++j)
    {
      const GLuint lastCount = counter;
      for (i=0; i<lastCount; ++i)
      {
        pa = (out[i].p1 + out[i].p2)*0.5;
        pb = (out[i].p2 + out[i].p3)*0.5;
        pc = (out[i].p3 + out[i].p1)*0.5;

        out[counter] = TriangleFace( out[i].p1, pa, pc ); ++counter;
        out[counter] = TriangleFace( pa, out[i].p2, pb ); ++counter;
        out[counter] = TriangleFace( pb, out[i].p3, pc ); ++counter;
        out[i]       = TriangleFace( pa, pb, pc );
      }
    }

    return out_;
  }
}
