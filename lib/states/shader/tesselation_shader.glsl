
-- interpolate
#if TESS_PRIMITVE==quads
#define INTERPOLATE_VALUE(V) mix(mix(V[1], V[0], gl_TessCoord.x), mix(V[2], V[3], gl_TessCoord.x), gl_TessCoord.y)
#define INTERPOLATE_STRUCT(S,V) mix(mix(S[1].V, S[0].V, gl_TessCoord.x), mix(S[2].V, S[3].V, gl_TessCoord.x), gl_TessCoord.y)
#else
#define INTERPOLATE_VALUE(V) (gl_TessCoord.z*V[0] + gl_TessCoord.x*V[1] + gl_TessCoord.y*V[2])
#define INTERPOLATE_STRUCT(S,V) (gl_TessCoord.z*S[0].V + gl_TessCoord.x*S[1].V + gl_TessCoord.y*S[2].V)
#endif

-- tc
uniform float in_lodFactor;

// When you transform a vertex by the projection matrix, you get clip coordinate.
// After w division this is called normalized device coordinates.
//    if z is from -1.0 to 1.0, then it is inside the znear and zfar clipping planes.
// A helper function to project a world space vertex to device normal space
vec4 worldToDeviceSpace(vec4 vertexWS){
    vec4 vertexNDS = in_viewProjectionMatrix * vertexWS;
    vertexNDS /= vertexNDS.w;
    return vertexNDS;
}

// test a vertex in device normal space against the view frustum
bool isOffscreenNDC(vec4 v){
    return v.x<-1.0 || v.y<-1.0 || v.z<-1.0 || v.x>1.0 || v.y>1.0 || v.z>1.0;
}

#if TESS_LOD == EDGE_SCREEN_DISTANCE
// This helper function converts a device normal space vector to screen space
vec2 deviceToScreenSpace(vec4 vertexDS, vec2 screen){
    return (vertexDS.xy*0.5 + vec2(0.5))*screen;
}

float metricSreenDistance(vec2 v0, vec2 v1, float factor){
     const float min = 7.0;
     float max = in_viewport.x*0.2;
     float d = (clamp(distance(v0,v1), min, max) - min)/(max-min);
     return clamp( d*64.0*factor, 1, 64);
}
#endif
#if TESS_LOD == EDGE_DEVICE_DISTANCE
float metricDeviceDistance(vec2 v0, vec2 v1, float factor){
     const float min = 0.025;
     const float max = 0.2;
     float d = (clamp(distance(v0,v1), min, max) - min)/(max-min);
     return clamp( d*64.0*factor, 1, 64);
}
#endif
#if TESS_LOD == CAMERA_DISTANCE_INVERSE
float metricCameraDistance(vec3 v, float factor){
     const float min = 0.0;
     const float max = 50.0;
     float d = (max - clamp(distance(v,in_cameraPosition), min, max))/(max-min);
     return clamp( d*64.0*factor, 1, 64);
}
#endif

void tesselationControl(){
  if(gl_InvocationID != 0) return;
  vec4 ws0 = gl_in[0].gl_Position;
  vec4 ws1 = gl_in[1].gl_Position;
  vec4 ws2 = gl_in[2].gl_Position;
#if TESS_PRIMITVE==quads
  vec4 ws3 = gl_in[3].gl_Position;
#endif
  vec4 nds0 = worldToDeviceSpace(ws0);
  vec4 nds1 = worldToDeviceSpace(ws1);
  vec4 nds2 = worldToDeviceSpace(ws2);
#if TESS_PRIMITVE==quads
  vec4 nds3 = worldToDeviceSpace(ws3);
#endif
  if(isOffscreenNDC(nds0)
        && isOffscreenNDC(nds1)
        && isOffscreenNDC(nds2)
#if TESS_PRIMITVE==quads
        && isOffscreenNDC(nds3)
#endif
  ){
    // if the patch is not visible on screen do not tesselate
#if TESS_PRIMITVE==quads
    gl_TessLevelInner[0] = gl_TessLevelInner[1] = 0;
#else
    gl_TessLevelInner[0] = 0;
#endif
    gl_TessLevelOuter = float[4]( 0, 0, 0, 0 );
  } else{
#if TESS_LOD == EDGE_SCREEN_DISTANCE
    vec2 ss0 = deviceToScreenSpace(nds0, in_viewport);
    vec2 ss1 = deviceToScreenSpace(nds1, in_viewport);
    vec2 ss2 = deviceToScreenSpace(nds2, in_viewport);
  #if TESS_PRIMITVE==quads
    vec2 ss3 = deviceToScreenSpace(nds3, in_viewport);
  #endif
    float e0 = metricSreenDistance(ss1.xy, ss2.xy, in_lodFactor);
    float e1 = metricSreenDistance(ss0.xy, ss1.xy, in_lodFactor);
  #if TESS_PRIMITVE==quads
    float e2 = metricSreenDistance(ss3.xy, ss0.xy, in_lodFactor);
    float e3 = metricSreenDistance(ss2.xy, ss3.xy, in_lodFactor);
  #else
    float e2 = metricSreenDistance(ss2.xy, ss0.xy, in_lodFactor);
  #endif

#elif TESS_LOD == EDGE_DEVICE_DISTANCE
    float e0 = metricDeviceDistance(nds1.xy, nds2.xy, in_lodFactor);
    float e1 = metricDeviceDistance(nds0.xy, nds1.xy, in_lodFactor);
    float e2 = metricDeviceDistance(nds2.xy, nds0.xy, in_lodFactor);
  #if TESS_PRIMITVE==quads
    float e2 = metricDeviceDistance(nds3.xy, nds0.xy, in_lodFactor);
    float e3 = metricDeviceDistance(nds2.xy, nds3.xy, in_lodFactor);
  #endif

#else
    float e0 = metricCameraDistance(ws0.xyz, in_lodFactor);
    float e1 = metricCameraDistance(ws1.xyz, in_lodFactor);
    float e2 = metricCameraDistance(ws2.xyz, in_lodFactor);
  #if TESS_PRIMITVE==quads
    float e3 = metricCameraDistance(ws3.xyz, in_lodFactor);
  #endif
#endif
#if TESS_PRIMITVE==quads
    gl_TessLevelInner[0] = mix(e1, e2, 0.5);
    gl_TessLevelInner[1] = mix(e0, e3, 0.5);
    gl_TessLevelOuter = float[4]( e0, e1, e2, e3 );
#else
    gl_TessLevelInner[0] = (e0 + e1 + e2);
    gl_TessLevelOuter = float[4]( e0, e1, e2, 1 );
#endif
  }
}


