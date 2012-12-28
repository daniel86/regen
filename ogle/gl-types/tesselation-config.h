/*
 * tesselation-config.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef ___TESSELATION_H_
#define ___TESSELATION_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <ogle/algebra/vector.h>

typedef enum {
  TESS_PRIMITVE_TRIANGLES,
  TESS_PRIMITVE_QUADS,
  TESS_PRIMITVE_ISOLINES
}TessPrimitive;
typedef enum {
  TESS_SPACING_EQUAL,
  TESS_SPACING_FRACTIONAL_EVEN,
  TESS_SPACING_FRACTIONAL_ODD
}TessVertexSpacing;
typedef enum {
  TESS_ORDERING_CCW,
  TESS_ORDERING_CW,
  TESS_ORDERING_POINT_MODE
}TessVertexOrdering;
/**
 * LoD distance metric for adaptive tesselation.
 */
typedef enum {
  TESS_LOD_EDGE_SCREEN_DISTANCE,
  TESS_LOD_EDGE_DEVICE_DISTANCE,
  TESS_LOD_CAMERA_DISTANCE_INVERSE
}TessLodMetric;

struct Tesselation {
  TessPrimitive primitive;
  TessVertexOrdering ordering;
  TessVertexSpacing spacing;
  GLuint numPatchVertices;
  // LoD distance metric for adaptive tesselation
  TessLodMetric lodMetric;
  // for !isAdaptive no TCS is used
  GLboolean isAdaptive;
  // only used if !isAdaptive
  Vec4f defaultOuterLevel;
  // only used if !isAdaptive
  Vec4f defaultInnerLevel;
  Tesselation(TessPrimitive _primitive, GLuint _numPatchVertices)
  : primitive(_primitive),
    ordering(TESS_ORDERING_CCW),
    spacing(TESS_SPACING_FRACTIONAL_ODD),
    numPatchVertices(_numPatchVertices),
    lodMetric(TESS_LOD_EDGE_DEVICE_DISTANCE),
    isAdaptive(GL_TRUE),
    defaultOuterLevel(Vec4f(8.0f, 8.0f, 8.0f, 8.0f)),
    defaultInnerLevel(Vec4f(8.0f, 8.0f, 8.0f, 8.0f))
  {
  }
};

#endif /* ___TESSELATION_H_ */
