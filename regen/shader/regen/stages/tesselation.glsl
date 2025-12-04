
-- interpolate
#if TESS_PRIMITVE==quads
#define INTERPOLATE_VALUE(V) mix(mix(V[1], V[0], gl_TessCoord.x), mix(V[2], V[3], gl_TessCoord.x), gl_TessCoord.y)
#define INTERPOLATE_STRUCT(S,V) mix(mix(S[1].V, S[0].V, gl_TessCoord.x), mix(S[2].V, S[3].V, gl_TessCoord.x), gl_TessCoord.y)
#else
#define INTERPOLATE_VALUE(V) (gl_TessCoord.x*V[0] + gl_TessCoord.y*V[1] + gl_TessCoord.z*V[2])
#define INTERPOLATE_STRUCT(S,V) (gl_TessCoord.x*S[0].V + gl_TessCoord.y*S[1].V + gl_TessCoord.z*S[2].V)
#endif

-- metricSreenDistance
#ifndef REGEN_metricSreenDistance_defined_
#define2 REGEN_metricSreenDistance_defined_
const float in_lodMinScreenDistance = 0.03;
const float in_lodMaxScreenDistance = 0.1;
float metricSreenDistance(vec2 v0, vec2 v1) {
    float max = in_viewport.x*in_lodMaxScreenDistance;
    float min = in_viewport.x*in_lodMinScreenDistance;
    // get the distance clamped to the min and max distance range
    float d = clamp(distance(v0,v1), in_lodMinScreenDistance, max);
    // normalize the distance to the range [0,1]
    d = (d - min)/(max-in_lodMinScreenDistance);
    return 1.0 - clamp(d, 0.0, 1.0);
}
#endif

-- metricDeviceDistance
#ifndef REGEN_metricDeviceDistance_defined_
#define2 REGEN_metricDeviceDistance_defined_
const float in_lodMinDeviceDistance = 0.025;
const float in_lodMaxDeviceDistance = 0.05;
float metricDeviceDistance(vec2 v0, vec2 v1) {
    // get the distance clamped to the min and max distance range
    float d = clamp(distance(v0,v1), in_lodMinDeviceDistance, in_lodMaxDeviceDistance);
    // normalize the distance to the range [0,1]
    return 1.0 - (d - in_lodMinDeviceDistance)/(in_lodMaxDeviceDistance-in_lodMinDeviceDistance);
}
#endif

-- metricCameraDistance
#ifndef REGEN_metricCameraDistance_defined_
#define2 REGEN_metricCameraDistance_defined_
const float in_lodMaxCameraDistance = 300.0;
float metricCameraDistance(vec3 v) {
    return clamp(distance(v,in_cameraPosition.xyz)/in_lodMaxCameraDistance, 0.0, 1.0);
}
#endif

-- tesselationControl
#include regen.states.camera.transformWorldToScreen
uniform float in_lodFactor;

// convert a world space vector to device space
vec4 worldToDeviceSpace(vec4 vertexWS) {
// TODO: not accurate for cubes! need another metric for cube render targets.
    vec4 vertexNDS = transformWorldToScreen(vertexWS,0);
    vertexNDS /= vertexNDS.w;
    return vertexNDS;
}
// convert a device space vector to screen space
vec2 deviceToScreenSpace(vec2 vertexDS, vec2 screen) {
    return (vertexDS*0.5 + vec2(0.5))*screen;
}

// test a vertex in device normal space against the view frustum
void offscreenNDC(vec4 v, inout ivec3 offscreen) {
    offscreen.x += int(v.x > 1.0) - int(v.x < -1.0);
    offscreen.y += int(v.y > 1.0) - int(v.y < -1.0);
    offscreen.z += int(v.z > 1.0) - int(v.z < -1.0);
}

#if TESS_LOD == EDGE_SCREEN_DISTANCE
    #include regen.stages.tesselation.metricSreenDistance
#endif
#if TESS_LOD == EDGE_DEVICE_DISTANCE
    #include regen.stages.tesselation.metricDeviceDistance
#endif
#if TESS_LOD == CAMERA_DISTANCE
    #include regen.stages.tesselation.metricCameraDistance
#endif
#ifdef HAS_HEIGHT_MAP
    #include regen.states.textures.input
    #include regen.states.textures.applyHeightMaps
#endif

const float in_minTessLevel = 1.0;
const float in_maxTessLevel = 64.0;

float tessLevel(float d0, float d1, float maxTessLevel) {
    return mix( maxTessLevel, in_minTessLevel, min(d0, d1) );
}
float tessLevel(float d0, float maxTessLevel) {
    return mix( maxTessLevel, in_minTessLevel, d0 );
}

void tesselationControl() {
    if(gl_InvocationID != 0) return;
#for INDEX to TESS_NUM_VERTICES
    vec4 ws${INDEX} = gl_in[${INDEX}].gl_Position;
#endfor
#for INDEX to TESS_NUM_VERTICES
    vec4 nds${INDEX} = worldToDeviceSpace(ws${INDEX});
#endfor
#ifndef NO_TESS_CULL
    ivec3 offscreen = ivec3(0);
#ifdef HAS_HEIGHT_MAP
    #for INDEX to TESS_NUM_VERTICES
    vec4 ws_h${INDEX} = gl_in[${INDEX}].gl_Position;
    applyHeightMaps(ws_h${INDEX}.xyz, in_norWorld[${INDEX}], ${INDEX});
    #endfor
    #for INDEX to TESS_NUM_VERTICES
    vec4 nds_h${INDEX} = worldToDeviceSpace(ws_h${INDEX});
    #endfor
    #for INDEX to TESS_NUM_VERTICES
    offscreenNDC(nds_h${INDEX}, offscreen);
    #endfor
#else // no height map
    #for INDEX to TESS_NUM_VERTICES
    offscreenNDC(nds${INDEX}, offscreen);
    #endfor
#endif
    offscreen = abs(offscreen);
#endif // NO_TESS_CULL

#ifndef NO_TESS_CULL
    // we require here that all vertices are offscreen at the same side.
    if(max(offscreen.x, max(offscreen.y, offscreen.z)) == 3) {
    // if the patch is not visible on screen do not tesselate
#if TESS_PRIMITVE==quads
        gl_TessLevelInner[0] = gl_TessLevelInner[1] = 0;
#else
        gl_TessLevelInner[0] = 0;
#endif // TESS_PRIMITVE==quads
        gl_TessLevelOuter = float[4]( 0, 0, 0, 0 );
    }
    else {
#endif // NO_TESS_CULL
#if TESS_LOD == EDGE_SCREEN_DISTANCE
    #for INDEX to TESS_NUM_VERTICES
        vec2 ss${INDEX} = deviceToScreenSpace(nds${INDEX}.xy, in_viewport);
    #endfor
    #if TESS_PRIMITVE==quads
        float e0 = metricSreenDistance(ss1.xy, ss2.xy);
        float e1 = metricSreenDistance(ss2.xy, ss3.xy);
        float e2 = metricSreenDistance(ss3.xy, ss0.xy);
        float e3 = metricSreenDistance(ss0.xy, ss1.xy);
    #else
        float e0 = metricSreenDistance(ss1.xy, ss2.xy);
        float e1 = metricSreenDistance(ss2.xy, ss0.xy);
        float e2 = metricSreenDistance(ss0.xy, ss1.xy);
    #endif

#elif TESS_LOD == EDGE_DEVICE_DISTANCE
    #if TESS_PRIMITVE==quads
        float e0 = metricDeviceDistance(nds1.xy, nds2.xy);
        float e1 = metricDeviceDistance(nds2.xy, nds3.xy);
        float e2 = metricDeviceDistance(nds3.xy, nds0.xy);
        float e3 = metricDeviceDistance(nds0.xy, nds1.xy);
    #else
        float e0 = metricDeviceDistance(nds1.xy, nds2.xy);
        float e1 = metricDeviceDistance(nds2.xy, nds0.xy);
        float e2 = metricDeviceDistance(nds0.xy, nds1.xy);
    #endif

#else
    #for INDEX to TESS_NUM_VERTICES
        float e${INDEX} = metricCameraDistance(ws${INDEX}.xyz);
    #endfor
#endif
#ifdef TESS_POWER_OF_TWO
    // round each level to the nearest power of two
    #for INDEX to TESS_NUM_VERTICES
        e${INDEX} = pow(2.0, floor(log2(e${INDEX})));
    #endfor
#endif

        float maxTessLevel = clamp(in_maxTessLevel*in_lodFactor, 1.0, 64.0);
#for INDEX to TESS_NUM_VERTICES
        e${INDEX} = tessLevel(e${INDEX}, maxTessLevel);
#endfor
#if TESS_PRIMITVE==quads
        gl_TessLevelInner[0] = max(e1, e3);
        gl_TessLevelInner[1] = max(e0, e2);
        gl_TessLevelOuter = float[4]( e0, e1, e2, e3 );
#else
        gl_TessLevelInner[0] = max(e0, max(e1, e2));
        gl_TessLevelOuter = float[4]( e0, e1, e2, 1 );
#endif
#ifndef NO_TESS_CULL
    }
#endif
}

-- tcs
#ifdef HAS_tessellation_shader
#ifdef TESS_IS_ADAPTIVE
#include regen.models.mesh.defines

layout(vertices=TESS_NUM_VERTICES) out;

#define ID gl_InvocationID

uniform vec2 in_viewport;
#include regen.states.camera.input
#include regen.stages.tesselation.tesselationControl

#define HANDLE_IO(i)

void main() {
    tesselationControl();
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    HANDLE_IO(gl_InvocationID);
}
#endif
#endif

