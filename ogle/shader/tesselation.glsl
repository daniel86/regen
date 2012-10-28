
-- tes
layout(TESS_PRIMITVE, TESS_SPACING, TESS_ORDERING) in;

#if TESS_PRIMITVE==quads
    #define INTERPOLATE_VALUE(Value) mix(\
        mix(Value[1], Value[0], gl_TessCoord.x),\
        mix(Value[2], Value[3], gl_TessCoord.x), gl_TessCoord.y)
    #define INTERPOLATE_STRUCT(Struct,Member) mix(\
        mix(Struct[1].Member, Struct[0].Member, gl_TessCoord.x),\
        mix(Struct[2].Member, Struct[3].Member, gl_TessCoord.x), gl_TessCoord.y)
    #define INTERPOLATE_ARRAY_STRUCT(Struct,Member,i) mix(\
        mix(Struct[1].Member[i], Struct[0].Member[i], gl_TessCoord.x),\
        mix(Struct[2].Member[i], Struct[3].Member[i], gl_TessCoord.x), gl_TessCoord.y)
#else
    #define INTERPOLATE_VALUE(Value) (\
        gl_TessCoord.z*Value[0] +\
        gl_TessCoord.x*Value[1] +\
        gl_TessCoord.y*Value[2])
    #define INTERPOLATE_STRUCT(Struct,Member) (\
        gl_TessCoord.z*Struct[0].Member +\
        gl_TessCoord.x*Struct[1].Member +\
        gl_TessCoord.y*Struct[2].Member)
    #define INTERPOLATE_ARRAY_STRUCT(Struct,Member,i) (\
        gl_TessCoord.z*Struct[0].Member[i] +\
        gl_TessCoord.x*Struct[1].Member[i] +\
        gl_TessCoord.y*Struct[2].Member[i])
#endif

float interpolate(float v[TESS_NUM_VERTICES]) { return INTERPOLATE_VALUE(v); }
vec2 interpolate(vec2 v[TESS_NUM_VERTICES]) { return INTERPOLATE_VALUE(v); }
vec3 interpolate(vec3 v[TESS_NUM_VERTICES]) { return INTERPOLATE_VALUE(v); }
vec4 interpolate(vec4 v[TESS_NUM_VERTICES]) { return INTERPOLATE_VALUE(v); }
int interpolate(int v[TESS_NUM_VERTICES]) { return int(INTERPOLATE_VALUE(v)); }
ivec2 interpolate(ivec2 v[TESS_NUM_VERTICES]) { return ivec2(INTERPOLATE_VALUE(v)); }
ivec3 interpolate(ivec3 v[TESS_NUM_VERTICES]) { return ivec3(INTERPOLATE_VALUE(v)); }
ivec4 interpolate(ivec4 v[TESS_NUM_VERTICES]) { return ivec4(INTERPOLATE_VALUE(v)); }
mat4 interpolate(mat4 v[TESS_NUM_VERTICES]) { return INTERPOLATE_VALUE(v); }
mat3 interpolate(mat3 v[TESS_NUM_VERTICES]) { return INTERPOLATE_VALUE(v); }

#define SAMPLE(TEX,TEXCO) texture(TEX, interpolate(TEXCO))

-- tesselationControl
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

// This helper function converts a device normal space vector to screen space
vec2 deviceToScreenSpace(vec4 vertexDS, vec2 screen){
    return (vertexDS.xy*0.5 + vec2(0.5))*screen;
}

// test a vertex in device normal space against the view frustum
bool isOffscreenNDC(vec4 v){
    return v.x<-1.0 || v.y<-1.0 || v.z<-1.0 || v.x>1.0 || v.y>1.0 || v.z>1.0;
}

float metricSreenDistance(vec2 v0, vec2 v1, float factor){
     const float min = 7.0;
     float max = in_viewport.x*0.2;
     float d = (clamp(distance(v0,v1), min, max) - min)/(max-min);
     return clamp( d*64.0*factor, 1, 64);
}
float metricDeviceDistance(vec2 v0, vec2 v1, float factor){
     const float min = 0.025;
     const float max = 0.2;
     float d = (clamp(distance(v0,v1), min, max) - min)/(max-min);
     return clamp( d*64.0*factor, 1, 64);
}
float metricCameraDistance(vec3 v, float factor){
     const float min = 4.0;
     const float max = 50.0;
     float d = (max - clamp(distance(v,in_cameraPosition), min, max))/(max-min);
     return clamp( d*64.0*factor, 1, 64);
}

void tesselationControl(){
  if(gl_InvocationID != 0) return;
  // quad vertex positions
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
    float e2 = metricDeviceDistance(nds3.xy, nds0.xy, in_lodFactor);
  #if TESS_PRIMITVE==quads
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


