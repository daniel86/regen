
-- interpolate
#if TESS_PRIMITVE==quads
#define INTERPOLATE_VALUE(V) mix(mix(V[1], V[0], gl_TessCoord.x), mix(V[2], V[3], gl_TessCoord.x), gl_TessCoord.y)
#define INTERPOLATE_STRUCT(S,V) mix(mix(S[1].V, S[0].V, gl_TessCoord.x), mix(S[2].V, S[3].V, gl_TessCoord.x), gl_TessCoord.y)
#else
#define INTERPOLATE_VALUE(V) (gl_TessCoord.z*V[0] + gl_TessCoord.x*V[1] + gl_TessCoord.y*V[2])
#define INTERPOLATE_STRUCT(S,V) (gl_TessCoord.z*S[0].V + gl_TessCoord.x*S[1].V + gl_TessCoord.y*S[2].V)
#endif

-- metricSreenDistance
#ifndef REGEN_metricSreenDistance_defined_
#define2 REGEN_metricSreenDistance_defined_
const float in_lodMinSD = 7.0;
const float in_lodMaxSD = 0.2;
float metricSreenDistance(vec2 v0, vec2 v1, float factor)
{
     float max = in_viewport.x*in_lodMaxSD;
     float d = (clamp(distance(v0,v1), in_lodMinSD, max) - min)/(max-in_lodMinSD);
     return clamp( d*64.0*factor, 1, 64);
}
#endif

-- metricDeviceDistance
#ifndef REGEN_metricDeviceDistance_defined_
#define2 REGEN_metricDeviceDistance_defined_
const float in_lodMinDD = 0.025;
const float in_lodMaxDD = 0.05;
float metricDeviceDistance(vec2 v0, vec2 v1, float factor)
{
     float d = (clamp(distance(v0,v1), in_lodMinDD, in_lodMaxDD) - in_lodMinDD)/(in_lodMaxDD-in_lodMinDD);
     return clamp( d*64.0*factor, 1, 64);
}
#endif

-- metricCameraDistance
#ifndef REGEN_metricCameraDistance_defined_
#define2 REGEN_metricCameraDistance_defined_
const float in_lodMinCD = 0.0;
const float in_lodMaxCD = 300.0;
float metricCameraDistance(vec3 v, float factor)
{
     float d = (in_lodMaxCD - clamp(distance(v,in_cameraPosition), in_lodMinCD, in_lodMaxCD))/(in_lodMaxCD-in_lodMinCD);
     return clamp( d*64.0*factor, 1, 64);
}
#endif

-- tesselationControl
#include regen.states.camera.transformWorldToScreen 
uniform float in_lodFactor;

// convert a world space vector to device space
vec4 worldToDeviceSpace(vec4 vertexWS)
{
// TODO: not accurate for cubes! need another metric for cube render targets.
    vec4 vertexNDS = transformWorldToScreen(vertexWS,0);
    vertexNDS /= vertexNDS.w;
    return vertexNDS;
}
// convert a device space vector to screen space
vec2 deviceToScreenSpace(vec4 vertexDS, vec2 screen)
{
    return (vertexDS.xy*0.5 + vec2(0.5))*screen;
}

// test a vertex in device normal space against the view frustum
void offscreenNDC(vec4 v, inout ivec3 offscreen)
{
    offscreen.x += int(v.x > 1.0) - int(v.x < -1.0);
    offscreen.y += int(v.y > 1.0) - int(v.y < -1.0);
    offscreen.z += int(v.z > 1.0) - int(v.z < -1.0);
}

#if TESS_LOD == EDGE_SCREEN_DISTANCE
    #include regen.states.tesselation.metricSreenDistance
#endif
#if TESS_LOD == EDGE_DEVICE_DISTANCE
    #include regen.states.tesselation.metricDeviceDistance
#endif
#if TESS_LOD == CAMERA_DISTANCE_INVERSE
    #include regen.states.tesselation.metricCameraDistance
#endif

void tesselationControl()
{
    if(gl_InvocationID != 0) return;
#for INDEX to TESS_NUM_VERTICES
    vec4 ws${INDEX} = gl_in[${INDEX}].gl_Position;
#endfor
#for INDEX to TESS_NUM_VERTICES
    vec4 nds${INDEX} = worldToDeviceSpace(ws${INDEX});
#endfor
    ivec3 offscreen = ivec3(0);
#for INDEX to TESS_NUM_VERTICES
    offscreenNDC(nds${INDEX}, offscreen);
#endfor
    offscreen = abs(offscreen);

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
#if TESS_LOD == EDGE_SCREEN_DISTANCE
    #for INDEX to TESS_NUM_VERTICES
        float ss${INDEX} = deviceToScreenSpace(nds${INDEX}.xy, in_viewport);
    #endfor
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
    #if TESS_PRIMITVE==quads
        float e2 = metricDeviceDistance(nds3.xy, nds0.xy, in_lodFactor);
        float e3 = metricDeviceDistance(nds2.xy, nds3.xy, in_lodFactor);
    #else
        float e2 = metricDeviceDistance(nds2.xy, nds0.xy, in_lodFactor);
    #endif

#else
    #for INDEX to TESS_NUM_VERTICES
        float e${INDEX} = metricCameraDistance(ws${INDEX}.xyz, in_lodFactor);
    #endfor
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

-- tcs
#ifdef HAS_tessellation_shader
#ifdef TESS_IS_ADAPTIVE
#include regen.models.mesh.defines

layout(vertices=TESS_NUM_VERTICES) out;

#define ID gl_InvocationID

uniform vec2 in_viewport;
#include regen.states.camera.input
#include regen.states.tesselation.tesselationControl

#define HANDLE_IO(i)

void main() {
    tesselationControl();
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    HANDLE_IO(gl_InvocationID);
}
#endif
#endif

