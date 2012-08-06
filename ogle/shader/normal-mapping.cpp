/*
 * normal_mapping.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include <sstream>

#include "normal-mapping.h"
#include <ogle/gl-types/vertex-attribute.h>

/**
 * normal mapping fragment shader.
 * this shader function takes the normal, modifies it and returns it.
 * this technique only changes the normal, advanced techniques may
 * modify texture and vertex coordinates.
 */
BumpMapFrag::BumpMapFrag(vector<string> &args)
: ShaderFunctions("bump", args)
{
}

string BumpMapFrag::code() const
{
  stringstream s;
  s << "void bump(vec4 texel, inout vec3 normal)" << endl;
  s << "{" << endl;
  s << "    normal = normalize( texel.xyz * 2.0 - 1.0 );" << endl;
  s << "}" << endl;
  return s.str();
}

BumpMapVert::BumpMapVert(vector<string> &args,
    const list<Light*> &lights)
: ShaderFunctions("bump", args),
  lights_(lights)
{

}
string BumpMapVert::code() const
{
  stringstream s;
  s << "void bump(vec3 n, vec4 tan, vec4 pos, out vec4 posTangent)" << endl;
  s << "{" << endl;
  s << "    // get the tangent in eye space (multiplication by gl_NormalMatrix transforms to eye space)" << endl;
  s << "    // the tangent should point in positive u direction on the uv plane in the tangent space." << endl;
  s << "    vec3 t = normalize( tan.xyz );" << endl;
  s << "    // calculate the binormal, cross makes sure tbn matrix is orthogonal" << endl;
  s << "    // multiplicated by handeness." << endl;
  s << "    vec3 b = cross(n, t) * tan.w;" << endl;
  s << "    // transpose tbn matrix will do the transformation to tangent space" << endl;
  s << "    vec3 buf;" << endl;
  s << "" << endl;
  s << "    // do the transformation of the eye vector (used for specuar light)" << endl;
  s << "    buf.x = dot( pos.xyz, t );" << endl;
  s << "    buf.y = dot( pos.xyz, b );" << endl;
  s << "    buf.z = dot( pos.xyz, n );" << endl;
  s << "    posTangent = normalize( vec4(buf,pos.w) );" << endl;
  s << "" << endl;
  s << "    // do the transformation of the light vectors" << endl;

  for(unsigned int i=0; i<lights_.size(); ++i)
  {
    s << "    buf.x = dot( f_lightVec[" << i << "], t );" << endl;
    s << "    buf.y = dot( f_lightVec[" << i << "], b );" << endl;
    s << "    buf.z = dot( f_lightVec[" << i << "], n ) ;" << endl;
    s << "    f_lightVec[" << i << "] = normalize( buf  );" << endl;
  }
  s << "}" << endl;

  return s.str();
}
