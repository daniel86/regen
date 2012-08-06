/*
 * debugging-shader.h
 *
 *  Created on: 06.11.2011
 *      Author: daniel
 */

#ifndef DEBUGGING_SHADER_H_
#define DEBUGGING_SHADER_H_

#include <ogle/shader/shader-function.h>

/**
 * Visualize Depth Shader for debugging purpose.
 */
class VisualizeDepthShaderVert : public ShaderFunctions {
public:
  VisualizeDepthShaderVert(const vector<string> &args);
  string code() const;
};
/**
 * Visualize Depth Shader for debugging purpose.
 */
class VisualizeDepthShaderFrag  : public ShaderFunctions {
public:
  VisualizeDepthShaderFrag(const vector<string> &args);
  string code() const;
};

#endif /* DEBUGGING_SHADER_H_ */
