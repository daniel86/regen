
-- vs
#include regen.models.mesh.vs
-- gs
#ifdef LIGHTNING_USE_LINES
    #include regen.models.mesh.gs
#else // LIGHTNING_USE_LINES
#include regen.states.camera.defines
#include regen.defines.all
#if RENDER_LAYER > 1
    // toggle on GS for layered rendering if requested
    // (better use indirect rendering with gl_DrawID!)
    #ifdef USE_GS_LAYERED_RENDERING
        #ifndef USE_GEOMETRY_SHADER
#define USE_GEOMETRY_SHADER
        #endif
    #endif
    // toggle on GS for writing to gl_Layer
    #ifndef ARB_shader_viewport_layer_array
        #ifndef USE_GEOMETRY_SHADER
#define USE_GEOMETRY_SHADER
        #endif
    #endif
#endif

layout(lines) in;
#ifdef USE_GS_LAYERED_RENDERING
#define2 GS_MAX_VERTICES ${${RENDER_LAYER}*12}
layout(triangle_strip, max_vertices=${GS_MAX_VERTICES}) out;
#else
layout(triangle_strip, max_vertices=12) out;
#endif

in vec3 in_posWorld[2];
in float in_brightness[2];
flat in uint in_strikeIdx[2];
out vec3 out_posEye;
out vec3 out_posWorld;
out vec2 out_texco0;
out float out_brightness;
flat out float out_matAlpha;
#if RENDER_LAYER > 1 && USE_GS_LAYERED_RENDERING
flat out int out_layer;
#endif

const float in_lightningWidth = 0.1;

#include regen.math.computeSpritePoints
#include regen.models.sprite.emitQuad_eye
#include regen.states.camera.transformWorldToEye

#define HANDLE_IO(i)

void emit(int layer) {
    uint strike = in_strikeIdx[0];
    vec3 e0 = transformWorldToEye(vec4(in_posWorld[0], 1.0),layer).xyz;
    vec3 e1 = transformWorldToEye(vec4(in_posWorld[1], 1.0),layer).xyz;
    float billboardHeight = distance(e0, e1);
    float width = in_lightningWidth * in_strikeWidth[strike];
    width *= in_brightness[0];
    //width *= max(0.25, min(1, in_brightness[0] + 0.25));
    //billboardHeight += in_lightningWidth * 0.5;
    vec3 quadPos[4] = computeSpritePoints(
            (e0 + e1) * 0.5,
            vec2(width, billboardHeight),
            normalize(e0 - e1));
#if RENDER_LAYER > 1
    gl_Layer = layer;
    #ifdef USE_GS_LAYERED_RENDERING
    out_layer = layer;
    #endif
#endif
    out_matAlpha = in_strikeAlpha[strike];
    emitQuad_eye(quadPos, layer);
}

void main() {
#ifdef USE_GS_LAYERED_RENDERING
    #for LAYER to ${RENDER_LAYER}
    #ifndef SKIP_LAYER${LAYER}
    emit(${LAYER});
    #endif // SKIP_LAYER
    #endfor
#else
    #if RENDER_LAYER > 1
    emit(in_layer[0]);
    #else
    emit(0);
    #endif
#endif
}
#endif

-- fs
#define HAS_MATERIAL
#define HAS_matAlpha
flat in float in_matAlpha;
#include regen.models.mesh.fs
