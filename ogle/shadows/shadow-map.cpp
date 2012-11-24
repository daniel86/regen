/*
 * shadow-map.cpp
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#include "shadow-map.h"

ShadowMap::ShadowMap()
: Animation()
{
}

void ShadowMap::addCaster(ref_ptr<StateNode> &caster)
{
  caster_.push_back(caster);
}
void ShadowMap::removeCaster(StateNode *caster)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    ref_ptr<StateNode> &n = *it;
    if(n.get()==caster) {
      caster_.erase(it);
      break;
    }
  }
}

void ShadowMap::animate(GLdouble dt)
{
}
