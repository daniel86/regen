/*
 * raycast-shader.h
 *
 *  Created on: 03.02.2011
 *      Author: daniel
 */

#ifndef _RAYCAST_SHADER_H_
#define _RAYCAST_SHADER_H_

#include <ogle/shader/shader-function.h>
#include <ogle/states/texture-state.h>

class RayCastShader : public ShaderFunctions {
public:
  RayCastShader(TextureState *texture, vector<string> &args);
  string code() const;
private:
  TextureState *texture_;
};

#endif /* _RAYCAST_SHADER_H_ */
