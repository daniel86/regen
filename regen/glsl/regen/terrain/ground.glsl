
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
#for W_I to NUM_WEIGHT_MAPS
layout(location = ${W_I}) out vec4 out_weights_${W_I};
#endfor
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

void main() {
    float heightNorm = texture(in_heightMap, in_groundUV).r;
    // compute slope from normal map
    vec3 nor = normalize((texture(in_normalMap, in_groundUV).xzy * 2.0) - 1.0);
    float slope = acos(nor.y); // = acos(dot(normal, up))
    float weightSum = 0.0;

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
    weightSum += weight_${MAT_I};
#endfor

    // normalize weights
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
    // write weights to output
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

------------
----- Ground render pass.
------------
-- vs
// regular mesh vertex shader, can do vertex displacement when ground height texture is available.
#include regen.models.mesh.vs

-- fs
// add custom fragment texture mapping to regular mesh fragment shader.
#include regen.terrain.ground.customFragmentMapping
#include regen.models.mesh.fs

-- customFragmentMapping
#ifndef customFragmentMapping_included
#define2 customFragmentMapping_included
#define HAS_CUSTOM_FRAGMENT_MAPPING
#include regen.terrain.ground.groundUV
#include regen.terrain.ground.materialUV

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
    vec2 uv = groundUV(worldPos);
    vec3 blendedNor = vec3(0.0);
    vec3 color = vec3(0.0);

    // read normal map and compute TBN
    vec3 baseNor = normalize((texture(in_normalMap, uv).xzy * 2.0) - 1.0);
    mat3 TBN = materialTBN(baseNor);

    // read material weights
    vec4 weights1 = texture(in_groundMaterialWeights0, uv);
    #define weights_0 weights1.r
    #define weights_1 weights1.g
    #define weights_2 weights1.b
    #define weights_3 weights1.a
#if NUM_MATERIALS > 4
    vec4 weights2 = texture(in_groundMaterialWeights1, uv);
    #define weights_4 weights2.r
    #define weights_5 weights2.g
    #define weights_6 weights2.b
    #define weights_7 weights2.a
#endif
    // Compute sum of weights
    float weightSum = 0.0;
#for MAT_I to NUM_MATERIALS
    weightSum += weights_${MAT_I};
#endfor
    // normalize weights
#for MAT_I to NUM_MATERIALS
    weights_${MAT_I} /= max(weightSum, 0.001);
#endfor

    // compute factors for triplanar blending
    vec3 blending = abs(baseNor * baseNor); // sharper transitions
    blending /= (blending.x + blending.y + blending.z);

    // accumulate color and normal from all materials
    // TODO: Optimization: Experiment with thresholds and conditional computation
    //        to avoid texture lookups.
    //       - maybe best to use dynamic loop with continue?
    //         Or branching in the unrolled code?
    //       - another idea: compute n-best first, maybe with fixed n
    //         then accumulate only these.
    // TODO: Optimization: Pack normals into alpha component of albedo.
    //       - reduces texture lookups 50%
    //       - A = normal.x*0.5+0.5  // store only X, reconstruct Z in shader
    //         vec2 normalXY = decodeNormal(sample.a); // e.g., reconstruct Z as sqrt(1-x²-y²)
    vec2 xz, yz, xy;
#ifdef HAS_noiseTexture
    float noiseVal = texture(in_noiseTexture, worldPos.xz * in_noiseScale).r;
    vec2 noiseOffset;
    #for MAT_I to NUM_MATERIALS
    #define2 MAT_K ${GROUND_MATERIAL_${MAT_I}}
    #define2 UV_SCALE ${${MAT_K}_MATERIAL_UV_SCALE}
    // TODO: make this configurable
    //noiseOffset = noiseOffset[matIndex];
    noiseOffset = vec2(0.1, 0.15);
    xz = (worldPos.xz + noiseVal * noiseOffset) * ${UV_SCALE};
    yz = (worldPos.yz + noiseVal * noiseOffset) * ${UV_SCALE};
    xy = (worldPos.xy + noiseVal * noiseOffset) * ${UV_SCALE};
    color      += weights_${MAT_I} * materialAlbedo(${MAT_I}, blending, xz, yz, xy);
    blendedNor += weights_${MAT_I} * materialNormal(${MAT_I}, blending, xz, yz, xy);
    #endfor
#else
    #for MAT_I to NUM_MATERIALS
    #define2 MAT_K ${GROUND_MATERIAL_${MAT_I}}
    #define2 UV_SCALE ${${MAT_K}_MATERIAL_UV_SCALE}
    xz = worldPos.xz * ${UV_SCALE};
    yz = worldPos.yz * ${UV_SCALE};
    xy = worldPos.xy * ${UV_SCALE};
    color      += weights_${MAT_I} * materialAlbedo(${MAT_I}, blending, xz, yz, xy);
    blendedNor += weights_${MAT_I} * materialNormal(${MAT_I}, blending, xz, yz, xy);
    #endfor
#endif

    // apply bumped normal, or fallback to base normal
    float blendedNorLength = length(blendedNor);
    blendedNor /= max(blendedNorLength, 0.001);
    blendedNor  = mix(baseNor,
            normalize(TBN * blendedNor),
            step(0.001, blendedNorLength));
#ifdef HAS_noiseTexture
    // adjust color based on noise
    color *= mix(vec3(1.0), vec3(0.5), noiseVal * 0.4);
#endif

    // Finally, write the color and normal to the output
    outNormal = blendedNor;
    outColor = vec4(color, 1.0);
    //outColor = vec4(noiseVal,0,0, 1.0);
}
#endif // customFragmentMapping_included
