
-- filtering.vsm
#ifndef REGEN_SM_FILTER_VSM_included__
#define2 REGEN_SM_FILTER_VSM_included__
#include regen.math.linstep

float chebyshevUpperBound(float dist, vec2 moments) {
    float variance = max(moments.y - moments.x*moments.x, 0.01);
    float d = dist - moments.x;
    float p = float(dist < moments.x);
    float p_max = linstep(0.2, 1.0, variance/(variance + d*d));
    return clamp(max(p, p_max), 0.0, 1.0);
}

float shadowVSM(sampler2DShadow tex, vec4 shadowCoord, float linearDepth) {
    float shadow = textureProj(tex, shadowCoord);
    return chebyshevUpperBound(linearDepth, vec2(shadow));
}
float shadowVSM(samplerCubeShadow tex, vec4 shadowCoord, float linearDepth) {
    float shadow = texture(tex, shadowCoord);
    return chebyshevUpperBound(linearDepth, vec2(shadow));
}
float shadowVSM_ortho(sampler2DArrayShadow tex, vec4 shadowCoord) {
    float shadow = texture(tex, shadowCoord);
    // Ortho matrix projects linear depth
    float depth = shadowCoord.w;
    return chebyshevUpperBound(depth, vec2(shadow));
}
#endif

-- filtering.gaussian
#ifndef REGEN_SM_FILTER_GAUSS_included__
#define2 REGEN_SM_FILTER_GAUSS_included__
#include regen.math.computeCubeOffset

// Gaussian 3x3 filter
float shadowGaussian(sampler2DShadow tex, vec4 shadowCoord) {
    float ret = textureProj(tex, shadowCoord) * 0.25;
    ret += textureProjOffset(tex, shadowCoord, ivec2( -1, -1)) * 0.0625;
    ret += textureProjOffset(tex, shadowCoord, ivec2( -1, 0)) * 0.125;
    ret += textureProjOffset(tex, shadowCoord, ivec2( -1, 1)) * 0.0625;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 0, -1)) * 0.125;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 0, 1)) * 0.125;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 1, -1)) * 0.0625;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 1, 0)) * 0.125;
    ret += textureProjOffset(tex, shadowCoord, ivec2( 1, 1)) * 0.0625;
    return ret;
}
float shadowGaussian(sampler2DArrayShadow tex, vec4 shadowCoord) {
    float ret = texture(tex, shadowCoord) * 0.25;
    ret += textureOffset(tex, shadowCoord, ivec2( -1, -1)) * 0.0625;
    ret += textureOffset(tex, shadowCoord, ivec2( -1, 0)) * 0.125;
    ret += textureOffset(tex, shadowCoord, ivec2( -1, 1)) * 0.0625;
    ret += textureOffset(tex, shadowCoord, ivec2( 0, -1)) * 0.125;
    ret += textureOffset(tex, shadowCoord, ivec2( 0, 1)) * 0.125;
    ret += textureOffset(tex, shadowCoord, ivec2( 1, -1)) * 0.0625;
    ret += textureOffset(tex, shadowCoord, ivec2( 1, 0)) * 0.125;
    ret += textureOffset(tex, shadowCoord, ivec2( 1, 1)) * 0.0625;
    return ret;
}
float shadowGaussian(samplerCubeShadow tex, vec4 coord, float texelSize) {
    vec3 dx, dy;
    computeCubeOffset(coord.xyz, texelSize, dx, dy);
    
    float ret = texture(tex, coord) * 0.25;
#define CUBE_MAP_OFFSET(off) float(texture(tex, vec4(coord.xyz + off.x*dx + off.y*dy, coord.w)))
    ret += CUBE_MAP_OFFSET(vec2(-1,-1)) * 0.0625;
    ret += CUBE_MAP_OFFSET(vec2(-1, 0)) * 0.125;
    ret += CUBE_MAP_OFFSET(vec2(-1, 1)) * 0.0625;
    ret += CUBE_MAP_OFFSET(vec2( 0,-1)) * 0.125;
    ret += CUBE_MAP_OFFSET(vec2( 0, 1)) * 0.125;
    ret += CUBE_MAP_OFFSET(vec2( 1,-1)) * 0.0625;
    ret += CUBE_MAP_OFFSET(vec2( 1, 0)) * 0.125;
    ret += CUBE_MAP_OFFSET(vec2( 1, 1)) * 0.0625;
#undef CUBE_MAP_OFFSET
    return ret;
}
#endif

-- filtering.all
#include regen.shading.shadow-mapping.filtering.gaussian
#include regen.shading.shadow-mapping.filtering.vsm

--------------------------------------
--------------------------------------
------ Shadow map sampling functions
--------------------------------------
--------------------------------------
-- sampling.dir
#include regen.shading.shadow-mapping.filtering.all

vec4 dirShadowCoord(int layer, vec3 posWorld, mat4 lightMatrix) {
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = lightMatrix*vec4(posWorld,1.0);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.w = shadowCoord.z;
    // tell glsl in which layer to do the look up
    shadowCoord.z = float(layer);
    return shadowCoord;
}

#define dirShadowSingle(tex,x)   float(texture(tex, x))
#define dirShadowGaussian(tex,x) shadowGaussian(tex, x)
#define dirShadowVSM(tex,x)      shadowVSM_ortho(tex, coord)

-- sampling.point.parabolic
#include regen.shading.shadow-mapping.filtering.all
#include regen.math.linstep

vec4 parabolicShadowCoord(int textureLayer, vec3 posWorld, mat4 lightMatrix, float near, float far) {
    // Transform the world space position to light eye space, i.e. as seen from the light perspective
    vec4 lightSpacePos = lightMatrix * vec4(posWorld, 1.0);
    vec4 shadowCoord = vec4(normalize(lightSpacePos.xyz), 0.0);
    // Compute the parabolic coordinates and
    // adjust the coordinates to fit within the [0, 1] range
    shadowCoord.xy = shadowCoord.xy * 0.5 / (1.0 + shadowCoord.z) + 0.5;
    // flip X coordinate
    shadowCoord.x = 1.0 - shadowCoord.x;
    // Z coordinate is the index into array texture
    shadowCoord.z = float(textureLayer);
    // Calculate the non-linear depth value for the paraboloid projection
    lightSpacePos.xyz /= lightSpacePos.w;
    shadowCoord.w = (length(lightSpacePos.xyz) - near) / (far - near);
    // convert into NDC space
    shadowCoord.w = (shadowCoord.w + 1.0) * 0.5;
    // NOTE: could add bias here if self-shadowing occurs
    //const float bias = 0.1;
    //shadowCoord.w -= bias;
    return shadowCoord;
}

#define parabolicShadowSingle(tex,x)   float(texture(tex, x))
#define parabolicShadowGaussian(tex,x) shadowGaussian(tex, x)
#define parabolicShadowVSM(tex,x)      shadowVSM_ortho(tex, coord)

-- sampling.point
#include regen.shading.shadow-mapping.filtering.all
#include regen.math.linstep

#define pointShadowSingle(tex,l,d,n,f,s)   texture(tex, vec4(-l,d))
#define pointShadowGaussian(tex,l,d,n,f,s) shadowGaussian(tex, vec4(-l,d), s)
#define pointShadowVSM(tex,l,d,n,f,s)      shadowVSM(tex,vec4(-l,d), linstep(n,f,length(l))

-- sampling.point.cube
#include regen.shading.shadow-mapping.sampling.point

-- sampling.spot
#include regen.shading.shadow-mapping.filtering.all
#include regen.math.linstep

#define spotShadowSingle(tex,x,l,n,f)   textureProj(tex,x)
#define spotShadowGaussian(tex,x,l,n,f) shadowGaussian(tex,x)
#define spotShadowVSM(tex,x,l,n,f)      shadowVSM(tex,x,linstep(n,f,length(l)))

-- sampling.all
#include regen.shading.shadow-mapping.sampling.dir
#include regen.shading.shadow-mapping.sampling.point
#include regen.shading.shadow-mapping.sampling.spot
