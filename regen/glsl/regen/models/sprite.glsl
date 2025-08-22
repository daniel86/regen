
-- getForceMatrix
#ifndef REGEN_getForceMatrix_defined_
#define2 REGEN_getForceMatrix_defined_
const float in_stiffness = 1.0;
mat3 getForceMatrix(vec2 force) {
    vec2 xz = force;
    float alpha = length(xz);
    // compute force axis
    xz /= max(alpha, 0.001);
    // bend up to 90 degrees
    alpha = min(1.0, alpha) * 1.5707963267948966 * in_stiffness;
    // Calculate the rotation matrix
    float ca = cos(alpha);
    vec2 xz_ka = xz * (1.0 - ca);
    vec2 xz_sa = xz * sin(alpha);
    float xzka = xz.y * xz_ka.x;
    return mat3(
        // first row
        ca + xz.x * xz_ka.x, -xz_sa.y, xzka,
        // second row
        xz_sa.y, ca, -xz_sa.x,
        // third row
        xzka, xz_sa.x, ca + xz.y * xz_ka.y
    );
}
#endif

-- applyForce
#ifndef REGEN_applyForce_defined_
#define2 REGEN_applyForce_defined_
#include regen.models.sprite.getForceMatrix
void applyForce(inout vec3 quadPos[4], vec2 force) {
    // Apply the rotation to the top points using the bottom points as the pivot
    mat3 rotationMatrix = getForceMatrix(force);
    quadPos[1] = quadPos[0] + rotationMatrix * (quadPos[1] - quadPos[0]);
    quadPos[3] = quadPos[2] + rotationMatrix * (quadPos[3] - quadPos[2]);
}
#endif

-- applyForceBase
#ifndef REGEN_applyForceBase_defined_
#define2 REGEN_applyForceBase_defined_
#include regen.models.sprite.getForceMatrix
void applyForceBase(inout vec3 quadPos, vec3 basePos, vec2 force) {
    // Apply the rotation to the top points using the base point as the pivot
    mat3 rotationMatrix = getForceMatrix(force);
    quadPos = basePos + rotationMatrix * (quadPos - basePos);
}
#endif

-- emitQuad_eye
#ifndef REGEN_emitQuad_eye_defined_
#define2 REGEN_emitQuad_eye_defined_

#include regen.states.camera.transformEyeToWorld
#include regen.states.camera.transformEyeToScreen

void emitQuad_eye(vec3 quadPos[4], int layer)
{
#ifdef HAS_QUAD_NORMAL
    vec3 norEye = normalize(cross(quadPos[1]-quadPos[0],quadPos[2]-quadPos[0]));
    out_norWorld = normalize(transformEyeToWorld(vec4(norEye,0.0),layer).xyz);
#endif
#ifdef HAS_UPWARDS_NORMAL
    out_norWorld = normalize(transformEyeToWorld(vec4(quadPos[1] - quadPos[0], 0.0),layer).xyz);
#endif
#ifdef HAS_brightness
    out_brightness = in_brightness[0];
#endif

    vec4 posEye;
    out_texco0 = vec2(1.0,1.0);
    posEye = vec4(quadPos[0],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    out_texco0 = vec2(1.0,0.0);
    posEye = vec4(quadPos[1],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

#ifdef HAS_UPWARDS_NORMAL
    out_norWorld = normalize(transformEyeToWorld(vec4(quadPos[3] - quadPos[2], 0.0),layer).xyz);
#endif
    out_texco0 = vec2(0.0,1.0);
    posEye = vec4(quadPos[2],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    out_texco0 = vec2(0.0,0.0);
    posEye = vec4(quadPos[3],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    EndPrimitive();
}
#endif

-- emitQuad_world
#ifndef REGEN_emitQuad_world_defined_
#define2 REGEN_emitQuad_world_defined_

#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#ifdef HAS_UV_FADED_COLOR
const float in_uvDarken = 0.5;
#endif

void emitQuad_world(vec3 quadPos[4], vec4 color, int layer)
{
#ifdef HAS_QUAD_NORMAL
    out_norWorld = normalize(cross(quadPos[1]-quadPos[0],quadPos[2]-quadPos[0]));
#endif
#ifdef HAS_UPWARDS_NORMAL
    out_norWorld = normalize(quadPos[1] - quadPos[0]);
#endif
#ifdef HAS_brightness
    out_brightness = in_brightness[0];
#endif
    //color.rgb *= in_uvDarken;

    vec4 posEye;
    out_texco0 = vec2(1.0, 1.0);
    out_posWorld = quadPos[0];
#ifdef HAS_UV_FADED_COLOR
    out_col = color * in_uvDarken;
#endif
    posEye = transformWorldToEye(vec4(quadPos[0],1.0), 0);
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    out_texco0 = vec2(1.0, 0.0);
    out_posWorld = quadPos[1];
#ifdef HAS_UV_FADED_COLOR
    out_col = color;
#endif
    posEye = transformWorldToEye(vec4(quadPos[1],1.0), 0);
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

#ifdef HAS_UPWARDS_NORMAL
    out_norWorld = normalize(quadPos[3] - quadPos[2]);
#endif
    out_texco0 = vec2(0.0, 1.0);
    out_posWorld = quadPos[2];
#ifdef HAS_UV_FADED_COLOR
    out_col = color * in_uvDarken;
#endif
    posEye = transformWorldToEye(vec4(quadPos[2],1.0), 0);
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    out_texco0 = vec2(0.0, 0.0);
    out_posWorld = quadPos[3];
#ifdef HAS_UV_FADED_COLOR
    out_col = color;
#endif
    posEye = transformWorldToEye(vec4(quadPos[3],1.0), 0);
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    EmitVertex();

    EndPrimitive();
}
#endif

-- emitBillboard
#ifndef REGEN_emitBillboard_defined_
#define2 REGEN_emitBillboard_defined_

#include regen.math.computeSpritePoints
#include regen.models.sprite.emitQuad_eye
#include regen.states.camera.transformWorldToEye

void emitBillboard(vec3 center, vec2 size
#ifdef USE_FORCE
        , vec2 force
#endif
        ) {
    vec4 centerEye = transformWorldToEye(vec4(center, 1.0),0);
    vec3 quadPos[4] = computeSpritePoints(centerEye.xyz, size, vec3(0.0, 1.0, 0.0));
#ifdef USE_FORCE
    applyForce(quadPos, force);
#endif
    emitQuad_eye(quadPos,0);
}
#endif

-- emitSpriteCross
#ifndef REGEN_emitSpriteCross_defined_
#define2 REGEN_emitSpriteCross_defined_

#include regen.math.rotateXZ
#include regen.math.computeSpritePoints
#include regen.models.sprite.emitQuad_world
#ifdef USE_FORCE
    #include regen.models.sprite.applyForce
#endif

void emitSpriteCross(vec3 center
        , vec2 size
        , vec4 color
        , float orientation
#ifdef USE_FORCE
        , vec2 force
#endif
        ) {
    const vec3 up = vec3(0.0, 1.0, 0.0);
    const vec3 front = vec3(0.0, 0.0, 1.0);
    vec3 quadPos[4];
    // first quad
    vec3 dir = rotateXZ(front, orientation);
    computeSpritePoints(center, size, dir, up, quadPos);
#ifdef USE_FORCE
    applyForce(quadPos, force);
#endif
    emitQuad_world(quadPos,color,0);
    // second quad
    dir = rotateXZ(front, orientation - 2.356194);
    computeSpritePoints(center, size, dir, up, quadPos);
#ifdef USE_FORCE
    applyForce(quadPos, force);
#endif
    emitQuad_world(quadPos,color,0);
    // third quad
    dir = rotateXZ(front, orientation - 3.92699);
    computeSpritePoints(center, size, dir, up, quadPos);
#ifdef USE_FORCE
    applyForce(quadPos, force);
#endif
    emitQuad_world(quadPos,color,0);
}
#endif

-- vs
#include regen.models.mesh.defines
#include regen.models.tf.transformModel

in vec3 in_pos;
out vec3 out_pos;

void main() {
    vec4 posWorld = transformModel(vec4(in_pos,1.0));
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

#include regen.models.sprite.emitBillboard

void main() {
    emitBillboard(in_pos[0], vec4(1.0), in_spriteSize);
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
    texco.y = 1.0 - texco.y;
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
