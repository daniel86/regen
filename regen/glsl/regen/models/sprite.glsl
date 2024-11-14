
-- emitSprite
#ifndef REGEN_emitSprite_defined_
#define2 REGEN_emitSprite_defined_

#include regen.states.camera.transformEyeToWorld
#include regen.states.camera.transformEyeToScreen

void emitSprite(vec3 quadPos[4], int layer)
{
    vec4 posEye;
    out_texco0 = vec2(1.0,0.0);
    posEye = vec4(quadPos[0],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    out_texco0 = vec2(1.0,1.0);
    posEye = vec4(quadPos[1],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    out_texco0 = vec2(0.0,0.0);
    posEye = vec4(quadPos[2],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    out_texco0 = vec2(0.0,1.0);
    posEye = vec4(quadPos[3],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    EndPrimitive();
}
#endif

-- vs
#include regen.models.mesh.defines
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

in vec3 in_pos;
out vec3 out_pos;

void main() {
#ifdef HAS_modelMatrix
    vec4 posWorld = in_modelMatrix * vec4(in_pos,1.0);
#else
    vec4 posWorld = vec4(in_pos,1.0);
#endif
    out_pos = posWorld.xyz;
    gl_Position = posWorld;
}

-- gs
layout(points) in;
layout(triangle_strip, max_vertices=4) out;

in vec3 in_pos[1];
out vec3 out_posEye;
out vec3 out_posWorld;
out vec2 out_texco0;

#include regen.states.camera.input
uniform vec2 in_viewport;
const vec2 in_spriteSize = vec2(4.0, 4.0);

#include regen.states.camera.transformWorldToEye
#include regen.math.computeSpritePoints
#include regen.models.sprite.emitSprite

void main() {
    vec4 centerEye = transformWorldToEye(vec4(in_pos[0],1.0),0);
    vec3 quadPos[4] = computeSpritePoints(centerEye.xyz, in_spriteSize, vec3(0.0, 1.0, 0.0));
    emitSprite(quadPos,0);
}

-- fs
#include regen.models.mesh.fs

----------------
------ fire sprite
----------------

-- fireTransfer1
#ifndef REGEN_fireTransfer1_
#define2 REGEN_fireTransfer1_
const vec3 in_fireScrollSpeeds = vec3(1.3, 2.1, 2.3);
const vec3 in_fireScales = vec3(1.0, 2.0, 3.0);
const float in_fireDistortionScale = 0.8;
const float in_fireDistortionBias = 0.5;
const vec2 in_fireDistortion1 = vec2(0.1, 0.2);
const vec2 in_fireDistortion2 = vec2(0.1, 0.3);
const vec2 in_fireDistortion3 = vec2(0.1, 0.1);

uniform sampler2D in_fireNoiseTexture;

void fireTransfer1(inout vec2 texco)
{
    // Sample the noise using coordinatesthat change over time.
    vec2 noise1 = texture(in_fireNoiseTexture, (texco * in_fireScales.x) -
        vec2(0.0, in_time * in_fireScrollSpeeds.x)).rr;
    vec2 noise2 = texture(in_fireNoiseTexture, (texco * in_fireScales.y) -
        vec2(0.0, in_time * in_fireScrollSpeeds.y)).rr;
    vec2 noise3 = texture(in_fireNoiseTexture, (texco * in_fireScales.z) -
        vec2(0.0, in_time * in_fireScrollSpeeds.z)).rr;
    // Move to range [-1, +1], and distort the noise.
    noise1 = (noise1 - 0.5f) * 2.0f * in_fireDistortion1;
    noise2 = (noise2 - 0.5f) * 2.0f * in_fireDistortion2;
    noise3 = (noise3 - 0.5f) * 2.0f * in_fireDistortion3;
    // Combine all three distorted noise results into a single noise result.
    vec2 finalNoise = noise1 + noise2 + noise3;
    // Perturb the input texture Y coordinates by the distortion scale and bias values.
    // The perturbation gets stronger as you move up the texture which creates the flame flickering at the top effect.
    finalNoise *= ((texco.y) * in_fireDistortionScale) + in_fireDistortionBias;

    // modify the texture coordinates by the final noise
    texco = vec2(finalNoise.x + texco.x, finalNoise.y + (1.0f - texco.y));
}
#endif
