/*
 * normal-mapping.h
 *
 *  Created on: 03.02.2011
 *      Author: daniel
 */

#ifndef _NORMAL_MAPPING_H_
#define _NORMAL_MAPPING_H_

#include <vector>
using namespace std;

#include <ogle/shader/shader-function.h>
#include <ogle/states/light-state.h>

/**
 * Shader that sets the normal per fragment using a normal map.
 */
class BumpMapFrag : public ShaderFunctions {
public:
  BumpMapFrag(vector<string> &args, GLboolean isTwoSided);
  string code() const;
  GLboolean isTwoSided_;
};
/**
 * Shader that sets the normal per fragment using a normal map.
 */
class BumpMapVert : public ShaderFunctions {
public:
  BumpMapVert(vector<string> &args,
      const list<Light*> &lights);
  string code() const;
private:
  list<Light*> lights_;
};

#endif /* GL_NORMAL_MAPPING_H_ */
