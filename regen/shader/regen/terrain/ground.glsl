
------------
----- Maps a world position to uv coordinate that spans the whole ground.
------------
-- groundUV
#ifndef ground_uv_included
#define2 ground_uv_included
vec2 groundUV(vec3 posWorld) {
    // compute uv given in_mapSize, in_mapCenter and a world position
    return (posWorld.xz - in_mapCenter.xz) / in_mapSize.xz + vec2(0.5);
}
vec2 groundUV(vec3 posWorld, vec3 normal) {
    return groundUV(posWorld);
}
#endif // ground_uv_included

------------
----- Maps a world position to uv coordinate that spans a local area of the ground.
------------
-- materialUV
#ifndef ground_materialUV_included
#define2 ground_materialUV_included
vec2 materialUV(vec3 pos, vec3 normal) {
    // Compute triplanar UV coordinates based on the position and normal
    vec3 absNormal = abs(normal);
    vec2 uv = vec2(0.0);
    if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
        uv = pos.zy * 0.5 + 0.5;
    } else if (absNormal.y > absNormal.x && absNormal.y > absNormal.z) {
        uv = pos.xz * 0.5 + 0.5;
    } else {
        uv = pos.xy * 0.5 + 0.5;
    }
    return uv;
}
vec3 materialUV(vec3 pos, vec3 normal, int materialIndex) {
    // material index is the index into the texture array
    return vec3(materialUV(pos, normal), materialIndex);
}
#endif // ground_materialUV_included

-- blend_ground
#ifndef blend_ground_included
#define2 blend_ground_included
#include regen.states.blending.color-space

#define SLOPE_MIN 0.0
#define SLOPE_MAX 1.2

const float in_snowAmount = 1.0;

void blend_ground(vec3 col1, inout vec3 col2, float factor) {
    // slope gating
    //float slope = dot(in_norWorld, vec3(0.0, 1.0, 0.0));
    float slope = acos(in_norWorld.y); // = acos(dot(normal, up))
    float blendFactor = (1.0 - smoothstep(SLOPE_MIN, SLOPE_MAX, slope)) * factor;
    // altitude factor
    //slopeFactor *= smoothstep(SNOWLINE - SNOW_FADE, SNOWLINE + SNOW_FADE, in_posWorld.y);
    // add simple noise to vary coverage (sample a low-frequency noise texture or value)
    //float noise = texture(in_snowNoise, REGEN_TEXCO${_ID}_Z * 0.01).r; // cheap; or use a 3D noise
    //slopeFactor *= mix(0.8, 1.2, noise);
    blendFactor = clamp(in_snowAmount * blendFactor, 0.0, 1.0);
    // mix based on factor
    col2 = mix(col2, col1, blendFactor);
}
#endif // blend_ground_included

------------
----- Update the material weights of the ground.
----- The weights are rendered into float buffers using the fragment shader.
------------
-- weights.vs
in vec3 in_pos;
out vec2 out_groundUV;

void main() {
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
    vec2 pixelCoord = ((in_pos.xy * 0.5 + 0.5) * in_viewport) + vec2(0.5);
    out_groundUV = pixelCoord * in_inverseViewport;
}

-- weights.fs
#include regen.defines.all
#for W_I to NUM_WEIGHT_MAPS
layout(location = ${W_I}) out vec4 out_weights_${W_I};
#endfor
#for W_I to NUM_BIOME_MAPS
out vec4 out_biomes_${W_I};
#endfor
in vec2 in_groundUV;

const float in_concavityFactor = 0.4;

float material_weight0(float minVal, float maxVal, float smoothStep, float val) {
    return (1.0 - smoothstep(maxVal-smoothStep, maxVal, val));
}
float material_weight1(float minVal, float maxVal, float smoothStep, float val) {
    return smoothstep(minVal, minVal+smoothStep, val);
}
float material_weight2(float minVal, float maxVal, float smoothStep, float val) {
    return smoothstep(minVal, minVal+smoothStep, val) *
        (1.0 - smoothstep(maxVal-smoothStep, maxVal, val));
}

void computeMaterialWeights(float heightNorm, vec3 nor, float slope) {
#for MAT_I to NUM_MATERIALS
    #define2 MAT_K ${GROUND_MATERIAL_${MAT_I}}
    #define2 SLOPE_MODE ${${MAT_K}_MATERIAL_SLOPE_MODE}
    #define2 HEIGHT_MODE ${${MAT_K}_MATERIAL_HEIGHT_MODE}
    #ifdef ${MAT_K}_MATERIAL_HAS_HEIGHT_RANGE
    float weight_${MAT_I} = material_weight${HEIGHT_MODE}(
            ${MAT_K}_MATERIAL_HEIGHT_MIN,
            ${MAT_K}_MATERIAL_HEIGHT_MAX,
            ${MAT_K}_MATERIAL_HEIGHT_SMOOTH,
            heightNorm);
    #else
    float weight_${MAT_I} = 1.0f;
    #endif
    #ifdef ${MAT_K}_MATERIAL_HAS_SLOPE_RANGE
    weight_${MAT_I} *= material_weight${SLOPE_MODE}(
            ${MAT_K}_MATERIAL_SLOPE_MIN,
            ${MAT_K}_MATERIAL_SLOPE_MAX,
            ${MAT_K}_MATERIAL_SLOPE_SMOOTH,
            slope);
    #endif
    #ifdef ${MAT_K}_MATERIAL_HAS_MASK
    #define2 MASK_I ${${MAT_K}_MATERIAL_MASK_IDX}
    // multiply by a mask value
    weight_${MAT_I} *= texture(in_groundMaterialMask, vec3(in_groundUV, ${MASK_I}).r;
    #endif
#endfor

    // normalize weights
    float weightSum = 0.0;
#for MAT_I to NUM_MATERIALS
    weightSum += weight_${MAT_I};
#endfor
    weightSum = max(weightSum, 0.001);
#for MAT_I to NUM_MATERIALS
    weight_${MAT_I} /= weightSum;
#endfor

#ifdef HAS_fallback_MATERIAL
    #define2 FALLBACK_I ${fallback_MATERIAL_IDX}
    weight_${FALLBACK_I} += 0.66 * step(weightSum, 0.001);
#endif

    #if NUM_MATERIALS > 3
    out_weights_0 = vec4(weight_0, weight_1, weight_2, weight_3);
    #elif NUM_MATERIALS > 2
    out_weights_0 = vec4(weight_0, weight_1, weight_2, 0.0);
    #elif NUM_MATERIALS > 1
    out_weights_0 = vec4(weight_0, weight_1, 0.0, 0.0);
    #else
    out_weights_0 = vec4(weight_0, 0.0, 0.0, 0.0);
    #endif
    #if NUM_MATERIALS > 7
    out_weights_1 = vec4(weight_4, weight_5, weight_6, weight_7);
    #elif NUM_MATERIALS > 6
    out_weights_1 = vec4(weight_4, weight_5, weight_6, 0.0);
    #elif NUM_MATERIALS > 5
    out_weights_1 = vec4(weight_4, weight_5, 0.0, 0.0);
    #elif NUM_MATERIALS > 4
    out_weights_1 = vec4(weight_4, 0.0, 0.0, 0.0);
    #endif
}

#ifdef HAS_BIOMES
void computeBiomeWeights(float heightNorm, vec3 nor, float slope) {
    // Concavity via 4-neighbor Laplacian (cheap proxy for valley-ness)
    vec2 texel = 1.0 / vec2(textureSize(in_heightMap, 0));
    float hC  = heightNorm;
    float hN  = texture(in_heightMap, in_groundUV + vec2(0, -texel.y)).r;
    float hS  = texture(in_heightMap, in_groundUV + vec2(0,  texel.y)).r;
    float hE  = texture(in_heightMap, in_groundUV + vec2( texel.x, 0)).r;
    float hW  = texture(in_heightMap, in_groundUV + vec2(-texel.x, 0)).r;
    float lap = (hN + hS + hE + hW - 4.0*hC);
    float concavity = clamp(0.5 + lap * in_concavityFactor, 0.0, 1.0);
    // Temperature & moisture from simple heuristics
    float aspect = atan(nor.x, nor.z);             // [-pi, pi]
    float northness = -cos(aspect) * sin(slope); // [-1,1] (+ north-facing)
    float temperature = clamp(
        1.0 - 0.8*heightNorm - 0.25*northness - 0.15*sin(slope), 0.0, 1.0);
    float moisture = clamp(
        0.35*(1.0 - heightNorm) + 0.50*concavity - 0.15*(slope/1.5707963), 0.0, 1.0);
    // Rockiness: slope + convexity bias
    float rockiness = clamp(
        smoothstep(radians(25.0), radians(50.0), slope) + clamp(0.5 - concavity, 0.0, 0.5), 0.0, 1.0);

#for BIOME_I to NUM_BIOMES
    #define2 BIOME_K ${GROUND_BIOME_${BIOME_I}}
    #ifdef ${BIOME_K}_BIOME_HAS_HEIGHT_RANGE
    #define2 HEIGHT_MODE ${${BIOME_K}_BIOME_HEIGHT_MODE}
    float biome_${BIOME_I} = material_weight${HEIGHT_MODE}(
            ${BIOME_K}_BIOME_HEIGHT_MIN,
            ${BIOME_K}_BIOME_HEIGHT_MAX,
            ${BIOME_K}_BIOME_HEIGHT_SMOOTH,
            heightNorm);
    #else
    float biome_${BIOME_I} = 1.0f;
    #endif
    #ifdef ${BIOME_K}_BIOME_HAS_SLOPE_RANGE
    #define2 SLOPE_MODE ${${BIOME_K}_BIOME_SLOPE_MODE}
    biome_${BIOME_I} *= material_weight${SLOPE_MODE}(
            ${BIOME_K}_BIOME_SLOPE_MIN,
            ${BIOME_K}_BIOME_SLOPE_MAX,
            ${BIOME_K}_BIOME_SLOPE_SMOOTH,
            slope);
    #endif
    #ifdef ${BIOME_K}_BIOME_HAS_TEMPERATURE_RANGE
    #define2 TEMPERATURE_MODE ${${BIOME_K}_BIOME_TEMPERATURE_MODE}
    biome_${BIOME_I} *= material_weight${TEMPERATURE_MODE}(
            ${BIOME_K}_BIOME_TEMPERATURE_MIN,
            ${BIOME_K}_BIOME_TEMPERATURE_MAX,
            ${BIOME_K}_BIOME_TEMPERATURE_SMOOTH,
            temperature);
    #endif
    #ifdef ${BIOME_K}_BIOME_HAS_MOISTURE_RANGE
    #define2 MOISTURE_MODE ${${BIOME_K}_BIOME_MOISTURE_MODE}
    biome_${BIOME_I} *= material_weight${MOISTURE_MODE}(
            ${BIOME_K}_BIOME_MOISTURE_MIN,
            ${BIOME_K}_BIOME_MOISTURE_MAX,
            ${BIOME_K}_BIOME_MOISTURE_SMOOTH,
            moisture);
    #endif
    #ifdef ${BIOME_K}_BIOME_HAS_ROCKINESS_RANGE
    #define2 ROCKINESS_MODE ${${BIOME_K}_BIOME_ROCKINESS_MODE}
    biome_${BIOME_I} *= material_weight${ROCKINESS_MODE}(
            ${BIOME_K}_BIOME_ROCKINESS_MIN,
            ${BIOME_K}_BIOME_ROCKINESS_MAX,
            ${BIOME_K}_BIOME_ROCKINESS_SMOOTH,
            rockiness);
    #endif
    #ifdef ${BIOME_K}_BIOME_HAS_CONCAVITY_RANGE
    #define2 CONCAVITY_MODE ${${BIOME_K}_BIOME_CONCAVITY_MODE}
    biome_${BIOME_I} *= material_weight${CONCAVITY_MODE}(
            ${BIOME_K}_BIOME_CONCAVITY_MIN,
            ${BIOME_K}_BIOME_CONCAVITY_MAX,
            ${BIOME_K}_BIOME_CONCAVITY_SMOOTH,
            concavity);
    #endif
    #ifdef ${BIOME_K}_BIOME_HAS_MASK
    #define2 MASK_I ${${BIOME_K}_BIOME_MASK_IDX}
    // multiply by a mask value
    biome_${BIOME_I} *= texture(in_groundMaterialMask, vec3(in_groundUV, ${MASK_I}).r;
    #endif
#endfor

    // normalize weights
    float biomeSum = 0.0;
#for BIOME_I to NUM_BIOMES
    biomeSum += biome_${BIOME_I};
#endfor
    biomeSum = max(biomeSum, 0.001);
#for BIOME_I to NUM_BIOMES
    biome_${BIOME_I} = clamp(biome_${BIOME_I}, 0.0, 1.0);
#endfor

#ifdef HAS_fallback_BIOME
    #define2 FALLBACK_I ${fallback_BIOME_IDX}
    biome_${FALLBACK_I} += 0.66 * step(biomeSum, 0.001);
#endif

    #if NUM_BIOMES > 3
    out_biomes_0 = vec4(biome_0, biome_1, biome_2, biome_3);
    #elif NUM_BIOMES > 2
    out_biomes_0 = vec4(biome_0, biome_1, biome_2, 0.0);
    #elif NUM_BIOMES > 1
    out_biomes_0 = vec4(biome_0, biome_1, 0.0, 0.0);
    #else
    out_biomes_0 = vec4(biome_0, 0.0, 0.0, 0.0);
    #endif
    #if NUM_BIOMES > 7
    out_biomes_1 = vec4(biome_4, biome_5, biome_6, biome_7);
    #elif NUM_BIOMES > 6
    out_biomes_1 = vec4(biome_4, biome_5, biome_6, 0.0);
    #elif NUM_BIOMES > 5
    out_biomes_1 = vec4(biome_4, biome_5, 0.0, 0.0);
    #elif NUM_BIOMES > 4
    out_biomes_1 = vec4(biome_4, 0.0, 0.0, 0.0);
    #endif
}
#endif

void main() {
    float heightNorm = texture(in_heightMap, in_groundUV).r;
    // compute slope from normal map
    vec3 nor = normalize((texture(in_normalMap, in_groundUV).xzy * 2.0) - 1.0);
    float slope = acos(nor.y); // = acos(dot(normal, up))
    computeMaterialWeights(heightNorm, nor, slope);
#ifdef HAS_BIOMES
    computeBiomeWeights(heightNorm, nor, slope);
#endif
}

------------
------------
-- skirt.vs
#include regen.terrain.ground.vs
-- skirt.fs
#include regen.terrain.ground.fs

------------
------------
-- blanket.vs
#define IS_GROUND_BLANKET
#include regen.terrain.ground.vs
-- blanket.fs
#define IS_GROUND_BLANKET
#include regen.terrain.ground.fs

------------
------------
-- blanket1.vs
#define IS_GROUND_BLANKET
#include regen.terrain.ground.vs
-- blanket1.fs
#include regen.models.mesh.defines
#include regen.layered.defines
#ifdef HAS_INSTANCES
flat in int in_instanceID;
#endif

in vec2 in_texco0;
out float out_collision;

void main() {
    #ifdef HAS_maskIndex
    vec3 maskUV = vec3(in_texco0, float(in_maskIndex));
    #else
    vec2 maskUV = in_texco0;
    #endif
    float blanketMaskVal = texture(in_blanketMask, maskUV).r;
    #ifdef HAS_blanketLifetime
    blanketMaskVal *= in_blanketLifetime;
    #endif
    out_collision = blanketMaskVal;
}

------------
----- Ground render pass.
------------
-- vs
#define HAS_CUSTOM_HANDLE_IO
#include regen.models.mesh.defines

out vec2 out_groundUV;
#ifdef HAS_noiseTexture
    #ifdef USE_VERTEX_NOISE
out vec2 out_offsetXZ;
out vec2 out_offsetYZ;
out vec2 out_offsetXY;
out float out_noiseVal;
    #endif
#endif
#ifdef IS_SKIRT_MESH
out vec3 out_posModel;
#endif

#include regen.terrain.ground.groundUV

void customHandleIO(vec3 posWorld, vec3 posEye, vec3 norWorld) {
    out_groundUV = groundUV(posWorld.xyz, norWorld);
#ifdef HAS_noiseTexture
    #ifdef USE_VERTEX_NOISE
    float noiseVal = texture(in_noiseTexture, posWorld.xz * in_noiseScale).r;
    vec2 noiseOffset = noiseVal * vec2(0.1, 0.15);
    out_offsetXZ = (posWorld.xz + noiseOffset);
    out_offsetYZ = (posWorld.yz + noiseOffset);
    out_offsetXY = (posWorld.xy + noiseOffset);
    out_noiseVal = noiseVal * 0.4;
    #endif
#endif
#ifdef IS_SKIRT_MESH
    out_posModel = in_pos.xyz;
#endif
}

#include regen.models.mesh.vs

-- fs
// add custom fragment texture mapping to regular mesh fragment shader.
#define CUSTOM_FRAGMENT_MAPPING_KEY regen.terrain.ground.fragmentMapping
#include regen.models.mesh.fs

-- fragmentMapping
#ifndef ground_fragmentMapping_included
#define2 ground_fragmentMapping_included
#ifndef MAX_NUM_BLENDED_MATERIALS
#define MAX_NUM_BLENDED_MATERIALS 3
#endif
#ifndef MIN_MATERIAL_WEIGHT
#define MIN_MATERIAL_WEIGHT 0.1
#endif
// Avoid triplanar blending of normals in case material does not have a normal map.
#define USE_NORMAL_STEPPING
#define USE_UNROLLED_LOOP

#define IDX_ID ${TEX_ID_groundMaterialIndices0}
#define IDX_WIDTH ${TEX_WIDTH${IDX_ID}}
#define IDX_HEIGHT ${TEX_HEIGHT${IDX_ID}}

#ifdef HAS_blanketMask
in vec2 in_texco0;
#endif
// computed in VS
in vec2 in_groundUV;
#ifdef HAS_noiseTexture
    #ifdef USE_VERTEX_NOISE
in vec2 in_offsetXZ;
in vec2 in_offsetYZ;
in vec2 in_offsetXY;
in float in_noiseVal;
    #endif
#endif
#ifdef IS_SKIRT_MESH
in vec3 in_posModel;
#endif

const float in_noiseScale = 0.005;

vec3 materialAlbedo(uint matIndex, vec3 blending, vec2 xz, vec2 yz, vec2 xy) {
    return
        blending.x * texture(in_groundMaterialAlbedo, vec3(yz,matIndex)).rgb +
        blending.y * texture(in_groundMaterialAlbedo, vec3(xz,matIndex)).rgb +
        blending.z * texture(in_groundMaterialAlbedo, vec3(xy,matIndex)).rgb;
}

#ifdef HAS_groundMaterialNormal
vec3 materialNormal(uint matIndex, vec3 blending, vec2 xz, vec2 yz, vec2 xy) {
    vec3 nx = texture(in_groundMaterialNormal, vec3(yz, matIndex)).xyz * 2.0 - 1.0;
    vec3 ny = texture(in_groundMaterialNormal, vec3(xz, matIndex)).xyz * 2.0 - 1.0;
    vec3 nz = texture(in_groundMaterialNormal, vec3(xy, matIndex)).xyz * 2.0 - 1.0;
    // note: we normalize once globally
    return nx * blending.x + ny * blending.y + nz * blending.z;
}
#endif

mat3 materialTBN(vec3 n) {
    // Build TBN from baseNor (world-space normal) and construct tangent + bitangent
    // Decide whether to use (0,1,0) or (1,0,0)
    // Note: we compute both cases below to avoid branches.
    float useY = step(abs(n.x), abs(n.y)); // 1 if |y| > |x|
    // Cross with up=(0,1,0) -> ( n.z, 0, -n.x )
    vec3 tY = vec3(n.z, 0.0, -n.x);
    // Cross with up=(1,0,0) -> ( 0, -n.z, n.y )
    vec3 tX = vec3(0.0, -n.z, n.y);
    // Blend based on which axis we choose
    vec3 t = mix(tX, tY, useY);
    // Normalize tangent
    t *= inversesqrt(dot(t, t));
    // Bitangent is cross(n, t)
    // Note: cross(n, t) is already normalized as n/t are normalized and orthogonal.
    return mat3(t, cross(n, t), n);
}

struct TopMaterials {
    float numTopMaterials; // integer count (0..MAX_NUM_BLENDED_MATERIALS)
    float topWeightSum;
    vec4 topWeights;
    ivec4 topIndices;
};

TopMaterials findTopWeights(in vec4 in_w) {
    TopMaterials tm;
    tm.numTopMaterials = 0.0;
    tm.topWeightSum = 0.0;
    tm.topWeights = in_w;
    tm.topIndices = ivec4(0,1,2,3);
    float tmpf;
    int tmpi;
    // Sorting network for 4 inputs (descending)
    // small helper inline: compare-exchange so that (x >= y) after
    // swap if left < right -> put larger to left
    #if 1 // CMP_SWAP without branches
    #define CMP_SWAP(a, b, ia, ib) { \
        tmpf = step((b), (a)); \
        tmpi = int(c); \
        (a) = mix(b,a,tmpf); \
        (b) = mix(a,b,tmpf); \
        (ia) = tmpi * (ia) + (1 - tmpi) * (ib); \
        (ib) = tmpi * (ib) + (1 - tmpi) * (ia); }
    #else
    #define CMP_SWAP(x, y, ix, iy) \
      if ((x) < (y)) { \
        tmpf = (x); (x) = (y); (y) = tmpf; \
        tmpi = (ix); (ix) = (iy); (iy) = tmpi; }
    #endif
    CMP_SWAP(tm.topWeights.x, tm.topWeights.y, tm.topIndices.x, tm.topIndices.y); // 0-1
    CMP_SWAP(tm.topWeights.z, tm.topWeights.w, tm.topIndices.z, tm.topIndices.w); // 2-3
    CMP_SWAP(tm.topWeights.x, tm.topWeights.z, tm.topIndices.x, tm.topIndices.z); // 0-2
    #if MAX_NUM_BLENDED_MATERIALS > 1
    CMP_SWAP(tm.topWeights.y, tm.topWeights.z, tm.topIndices.y, tm.topIndices.z); // 1-2
    #endif
    #if MAX_NUM_BLENDED_MATERIALS > 2
    CMP_SWAP(tm.topWeights.z, tm.topWeights.w, tm.topIndices.z, tm.topIndices.w); // 2-3
    #endif
    #undef CMP_SWAP
    // count how many exceed MIN_MATERIAL_WEIGHT
    // step(MIN_MATERIAL_WEIGHT, v) => 1.0 when v >= MIN_MATERIAL_WEIGHT else 0.0
#for TOP_I to MAX_NUM_BLENDED_MATERIALS
    tmpf = step(MIN_MATERIAL_WEIGHT, tm.topWeights[${TOP_I}]);
    tm.numTopMaterials += tmpf;
    tm.topWeightSum    += tmpf * tm.topWeights[${TOP_I}];
#endfor
    return tm;
}

#if NUM_MATERIALS > 4
TopMaterials findTopWeights(in vec4 in_w0, in vec4 in_w1) {
    // compute top weights for each 4-material block
    TopMaterials tm0 = findTopWeights(in_w0);
    TopMaterials tm1 = findTopWeights(in_w1);
    TopMaterials tm;
    ivec4 aI = tm0.topIndices; // 0..3
    ivec4 bI = tm1.topIndices + 4; // shift second block
    int ia = 0, ib = 0;
    // merge top-4 branchlessly
    for (int i = 0; i < 4; i++) {
        // clamp indices
        int curA = min(ia, 3);
        int curB = min(ib, 3);
        // branchless selection mask
        float useA = step(b[curB], a[curA]); // 1.0 if a >= b, else 0.0
        // select weight
        tm.topWeights[i] = mix(
            tm1.topWeights[curB],
            tm0.topWeights[curA], useA);
        // select index
        tm.topIndices[i] = ivec4(mix(
            float(bI[curB]),
            float(aI[curA]), useA))[0];
        // increment counters
        ia += int(useA);
        ib += int(1.0 - useA);
    }
    // compute mask/count/sum using MIN_MATERIAL_WEIGHT
    vec4 mask = step(vec4(MIN_MATERIAL_WEIGHT), tm.topWeights);
    tm.numTopMaterials = mask.x + mask.y + mask.z + mask.w;
    tm.topWeightSum    = dot(mask, tm.topWeights);
    return tm;
}
#endif // NUM_MATERIALS > 4

void customFragmentMapping(in vec3 worldPos, inout vec4 outColor, inout vec3 outNormal) {
#ifdef IS_SKIRT_MESH
    vec3 baseNor = vec3(
            step(SKIRT_PATCH_SIZE_HALF - 0.001, abs(in_posModel.x)) * sign(in_posModel.x),
            0.0,
            step(SKIRT_PATCH_SIZE_HALF - 0.001, abs(in_posModel.z)) * sign(in_posModel.z));
#else
    vec3 baseNor = normalize((texture(in_normalMap, in_groundUV).xzy * 2.0) - 1.0);
#endif
    vec3 blending = abs(baseNor * baseNor); // sharper transitions
    blending /= (blending.x + blending.y + blending.z);
#ifdef IS_SKIRT_MESH
    baseNor = normalize((texture(in_normalMap, in_groundUV).xzy * 2.0) - 1.0);
#endif
    mat3 TBN = materialTBN(baseNor);

#ifdef HAS_noiseTexture
    #ifndef USE_VERTEX_NOISE
    float in_noiseVal = texture(in_noiseTexture, worldPos.xz * in_noiseScale).r;
    vec2 noiseOffset = noiseVal * vec2(0.1, 0.15);
    vec2 in_offsetXZ = (worldPos.xz + noiseOffset);
    vec2 in_offsetYZ = (worldPos.yz + noiseOffset);
    vec2 in_offsetXY = (worldPos.xy + noiseOffset);
    in_noiseVal *= 0.4;
    #endif
#else
    #define in_offsetXZ worldPos.xz
    #define in_offsetYZ worldPos.yz
    #define in_offsetXY worldPos.xy
#endif

    // Sample the material weights
#if NUM_MATERIALS > 4
    vec4 materialWeights0 = texture(in_groundMaterialWeights0, in_groundUV);
    vec4 materialWeights1 = texture(in_groundMaterialWeights1, in_groundUV);
#else
    vec4 materialWeights0 = texture(in_groundMaterialWeights0, in_groundUV);
#endif

#if NUM_GROUND_MASKS > 0
    // Apply ground masks
    vec2 screenUV = gl_FragCoord.xy * in_inverseViewport;
    float maskVal;
    #for MASK_I to NUM_GROUND_MASKS
        #define2 _NUM_CH ${MASK_${MASK_I}_NUM_CHANNELS}
        #define2 _FALLBACK_CH ${MASK_${MASK_I}_FALLBACK_CHANNEL}
        #define2 _FALLBACK_INT ${MASK_${MASK_I}_FALLBACK_INTENSITY}
        #if ${_FALLBACK_INT} < 4
            #define2 FALLBACK_VEC materialWeights0
        #else
            #define2 FALLBACK_VEC materialWeights1
        #endif
    maskVal = texture(in_groundMask${MASK_I}, screenUV).r;
    // Apply fallback intensity to fallback channel
    ${FALLBACK_VEC}[${_FALLBACK_CH}] = min(1.0,
        ${FALLBACK_VEC}[${_FALLBACK_CH}] + maskVal * ${_FALLBACK_INT});
    // Apply mask to channels
        #for CH_I to ${_NUM_CH}
            #define2 _CHANNEL ${MASK_${MASK_I}_CHANNEL_${CH_I}}
            #define2 _INVERT ${MASK_${MASK_I}_INVERT_${CH_I}}
            #define2 _BLEND_MODE ${MASK_${MASK_I}_BLEND_MODE_${CH_I}}
            #define2 _BLEND_FACTOR ${MASK_${MASK_I}_BLEND_FACTOR_${CH_I}}
            #if ${_CHANNEL} < 4
                #define2 W_VEC materialWeights0
            #else
                #define2 W_VEC materialWeights1
            #endif
    maskVal *= ${_BLEND_FACTOR};
            #if ${_INVERT} == 1
    maskVal = 1.0 - maskVal;
            #endif
            #if ${_BLEND_MODE} == mul
    ${W_VEC}[${_CHANNEL}] *= maskVal;
            #else
                #if ${_BLEND_MODE} == add
    ${W_VEC}[${_CHANNEL}] += maskVal;
                #else
    #warning "Unknown MASK_BLEND_MODE ${_BLEND_MODE}"
                #endif
            #endif
        #endfor
    #endfor
#endif

    // Find the top N weights and indices
    #if NUM_MATERIALS > 4
    TopMaterials tm = findTopWeights(materialWeights0, materialWeights1);
    #else
    TopMaterials tm = findTopWeights(materialWeights0);
    #endif
    float ww = 1.0 / tm.topWeightSum;
    // normalize top weights
    #for TOP_I to MAX_NUM_BLENDED_MATERIALS
    tm.topWeights[${TOP_I}] *= ww;
    #endfor

    int topIdx, norIdx;
    int count = int(tm.numTopMaterials);
    vec3 color = vec3(0.0);
#ifdef HAS_groundMaterialNormal
    vec3 normal = vec3(0.0);
#endif
    vec2 xz, yz, xy;
    float matUVScale;
#ifdef USE_NORMAL_STEPPING
    float norStep;
#endif
    // note: help GPU unrolling the loop by looping over MAX_NUM_BLENDED_MATERIALS.
    //       this is faster in my tests than using a for-loop with count.
    for (int i = 0; i < MAX_NUM_BLENDED_MATERIALS; ++i) {
        if (i >= count) {  break; }
        topIdx = tm.topIndices[i];
        norIdx = in_groundMaterialNormalIdx[topIdx];
        matUVScale = in_groundMaterialUVScale[topIdx];
        ww = tm.topWeights[i];
        xz = in_offsetXZ * matUVScale;
        yz = in_offsetYZ * matUVScale;
        xy = in_offsetXY * matUVScale;
        color += ww * materialAlbedo(topIdx, blending, xz, yz, xy);
#ifdef HAS_groundMaterialNormal
    #ifdef USE_NORMAL_STEPPING
        norStep = step(GROUND_NUM_NORMAL_MAPS, norIdx);
        normal   += norStep * ww * materialNormal(norIdx, blending, xz, yz, xy);
        normal.z += (1.0 - norStep) * ww; // TBN up vector vec3(0,0,1)
    #else
        normal += ww * materialNormal(norIdx, blending, xz, yz, xy);
    #endif
#endif
    }
#ifdef HAS_noiseTexture
    // adjust color based on noise
    color *= mix(vec3(1.0), vec3(0.5), in_noiseVal);
#endif
#ifdef HAS_groundMaterialNormal
    // Finally, write the color and normal to the output
    // Note: normalization is done in the main().
    outNormal = TBN * normal;
#else
    outNormal = TBN * vec3(0.0, 0.0, 1.0);
#endif
    outColor = vec4(color, 1.0);
}
#endif // ground_fragmentMapping_included
