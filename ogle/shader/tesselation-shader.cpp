/*
 * tesselation.cpp
 *
 *  Created on: 11.04.2012
 *      Author: daniel
 */

#include "tesselation-shader.h"

// When you transform a vertex by the projection matrix, you get clip coordinate.
// After w division this is called normalized device coordinates.
//    if z is from -1.0 to 1.0, then it is inside the znear and zfar clipping planes.
static const string worldToDeviceSpace =
"// A helper function to project a world space vertex to device normal space\n"
"vec4 worldToDeviceSpace(vec4 vertexWS){\n"
"    vec4 vertexNDS = in_viewProjectionMatrix * vertexWS;\n"
"    vertexNDS /= vertexNDS.w;\n"
"    return vertexNDS;\n"
"}\n";

static const string deviceToScreenSpace =
"// This helper function converts a device normal space vector to screen space\n"
"vec2 deviceToScreenSpace(vec4 vertexDS, vec2 screen){\n"
"    return (vertexDS.xy*0.5 + vec2(0.5))*screen;\n"
"}\n";

// normalized device space is a unit cube,
// only coordinates in this range are on screen.
static const string isOffscreenNDC =
"// test a vertex in device normal space against the view frustum\n"
"bool isOffscreenNDC(vec4 v){\n"
"    return v.x<-1.0 || v.y<-1.0 || v.z<-1.0 || v.x>1.0 || v.y>1.0 || v.z>1.0;\n"
"}\n";

static const string metricSreenDistance =
"float metricSreenDistance(vec2 v0, vec2 v1, float factor){\n"
"     const float min = 7.0; \n"
"     float max = in_viewport.x*0.2; \n"
"     float d = (clamp(distance(v0,v1), min, max) - min)/(max-min);\n"
"     return clamp( d*64.0*factor, 1, 64);\n"
"}\n";
static const string metricDeviceDistance =
"float metricDeviceDistance(vec2 v0, vec2 v1, float factor){\n"
"     const float min = 0.025; \n"
"     const float max = 0.2; \n"
"     float d = (clamp(distance(v0,v1), min, max) - min)/(max-min);\n"
"     return clamp( d*64.0*factor, 1, 64);\n"
"}\n";
static const string metricCameraDistance =
"float metricCameraDistance(vec3 v, float factor){\n"
"     const float min = 4.0; \n"
"     const float max = 50.0; \n"
"     float d = (max - clamp(distance(v,in_cameraPosition), min, max))/(max-min);\n"
"     return clamp( d*64.0*factor, 1, 64);\n"
"}\n";

const string tessWorldPos(GLint i, bool useDisplacement)
{
  stringstream s;
  if(useDisplacement) {
    s << "  gl_in["<<i<<"].gl_Position + vec4(in_displacement["<<i<<"],0.0);" << endl;
  } else {
    s << "  gl_in["<<i<<"].gl_Position;" << endl;
  }
  return s.str();
}

TesselationControlNDC::TesselationControlNDC(
    const vector<string> &args,
    bool useDisplacement)
: ShaderFunctions("tessControl", args),
  useDisplacement_(useDisplacement),
  metric_(TESS_LOD_EDGE_DEVICE_DISTANCE)
{
  addUniform( GLSLUniform( "vec3", "in_cameraPosition" ) );
  addUniform( GLSLUniform( "vec2", "in_viewport" ) );
  addUniform( GLSLUniform( "float", "in_lodFactor" ) );
  addDependencyCode("isOffscreenNDC", isOffscreenNDC);
  addDependencyCode("worldToDeviceSpace", worldToDeviceSpace);
  addDependencyCode("deviceToScreenSpace", deviceToScreenSpace);

  addDependencyCode("metricCameraDistance", metricCameraDistance);
  addDependencyCode("metricDeviceDistance", metricDeviceDistance);
  addDependencyCode("metricSreenDistance", metricSreenDistance);
  if(useDisplacement_) addInput(GLSLTransfer("vec3", "in_displacement"));
}

QuadTesselationControl::QuadTesselationControl(
    const vector<string> &args,
    bool useDisplacement)
: TesselationControlNDC(args, useDisplacement)
{
  set_tessNumVertices( 4 );
}
string QuadTesselationControl::code() const
{
  stringstream s;
  s << "void tessControl(){" << endl;
  s << "  if(gl_InvocationID != 0) return;" << endl;
  s << "  // quad vertex positions" << endl;
  s << "  vec4 ws0 = " << tessWorldPos(0,useDisplacement_) << ";" << endl;
  s << "  vec4 ws1 = " << tessWorldPos(1,useDisplacement_) << ";" << endl;
  s << "  vec4 ws2 = " << tessWorldPos(2,useDisplacement_) << ";" << endl;
  s << "  vec4 ws3 = " << tessWorldPos(3,useDisplacement_) << ";" << endl;
  s << "  vec4 nds0 = worldToDeviceSpace(ws0);" << endl;
  s << "  vec4 nds1 = worldToDeviceSpace(ws1);" << endl;
  s << "  vec4 nds2 = worldToDeviceSpace(ws2);" << endl;
  s << "  vec4 nds3 = worldToDeviceSpace(ws3);" << endl;
  s << "  if(isOffscreenNDC(nds0) && isOffscreenNDC(nds1) &&" << endl;
  s << "     isOffscreenNDC(nds2) && isOffscreenNDC(nds3) ) {" << endl;
  s << "    // if the patch is not visible on screen do not tesselate" << endl;
  s << "    gl_TessLevelInner[0] = gl_TessLevelInner[1] = 0;" << endl;
  s << "    gl_TessLevelOuter = float[4]( 0, 0, 0, 0 );" << endl;
  s << "  } else{" << endl;
  switch(metric_) {
  case TESS_LOD_EDGE_SCREEN_DISTANCE:
    s << "    vec2 ss0 = deviceToScreenSpace(nds0, in_viewport);" << endl;
    s << "    vec2 ss1 = deviceToScreenSpace(nds1, in_viewport);" << endl;
    s << "    vec2 ss2 = deviceToScreenSpace(nds2, in_viewport);" << endl;
    s << "    vec2 ss3 = deviceToScreenSpace(nds3, in_viewport);" << endl;
    s << "    float e0 = metricSreenDistance(ss1.xy, ss2.xy, in_lodFactor);" << endl;
    s << "    float e1 = metricSreenDistance(ss0.xy, ss1.xy, in_lodFactor);" << endl;
    s << "    float e2 = metricSreenDistance(ss3.xy, ss0.xy, in_lodFactor);" << endl;
    s << "    float e3 = metricSreenDistance(ss2.xy, ss3.xy, in_lodFactor);" << endl;
    break;
  case TESS_LOD_EDGE_DEVICE_DISTANCE:
    s << "    float e0 = metricDeviceDistance(nds1.xy, nds2.xy, in_lodFactor);" << endl;
    s << "    float e1 = metricDeviceDistance(nds0.xy, nds1.xy, in_lodFactor);" << endl;
    s << "    float e2 = metricDeviceDistance(nds3.xy, nds0.xy, in_lodFactor);" << endl;
    s << "    float e3 = metricDeviceDistance(nds2.xy, nds3.xy, in_lodFactor);" << endl;
    break;
  case TESS_LOD_CAMERA_DISTANCE_INVERSE:
    s << "    float e0 = metricCameraDistance(ws0.xyz, in_lodFactor);" << endl;
    s << "    float e1 = metricCameraDistance(ws1.xyz, in_lodFactor);" << endl;
    s << "    float e2 = metricCameraDistance(ws2.xyz, in_lodFactor);" << endl;
    s << "    float e3 = metricCameraDistance(ws3.xyz, in_lodFactor);" << endl;
    break;
  }
  s << "    gl_TessLevelInner[0] = mix(e1, e2, 0.5);" << endl;
  s << "    gl_TessLevelInner[1] = mix(e0, e3, 0.5);" << endl;
  s << "    gl_TessLevelOuter = float[4]( e0, e1, e2, e3 );" << endl;
  s << "  }" << endl;
  s << "}" << endl;
  return s.str();
}


TriangleTesselationControl::TriangleTesselationControl(
    const vector<string> &args,
    bool useDisplacement)
: TesselationControlNDC(args, useDisplacement)
{
  set_tessNumVertices( 3 );
}
string TriangleTesselationControl::code() const
{
  stringstream s;
  s << "void tessControl(){" << endl;
  s << "  if(gl_InvocationID != 0) return;" << endl;
  s << "  // triangle vertex positions" << endl;
  s << "  vec4 ws0 = " << tessWorldPos(0,useDisplacement_) << ";" << endl;
  s << "  vec4 ws1 = " << tessWorldPos(1,useDisplacement_) << ";" << endl;
  s << "  vec4 ws2 = " << tessWorldPos(2,useDisplacement_) << ";" << endl;
  s << "  vec4 nds0 = worldToDeviceSpace(ws0);" << endl;
  s << "  vec4 nds1 = worldToDeviceSpace(ws1);" << endl;
  s << "  vec4 nds2 = worldToDeviceSpace(ws2);" << endl;
  s << "  if(isOffscreenNDC(nds0) && isOffscreenNDC(nds1) && isOffscreenNDC(nds2)) {" << endl;
  s << "    // if the patch is not visible on screen do not tesselate" << endl;
  s << "    gl_TessLevelInner[0] = 0;" << endl;
  s << "    gl_TessLevelOuter = float[4]( 0, 0, 0, 0 );" << endl;
  s << "  } else{" << endl;
  s << "    // The LOD calculation as a function of distance in normalized device space" << endl;
  switch(metric_) {
  case TESS_LOD_EDGE_SCREEN_DISTANCE:
    s << "    vec2 ss0 = deviceToScreenSpace(nds0, in_viewport);" << endl;
    s << "    vec2 ss1 = deviceToScreenSpace(nds1, in_viewport);" << endl;
    s << "    vec2 ss2 = deviceToScreenSpace(nds2, in_viewport);" << endl;
    s << "    float e0 = metricSreenDistance(ss1.xy, ss2.xy, in_lodFactor);" << endl;
    s << "    float e1 = metricSreenDistance(ss0.xy, ss1.xy, in_lodFactor);" << endl;
    s << "    float e2 = metricSreenDistance(ss2.xy, ss0.xy, in_lodFactor);" << endl;
    break;
  case TESS_LOD_EDGE_DEVICE_DISTANCE:
    s << "    float e0 = metricDeviceDistance(nds1.xy, nds2.xy, in_lodFactor);" << endl;
    s << "    float e1 = metricDeviceDistance(nds0.xy, nds1.xy, in_lodFactor);" << endl;
    s << "    float e2 = metricDeviceDistance(nds2.xy, nds0.xy, in_lodFactor);" << endl;
    break;
  case TESS_LOD_CAMERA_DISTANCE_INVERSE:
    s << "    float e0 = metricCameraDistance(ws0.xyz, in_lodFactor);" << endl;
    s << "    float e1 = metricCameraDistance(ws1.xyz, in_lodFactor);" << endl;
    s << "    float e2 = metricCameraDistance(ws2.xyz, in_lodFactor);" << endl;
    break;
  }
  s << "    gl_TessLevelInner[0] = (e0 + e1 + e2);" << endl;
  s << "    gl_TessLevelOuter = float[4]( e0, e1, e2, 1 );" << endl;
  s << "  }" << endl;
  s << "}" << endl;
  return s.str();
}

