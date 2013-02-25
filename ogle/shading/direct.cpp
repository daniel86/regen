/*
 * direct.cpp
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#include "direct.h"
#include <ogle/utility/string-util.h>

DirectShading::DirectShading() : State()
{
  shaderDefine("NUM_LIGHTS", "0");
}

void DirectShading::addLight(const ref_ptr<Light> &l)
{
  GLuint numLights = lights_.size();
  // map for loop index to light id
  shaderDefine(
      FORMAT_STRING("LIGHT" << numLights << "_ID"),
      FORMAT_STRING(l->id()));
  // remember the number of lights used
  shaderDefine("NUM_LIGHTS", FORMAT_STRING(numLights+1));

  joinStatesFront(ref_ptr<State>::cast(l));
  lights_.push_back(l);
}
void DirectShading::removeLight(const ref_ptr<Light> &l)
{
  for(list< ref_ptr<Light> >::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    ref_ptr<Light> &x = *it;
    if(x.get()==l.get()) {
      lights_.erase(it);
      break;
    }
  }
  disjoinStates(ref_ptr<State>::cast(l));

  GLuint numLights = lights_.size(), lightIndex=0;
  // update shader defines
  shaderDefine("NUM_LIGHTS", FORMAT_STRING(numLights));
  for(list< ref_ptr<Light> >::iterator
      it=lights_.begin(); it!=lights_.end(); ++it)
  {
    ref_ptr<Light> &x = *it;
    shaderDefine(
        FORMAT_STRING("LIGHT" << lightIndex << "_ID"),
        FORMAT_STRING(x->id()));
    ++lightIndex;
  }
}
