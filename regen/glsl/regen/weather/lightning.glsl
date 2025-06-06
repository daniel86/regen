
-- vs
#include regen.models.mesh.vs
-- gs
#ifdef LIGHTNING_USE_LINES
    #include regen.models.mesh.gs
#else // LIGHTNING_USE_LINES
layout(lines) in;
layout(triangle_strip, max_vertices=12) out;

in vec3 in_posWorld[2];
in float in_brightness[2];
out vec3 out_posEye;
out vec3 out_posWorld;
out vec2 out_texco0;
out float out_brightness;

const float in_lightningWidth = 0.1;

#include regen.math.computeSpritePoints
#include regen.models.sprite.emitQuad_eye
#include regen.states.camera.transformWorldToEye

void main() {
    vec3 e0 = transformWorldToEye(vec4(in_posWorld[0], 1.0),0).xyz;
    vec3 e1 = transformWorldToEye(vec4(in_posWorld[1], 1.0),0).xyz;
    float billboardHeight = distance(e0, e1);
    float width = in_lightningWidth;
    width *= in_brightness[0];
    //width *= max(0.25, min(1, in_brightness[0] + 0.25));
    //billboardHeight += in_lightningWidth * 0.5;
    vec3 quadPos[4] = computeSpritePoints(
            (e0 + e1) * 0.5,
            vec2(width, billboardHeight),
            normalize(e0 - e1));
    emitQuad_eye(quadPos,0);
}
#endif
-- fs
#define HAS_MATERIAL
#include regen.models.mesh.fs
