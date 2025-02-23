
-- windAtPosition
#ifndef REGEN_windAtPosition_Included_
#define REGEN_windAtPosition_Included_
const float in_windFlowScale = 100.0;
const float in_windFlowTime = 0.2;
#ifdef HAS_windNoise
const float in_windNoiseScale = 100.0;
const float in_windNoiseSpeed = 1.0;
const float in_windNoiseStrength = 0.1;
#endif
const vec2 in_wind = vec2(1.0, 0.0);

uniform sampler2D in_windFlow;

vec2 windAtPosition(vec3 posWorld)
{
#ifdef HAS_windFlow
    vec2 windFlow_uv =
        // map position to "wind flow space"
        posWorld.xz/in_windFlowScale +
        // translate the wind flow with the time in the direction of the wind
        normalize(in_wind) * in_time * in_windFlowTime;
    // wrap around the wind flow texture
    windFlow_uv.x = mod(windFlow_uv.x, 1.0);
    windFlow_uv.y = mod(windFlow_uv.y, 1.0);
    // sample the wind flow texture, scale it to -1,1, and scale it by the wind strength
    vec2 windSample = texture(in_windFlow, windFlow_uv).xy;
    vec2 wind = (2.0*windSample - vec2(1.0)) * length(in_wind);
#else
    vec2 wind = in_wind;
    //wind.x -= 0.1*sin(in_time * in_windFlowTime * 10.0 + posWorld.x/in_windFlowScale);
    //wind.y -= 0.1*cos(in_time * in_windFlowTime * 10.0 + posWorld.z/in_windFlowScale);
#endif
#ifdef HAS_windNoise
    float scaledTime = in_time*0.01*in_windNoiseSpeed;
    // add some noise to the wind
    float windNoise_x = texture(in_windNoise,
        (posWorld.xz/in_windNoiseScale + scaledTime)).x;
    float windNoise_y = texture(in_windNoise,
        (posWorld.zx/in_windNoiseScale + scaledTime)).x;
    wind.x += (2.0*windNoise_x - 1.0) * in_windNoiseStrength;
    wind.y += (2.0*windNoise_y - 1.0) * in_windNoiseStrength;
#endif
    return wind;
}
#endif // REGEN_windAtPosition_Included_

-- wavingQuad.gs
#include regen.states.camera.defines
#include regen.defines.all
#define2 GS_MAX_VERTICES ${${RENDER_LAYER}*3}
layout(triangles) in;
layout(triangle_strip, max_vertices=${GS_MAX_VERTICES}) out;

out vec3 out_posWorld;
out vec3 out_posEye;
flat out int out_layer;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#include regen.models.sprite.applyForce
#include regen.states.wind.windAtPosition
#include regen.layered.gs.computeVisibleLayers

#define HANDLE_IO(i)

void emitVertex(vec3 posWorld, int index, int layer) {
    vec4 posEye = transformWorldToEye(vec4(posWorld,1.0),layer);
    out_posWorld = posWorld.xyz;
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    HANDLE_IO(index);
    // TODO: also rotate the normal
    EmitVertex();
}

int addPoint(inout vec3 quadPos[4], int vIndex) {
    int quadIndex;
    if (in_texco0[vIndex].y > 0.5 && in_texco0[vIndex].x < 0.5) {
        quadIndex = 0;
    } else if (in_texco0[vIndex].y > 0.5 && in_texco0[vIndex].x > 0.5) {
        quadIndex = 2;
    } else if (in_texco0[vIndex].x < 0.5) {
        quadIndex = 1;
    } else {
        quadIndex = 3;
    }
    quadPos[quadIndex] = gl_in[vIndex].gl_Position.xyz;
    return quadIndex;
}

void wavingQuad(int layer) {
    // A list of quad points. We assume here that the "bottom" points
    // of the quad are indicated by a uv coordinate of (_,0).
    // Below we need to figure out if v1,..,v3 are the bottom points
    // or at the top, and compute the remaining point.
    vec3 quadPos[4] = vec3[4](vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0));
    int vIndex0 = addPoint(quadPos, 0);
    int vIndex1 = addPoint(quadPos, 1);
    int vIndex2 = addPoint(quadPos, 2);
    int missingIndex = 6 - vIndex0 - vIndex1 - vIndex2;
    if (missingIndex == 0) {
        quadPos[0] = quadPos[1] + quadPos[2] - quadPos[3];
    } else if (missingIndex == 1) {
        quadPos[1] = quadPos[0] + quadPos[3] - quadPos[2];
    } else if (missingIndex == 2) {
        quadPos[2] = quadPos[3] + quadPos[0] - quadPos[1];
    } else {
        quadPos[3] = quadPos[2] + quadPos[1] - quadPos[0];
    }
    vec3 bottomCenter = 0.5*(quadPos[0] + quadPos[2]);
    vec2 wind = windAtPosition(bottomCenter);
    applyForce(quadPos, wind);

    emitVertex(quadPos[vIndex0], 0, layer);
    emitVertex(quadPos[vIndex1], 1, layer);
    emitVertex(quadPos[vIndex2], 2, layer);
    EndPrimitive();
}

void main() {
#ifdef COMPUTE_LAYER_VISIBILITY
    bool visibleLayers[RENDER_LAYER];
    computeVisibleLayers(visibleLayers);
#endif
#for LAYER to ${RENDER_LAYER}
    #ifndef SKIP_LAYER${LAYER}
        #ifdef COMPUTE_LAYER_VISIBILITY
    if (visibleLayers[${LAYER}]) {
        #endif // COMPUTE_LAYER_VISIBILITY
        // select framebuffer layer
        gl_Layer = ${LAYER};
        out_layer = ${LAYER};
        wavingQuad(${LAYER});
        #ifdef COMPUTE_LAYER_VISIBILITY
    }
        #endif // COMPUTE_LAYER_VISIBILITY
    #endif // SKIP_LAYER
#endfor
}
