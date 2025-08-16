
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

-- groundHeightBlend
#ifndef ground_heightBlend_included
#define2 ground_heightBlend_included
#include regen.terrain.ground.groundUV
void groundHeightBlend(in vec3 offset, inout vec3 P, float one) {
    P += offset;
    // next do some special treatment for skirt vertices, we detect them by y position...
    float zeroLevel = in_mapCenter.y - in_mapSize.y * 0.5f;
    if (P.y < zeroLevel - 1e-6) {
        // TODO: Reconsider the skirt handling.
        //      - maybe we can avoid sampling the normal map here? we could just push downwards
        //        which would be less expensive.
        //      - avoiding the conditional would also be good.
        //      - also I had the feeling that this still looks wrong in some cases, better to investigate again.
        // sample the normal map
        vec3 nor = normalize((texture(in_normalMap, groundUV(P)).xzy * 2.0) - 1.0);
        //vec3 nor = vec3(0.0, 1.0, 0.0);
        // translate back to original vertex, and
        // offset the vertex position in opposite direction of the normal
        P += nor * in_skirtSize - vec3(0.0, in_skirtSize, 0.0);
    }
}
#endif // ground_heightBlend_included

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
layout(location = 0) out vec4 out_weights;
layout(location = 1) out uvec4 out_indices;
in vec2 in_groundUV;

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

struct TopEntry {
    float w;
    uint idx;
};
TopEntry TopEntry_ctor(float w, uint idx) {
    TopEntry t;
    t.w=w;
    t.idx=idx;
    return t;
}
void insertTopN(in float w, in uint idx, inout TopEntry best[MAX_NUM_BLENDED_MATERIALS]) {
    if (w > best[0].w) {
#if ${MAX_NUM_BLENDED_MATERIALS} >= 4
        best[3] = best[2];
#endif
#if ${MAX_NUM_BLENDED_MATERIALS} >= 3
        best[2] = best[1];
#endif
        best[1] = best[0];
        best[0] = TopEntry_ctor(w, idx);
    }
    else if (w > best[1].w) {
#if ${MAX_NUM_BLENDED_MATERIALS} >= 4
        best[3] = best[2];
#endif
#if ${MAX_NUM_BLENDED_MATERIALS} >= 3
        best[2] = best[1];
#endif
        best[1] = TopEntry_ctor(w, idx);
    }
#if ${MAX_NUM_BLENDED_MATERIALS} >= 3
    else if (w > best[2].w) {
#if ${MAX_NUM_BLENDED_MATERIALS} >= 4
        best[3] = best[2];
#endif
        best[2] = TopEntry_ctor(w, idx);
    }
#endif
#if ${MAX_NUM_BLENDED_MATERIALS} >= 4
    else if (w > best[3].w) {
        best[3] = TopEntry_ctor(w, idx);
    }
#endif
}

void main() {
    float heightNorm = texture(in_heightMap, in_groundUV).r;
    // compute slope from normal map
    vec3 nor = normalize((texture(in_normalMap, in_groundUV).xzy * 2.0) - 1.0);
    float slope = acos(nor.y); // = acos(dot(normal, up))

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

#ifdef HAS_stone_MATERIAL && HAS_dirt_MATERIAL
    #define2 STONE_I ${stone_MATERIAL_IDX}
    #define2 DIRT_I ${dirt_MATERIAL_IDX}
    // Remove dirt where rock is strong
    // TODO: can we make this configurable?
    weight_${DIRT_I} *= (1.0 - weight_${STONE_I});
#endif
#ifdef HAS_fallback_MATERIAL
    #define2 FALLBACK_I ${fallback_MATERIAL_IDX}
    weight_${FALLBACK_I} += 0.66 * step(weightSum, 0.001);
#endif

    TopEntry best[MAX_NUM_BLENDED_MATERIALS];
#for TOP_I to MAX_NUM_BLENDED_MATERIALS
    best[${TOP_I}] = TopEntry_ctor(-1e9, 0);
#endfor
#for MAT_I to NUM_MATERIALS
    insertTopN(weight_${MAT_I}, ${MAT_I}, best);
#endfor

    // Normalize weights and clamp negatives
    float topSum = 0.0;
#for TOP_I to MAX_NUM_BLENDED_MATERIALS
    topSum += best[${TOP_I}].w;
#endfor
    float invTopSum = topSum > 1e-5 ? 1.0/topSum : 0.0;

#if MAX_NUM_BLENDED_MATERIALS > 3
    out_weights = vec4(best[0].w, best[1].w, best[2].w, best[3].w) * invTopSum;
#elif MAX_NUM_BLENDED_MATERIALS > 2
    out_weights = vec4(vec3(best[0].w, best[1].w, best[2].w) * invTopSum, 0.0);
#elif MAX_NUM_BLENDED_MATERIALS > 1
    out_weights = vec4(vec2(best[0].w, best[1].w) * invTopSum, 0.0, 0.0);
#else
    out_weights = vec4(1.0, 0.0, 0.0, 0.0);
#endif
#if MAX_NUM_BLENDED_MATERIALS > 3
    out_indices = uvec4(best[0].idx, best[1].idx, best[2].idx, best[3].idx);
#elif MAX_NUM_BLENDED_MATERIALS > 2
    out_indices = uvec4(best[0].idx, best[1].idx, best[2].idx, -1);
#elif MAX_NUM_BLENDED_MATERIALS > 1
    out_indices = uvec4(best[0].idx, best[1].idx, -1, -1);
#else
    out_indices = uvec4(best[0].idx, -1, -1, -1);
#endif
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
}

#include regen.models.mesh.vs

-- fs
// TODO: Add support for linear filtering of the material textures.
//       But this is not trivial using the top-N approach!
//       - ordering is not consistent (ordered by weight, not material index)
//       - samples may have different set of indices
// add custom fragment texture mapping to regular mesh fragment shader.
#include regen.terrain.ground.customFragmentMapping
#include regen.models.mesh.fs

-- customFragmentMapping
#ifndef customFragmentMapping_included
#define2 customFragmentMapping_included
#define HAS_CUSTOM_FRAGMENT_MAPPING
#include regen.terrain.ground.materialUV

#define IDX_ID ${TEX_ID_groundMaterialIndices0}
#define IDX_WIDTH ${TEX_WIDTH${IDX_ID}}
#define IDX_HEIGHT ${TEX_HEIGHT${IDX_ID}}

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

const float in_noiseScale = 0.005;

vec3 materialAlbedo(uint matIndex, vec3 blending, vec2 xz, vec2 yz, vec2 xy) {
    return
        blending.x * texture(in_groundMaterialAlbedo, vec3(yz,matIndex)).rgb +
        blending.y * texture(in_groundMaterialAlbedo, vec3(xz,matIndex)).rgb +
        blending.z * texture(in_groundMaterialAlbedo, vec3(xy,matIndex)).rgb;
}

vec3 materialNormal(uint matIndex, vec3 blending, vec2 xz, vec2 yz, vec2 xy) {
    vec3 nx = texture(in_groundMaterialNormal, vec3(yz, matIndex)).xyz * 2.0 - 1.0;
    vec3 ny = texture(in_groundMaterialNormal, vec3(xz, matIndex)).xyz * 2.0 - 1.0;
    vec3 nz = texture(in_groundMaterialNormal, vec3(xy, matIndex)).xyz * 2.0 - 1.0;
    // note: we normalize once globally
    return nx * blending.x + ny * blending.y + nz * blending.z;
}

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

void customFragmentMapping(in vec3 worldPos, inout vec4 outColor, inout vec3 outNormal) {
    // read normal map and compute TBN
    vec3 baseNor = normalize((texture(in_normalMap, in_groundUV).xzy * 2.0) - 1.0);
    mat3 TBN = materialTBN(baseNor);
    // compute factors for triplanar blending
    vec3 blending = abs(baseNor * baseNor); // sharper transitions
    blending /= (blending.x + blending.y + blending.z);

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

    vec4 weights1 = texture(in_groundMaterialWeights0, in_groundUV);
    uvec4 indices1 = texelFetch(in_groundMaterialIndices0,
        ivec2(floor(in_groundUV * vec2(${IDX_WIDTH}, ${IDX_HEIGHT}))), 0);

    // Note: we do nearest sampling, so weights should add to 1 already.
    // Accumulate color and normal from all materials
    vec2 xz, yz, xy;
    float matUVScale;
    vec3 blendedNor = vec3(0.0);
    vec3 color = vec3(0.0);
#for TOP_I to MAX_NUM_BLENDED_MATERIALS
    matUVScale = in_groundMaterialUVScale[indices1[${TOP_I}]];
    xz = in_offsetXZ * matUVScale;
    yz = in_offsetYZ * matUVScale;
    xy = in_offsetXY * matUVScale;
    color += weights1[${TOP_I}] * materialAlbedo(
                indices1[${TOP_I}],
                blending,
                xz, yz, xy);
    // Read the normal index for this material. This is not necessarily the same as material index
    // because some materials may not have a normal map at all (indicated by -1)
    blendedNor += weights1[${TOP_I}] * materialNormal(
                in_groundMaterialNormalIdx[indices1[${TOP_I}]],
                blending,
                xz, yz, xy);
#endfor
#ifdef HAS_noiseTexture
    // adjust color based on noise
    color *= mix(vec3(1.0), vec3(0.5), in_noiseVal);
#endif
    // Finally, write the color and normal to the output
    outNormal = normalize(TBN * blendedNor);
    outColor = vec4(color, 1.0);
}
#endif
