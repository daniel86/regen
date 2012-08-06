/*
 * utility-shader.h
 *
 *  Created on: 06.11.2011
 *      Author: daniel
 */

#ifndef UTILITY_SHADER_H_
#define UTILITY_SHADER_H_

#include <ogle/shader/shader-function.h>
#include <ogle/gl-types/shader.h>

/**
 * A shader that helps picking objects in the scene.
 */
class PickShaderVert : public ShaderFunctions {
public:
  PickShaderVert(const vector<string> &args);
  string code() const;
};
/**
 * A shader that helps picking objects in the scene.
 */
class PickShaderFrag : public ShaderFunctions {
public:
  PickShaderFrag(const vector<string> &args);
  string code() const;
};

#endif /* UTILITY_SHADER_H_ */
