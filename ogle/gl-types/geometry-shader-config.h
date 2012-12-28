/*
 * geometry-shader-config.h
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#ifndef GEOMETRY_SHADER_CONFIG_H_
#define GEOMETRY_SHADER_CONFIG_H_

enum GeometryShaderInput {
  GS_INPUT_POINTS,
  GS_INPUT_LINES,
  GS_INPUT_LINES_ADJACENCY,
  GS_INPUT_TRIANGLES,
  GS_INPUT_TRIANGLES_ADJACENCY
};
enum GeometryShaderOutput {
  GS_OUTPUT_POINTS,
  GS_OUTPUT_LINE_STRIP,
  GS_OUTPUT_TRIANGLE_STRIP
};
struct GeometryShaderConfig {
  GeometryShaderInput input;
  // As of OpenGL 4.0, a geometry shader can be
  // invoked more than once for each input primitive.
  GLuint invocations;

  GeometryShaderOutput output;
  // The max_vertices limits the number of vertices that the geometry
  // shader will output. If we try to output more vertices than stated,
  // the exceeding vertices will not be sent to the remaining of the pipeline.
  GLuint maxVertices;

  GeometryShaderConfig()
  : input(GS_INPUT_TRIANGLES),
    invocations(1),
    output(GS_OUTPUT_TRIANGLE_STRIP),
    maxVertices(0)
  {
  }
  GeometryShaderConfig(
      GeometryShaderInput _input,
      GLuint _invocations,
      GeometryShaderOutput _output,
      GLuint _maxVertices)
  : input(_input),
    invocations(_invocations),
    output(_output),
    maxVertices(_maxVertices)
  {
  }
};

#endif /* GEOMETRY_SHADER_CONFIG_H_ */
