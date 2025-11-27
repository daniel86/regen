--------------------------------------
--------------------------------------
---- Rendering Water as a Post-process Effect.
---- @see http://www.gamedev.net/page/reference/index.html/_/technical/graphics-programming-and-theory/rendering-water-as-a-post-process-effect-r2642
--------------------------------------
--------------------------------------
-- defines
#ifndef NUM_HEIGHT_SAMPLES
#define NUM_HEIGHT_SAMPLES 4
#endif
#define BIAS_FACTOR ${1.0/${NUM_HEIGHT_SAMPLES}}

-- plane.vs
#define IS_WATER_PLANE
#include regen.models.mesh.defines
#ifndef IS_UNDER_WATER
    #define HAS_CUSTOM_HANDLE_IO
    #include regen.terrain.water.defines
    #include regen.terrain.water.io.under-water.vs
#endif
#include regen.models.mesh.vs
-- plane.gs
#define IS_WATER_PLANE
#include regen.models.mesh.gs
-- plane.fs
#define IS_WATER_PLANE
#include regen.terrain.water.fs

-- fullscreen.vs
#include regen.states.camera.defines
#ifndef IS_UNDER_WATER
    #define HAS_CUSTOM_HANDLE_IO
    #include regen.terrain.water.defines
    #include regen.terrain.water.io.under-water.vs
#endif
in vec3 in_pos;
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#endif
#include regen.layered.VS_SelectLayer

void main() {
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
    #ifndef IS_UNDER_WATER
    handleWaterIO();
    #endif
    VS_SelectLayer(regen_RenderLayer());
}
-- fullscreen.gs
#include regen.filter.sampling.gs
-- fullscreen.fs
#include regen.terrain.water.fs

-- io.under-water.vs
#ifndef WATER_IO_defined_
#define WATER_IO_defined_
out float out_heightBiasAmplitude;
out vec2 out_heightBiasWave;
out vec2 out_heightBiasWind;
out vec3 out_k_waterColor;
out vec3 out_k_deepWaterColor;
out float out_inv_visibility;
out vec3 out_inv_colorExtinction;
out float out_specularFactor;

#ifdef IS_WATER_PLANE
#include regen.states.camera.transformEyeToWorld
#endif

void handleWaterIO() {
    out_heightBiasAmplitude = BIAS_FACTOR * in_maxAmplitude;
    out_heightBiasWave = BIAS_FACTOR * in_waveScale;
    out_heightBiasWind = 0.03 * in_time * in_windDirection;

    float k = clamp(length(in_lightDiffuse_Sun) / in_sunScale, 0.0, 1.0);
    out_k_waterColor     = k * in_waterColor;
    out_k_deepWaterColor = k * in_deepWaterColor;
    out_inv_visibility = 1.0 / in_visibility;
    out_inv_colorExtinction = vec3(
        1.0 / in_colorExtinction.x,
        1.0 / in_colorExtinction.y,
        1.0 / in_colorExtinction.z);
    out_specularFactor = (in_shininess*1.8 + 0.2)
        * clamp(in_lightDirection_Sun.y,0.0,1.0);
}
void customHandleIO(vec3 posWorld, vec3 posEye, vec3 norWorld) {
    handleWaterIO();
    #ifdef IS_WATER_PLANE && HAS_planeSize
    // Clip the water plane to the map boundaries.
    float halfX = 0.5 * in_planeSize.x;
    float halfZ = 0.5 * in_planeSize.y;
    vec3 p = transformEyeToWorld(vec4(posEye, 1.0), 0).xyz;
    #ifdef HAS_mapCenter
    p -= in_mapCenter;
    #endif
    gl_ClipDistance[0] = halfX -  p.x; // right bound
    gl_ClipDistance[1] = halfX +  p.x; // left: positive if inside
    gl_ClipDistance[2] = halfZ -  p.z; // front bound
    gl_ClipDistance[3] = halfZ +  p.z; // back: positive if inside
    #endif
}
#endif // WATER_IO_defined_

-- fs
#include regen.states.camera.defines
#include regen.terrain.water.defines

out vec4 out_color;

#ifndef IS_UNDER_WATER
    #ifdef IS_WATER_PLANE
in vec3 in_posWorld;
    #endif
in float in_heightBiasAmplitude;
in vec2 in_heightBiasWave;
in vec2 in_heightBiasWind;
in vec3 in_k_waterColor;
in vec3 in_k_deepWaterColor;
in float in_inv_visibility;
in vec3 in_inv_colorExtinction;
in float in_specularFactor;
#endif

uniform sampler2D in_depthTexture;
uniform sampler2D in_refractionTexture;
uniform sampler2D in_reflectionTexture;
uniform sampler2D in_heightTexture;
#ifdef USE_RIPPLES
uniform sampler2D in_normalTexture;
#endif
#ifdef USE_FOAM
uniform sampler2D in_foamTexture;
#endif
uniform vec2 in_inverseViewport;
// camera input
#include regen.states.camera.input
uniform mat4 in_viewProjectionMatrix_Reflection;

const float in_heightTextureSize = 256.0;
const float in_time = 0.0;

const float in_surfaceHeight = 0.0;
const float in_normalScale = 3.0;
// A constant related to the index of refraction (IOR).
// It should be computed on the CPU and passed to the shader.
const float in_refractionConstant = 0.25;

// The smaller this value is, the more soft the transition between
// shore and water. If you want hard edges use very big value.
// Default is 1.0f.
const float in_shoreHardness = 0.1;

// Wind force in x and z axes.
const vec2 in_windDirection = vec2(0.0,-1.0);
const vec2 in_waveScale = vec2(0.005);
// Maximum waves amplitude
const float in_maxAmplitude = 1.5;

// This value modifies current fresnel term. If you want to weaken
// reflections use bigger value. If you want to empasize them use
// value smaller then 0. Default is 0.
const float in_refractionStrength = 0.0;
// Strength of displacement along normal.
const float in_reflectionDisplace = 30.0;

// Sun configuration
const vec3 in_lightDiffuse_Sun = vec3(1.0,1.0,1.0);
const vec3 in_lightDirection_Sun = vec3(0.0,-1.0,0.0);
const float in_sunScale = 3.0;
// Color of the water surface
const vec3 in_waterColor = vec3(0.0078,0.5176,0.7);
// Water shininess factor
const float in_shininess = 0.7;
// Color of the water depth
const vec3 in_deepWaterColor = vec3(0.0039,0.00196,0.145);
const vec3 in_colorExtinction = vec3(7.0,30.0,40.0);
// How fast will colours fade out. You can also think about this
// values as how clear water is. Therefore use smaller values (eg. 0.05)
// to have crystal clear water and bigger to achieve "muddy" water.
const float in_fadeSpeed = 0.1;
// Water transparency along eye vector.
const float in_visibility = 3.0;

#ifdef USE_FOAM
// Describes at what depth foam starts to fade out and
// at what it is completely invisible. The fird value is at
// what height foam for waves appear (+ waterLevel).
const vec3 in_foamExistence = vec3(0.45,4.35,1.5);
const float in_foamIntensity = 0.5;
const float in_foamHardness = 1.0;
#endif

// Modifies 4 sampled normals. Increase first values to have more
// smaller "waves" or last to have more bigger "waves"
const vec4 in_normalModifier = vec4(1.0,2.0,4.0,8.0);

#include regen.filter.sampling.computeTexco
#include regen.states.camera.transformTexcoToWorld
#include regen.states.textures.texco_planar_reflection
#include regen.math.mat3.inverse

// Schlick: F = F0 + (1-F0)*(1 - cosTheta)^5
float fresnelTerm(float n_dot_eye) {
    float x = 1.0 - clamp(n_dot_eye, 0.0, 1.0);
    float x2 = x*x;
    x2 = x2*x2;
    x2 = x2*x;
    float F0 = clamp(in_refractionConstant, 0.0, 1.0);
    return F0 + (1.0 - F0) * x2;
}

#ifdef HAS_planeSize
    #ifndef IS_WATER_PLANE
bool inBounds(vec2 p) {
    vec2 planeSizeHalf = 0.5*in_planeSize;
    return all(lessThan(-planeSizeHalf, p)) && all(lessThan(p, planeSizeHalf));
}
    #endif
#endif

#ifdef IS_UNDER_WATER
vec3 computeUnderWaterColor(vec3 position, float sceneDepth, vecTexco texco, vec3 outColor) {
    // Distance from camera to fragment (sceneDepth is 0..1)
    float viewDepth = sceneDepth / in_visibility;

    // Base attenuation (light absorption by water)
    vec3 absorption = exp(-in_colorExtinction * viewDepth * in_fadeSpeed);
    vec3 attenuated = outColor * absorption;

    // Depth-based fog (scattering and turbidity) ---
    float depthBelowSurface = in_surfaceHeight - position.y;
    float fogFactor = clamp(depthBelowSurface * 0.25 + viewDepth * 0.8, 0.0, 1.0);
    vec3 fogged = mix(attenuated, in_deepWaterColor, fogFactor);

    // Caustic shimmer (optional, if height or normal texture available)
    #ifdef USE_RIPPLES
    vec2 causticUV = texco + in_time * in_windDirection * 0.1;
    vec3 ripple = texture(in_normalTexture, causticUV * 2.0).rgb;
    float caustic = pow(ripple.g, 5.0) * 0.5; // bright specks
    fogged += in_lightDiffuse_Sun * caustic * 0.2;
    #endif

    // Simulate refractive distortion of the view
    #ifdef USE_RIPPLES
    vec2 distortion = (ripple.rg - 0.5) * 0.03;
    fogged = texture(in_refractionTexture, texco + distortion).rgb * absorption;
    fogged = mix(fogged, in_deepWaterColor, fogFactor * 0.3);
    #endif

    // Fade to darker tone near far plane
    float isAtFarPlane = step(0.9998, sceneDepth);
    fogged = mix(fogged, in_deepWaterColor * 0.5, 0.6 * isAtFarPlane);

    // Add subtle directional light falloff
    float sunFactor = clamp(dot(normalize(in_lightDirection_Sun), vec3(0.0, -1.0, 0.0)) * 0.5 + 0.5, 0.0, 1.0);
    fogged += in_lightDiffuse_Sun * sunFactor * 0.05;

    return fogged;
}
#endif

#ifndef IS_UNDER_WATER
vec3 computeNormal(vec2 uv) {
    float h = texture(in_heightTexture, uv).r;
    float hx = dFdx(h); // derivative in texture domain
    float hy = dFdy(h);
    // scale derivatives to world-space slope;
    return normalize(vec3(
        -hx * in_maxAmplitude,
        in_normalScale,
        -hy * in_maxAmplitude));
}

    #ifdef USE_RIPPLES
mat3 computeTangentFrame(in vec3 N, in vec3 P, in vec2 UV) {
    vec3 dp1 = dFdx(P);
    vec3 dp2 = dFdy(P);
    vec2 duv1 = dFdx(UV);
    vec2 duv2 = dFdy(UV);
    // solve the linear system
    mat3 M = mat3(dp1, dp2, cross(dp1, dp2));
    mat3 inverseM = matrixInverse(M);
    vec3 T = inverseM * vec3(duv1.x, duv2.x, 0.0);
    vec3 B = inverseM * vec3(duv1.y, duv2.y, 0.0);
    // construct tangent frame
    float maxLength = max(length(T), length(B));
    T = T / maxLength;
    B = B / maxLength;
    return mat3(T, B, N);
}
vec3 computeRippledNormal(vec3 pos, vec3 nor, vec3 eyeVecNorm, float t0, float t1) {
    vec2 uv = pos.xz * t0 + in_windDirection * in_time * t1;
    mat3 tangentFrame = computeTangentFrame(nor, eyeVecNorm, uv);
    return normalize(tangentFrame*(2.0 * texture(in_normalTexture,uv).xyz - 1.0));
}
    #endif

    #ifdef USE_RIPPLES
vec3 getSurfaceNormal(vec3 surfacePoint, vec3 eyeVecNorm, vec2 uv) {
    vec3 surfaceNormal = computeNormal(uv);
    vec3 n0 = computeRippledNormal(surfacePoint,surfaceNormal,eyeVecNorm,3.2,1.6);
    vec3 n1 = computeRippledNormal(surfacePoint,surfaceNormal,eyeVecNorm,1.6,0.8);
    vec3 n2 = computeRippledNormal(surfacePoint,surfaceNormal,eyeVecNorm,0.8,0.4);
    vec3 n3 = computeRippledNormal(surfacePoint,surfaceNormal,eyeVecNorm,0.4,0.2);
    return normalize(
        n0 * in_normalModifier.x +
        n1 * in_normalModifier.y +
        n2 * in_normalModifier.z +
        n3 * in_normalModifier.w);
}
    #else
#define getSurfaceNormal(surfacePoint,eyeVecNorm,uv) computeNormal(uv)
    #endif
#endif

#ifndef IS_UNDER_WATER
vec3 getReflectionColor(vec3 surfacePoint, vec3 surfaceNormal) {
    vec4 proj = in_viewProjectionMatrix_Reflection * vec4(surfacePoint,1.0);
    proj.x += in_reflectionDisplace * surfaceNormal.x;
    vec2 reflectionTexco = (proj.xy/proj.w + vec2(1.0))*0.5;
    return texture(in_reflectionTexture, computeTexco(reflectionTexco)).rgb;
    //return textureLod(in_reflectionTexture, computeTexco(reflectionTexco),
    //        clamp(l_eyeVec * 0.01, 0.0, 4.0)).rgb;
}

vec3 computeOverWaterColor(vec3 position, float sceneDepth, vec3 sceneColor, vecTexco texco) {
    vec3 cam = REGEN_CAM_POS_(in_layer);
    // Height of water intersecting with view ray.
    float level = in_surfaceHeight;
    // Vector from camera to sampled scene point
    vec3 eyeNorm = normalize(position - cam);
    vec3 eyeNormYInv = eyeNorm / eyeNorm.y;
    vec2 uv;

    #ifdef IS_WATER_PLANE
    vec3 surfacePoint = in_posWorld;
    #else
    vec3 surfacePoint = cam + eyeNormYInv * (level - cam.y);
    #endif

    // Sample height map a few times to find wave amplitude
    vec2 heightOffset = in_heightBiasWave * eyeNorm.xz + in_heightBiasWind;
    #for INDEX to ${NUM_HEIGHT_SAMPLES}
    uv = in_heightBiasWave * surfacePoint.xz + heightOffset;
    level += in_heightBiasAmplitude * texture(in_heightTexture, uv).r;
    surfacePoint = cam + eyeNormYInv * (level - cam.y);
    #endfor
    //surfacePoint.y -= (level - in_surfaceHeight);
    surfacePoint.y -= in_maxAmplitude;

    // Compute eye vector with corrected surface point
    eyeNorm = normalize(cam - surfacePoint);
    float depth1 = length(position - surfacePoint);
    float depth2 = surfacePoint.y - position.y;
    #ifdef USE_FOAM
    // XXX: HACK ALERT: Increase water depth to infinity if at far plane, Prevents "foam on horizon" issue
    // For best results, replace the "40.0" below with the highest value in the m_ColorExtinction vec3
    float isAtFarPlane = step(0.99998, sceneDepth) * 40.0;
    depth1 += isAtFarPlane;
    depth2 += isAtFarPlane;
    #endif
    float depthN = depth1 * in_fadeSpeed;

    // Compute the normal vector at the surface point
    vec3 surfaceNormal = getSurfaceNormal(surfacePoint, eyeNorm, uv);
    // Compute reflection color
    vec3 reflection = getReflectionColor(surfacePoint, surfaceNormal);
    #ifdef USE_REFRACTION
    // Compute refraction color
    texco.xy += vec2(sin(in_time*0.2 + 3.0 * abs(position.y)) * (in_waveScale.x * min(depth2, 1.0)));
    vec3 refraction = texture(in_refractionTexture, computeTexco(texco)).rgb;
    #else
    vec3 refraction = sceneColor;
    #endif
    // Compute the water color based on depth and color extinction
    vec3 color = mix(refraction, in_k_waterColor,
        clamp(depthN*in_inv_visibility, 0.0, 1.0));
    refraction = mix(color, in_k_deepWaterColor,
        clamp(depth2*in_inv_colorExtinction, 0.0, 1.0));
    // Compute the fresnel term and mix reflection and refraction
    float nde = dot(surfaceNormal, eyeNorm);
    float fresnel = fresnelTerm(nde);
    color = mix(refraction, reflection, fresnel);

    #ifdef USE_FOAM
    float foam = 0.0;
    vec2 foam0 = 0.05*(surfacePoint.xz + 0.1*eyeNorm.xz);
    vec2 foam1 = in_time*in_windDirection;
    uv       = foam0 + 0.05*foam1 + 0.005*sin(0.001*in_time + position.x);
    vec2 uv2 = foam0 +  0.1*foam1 + 0.005*sin(0.001*in_time + position.z);
    if(depth2 < in_foamExistence.x){
        foam = in_foamIntensity*(texture(in_foamTexture,uv).r + texture(in_foamTexture,uv2).r);
    }
    else if(depth2 < in_foamExistence.y){
        foam = mix(
            in_foamIntensity*(texture(in_foamTexture,uv).x + texture(in_foamTexture,uv2).x),
            0.0,
            (depth2 - in_foamExistence.x) / (in_foamExistence.y - in_foamExistence.x));
    }
    if(in_maxAmplitude - in_foamExistence.z > 0.0001){
        float p = clamp(
            (level - (in_surfaceHeight + in_foamExistence.z))/
            (in_maxAmplitude - in_foamExistence.z),
            0.0, 1.0);
        foam += in_foamIntensity*in_foamIntensity*p*0.3*(texture(in_foamTexture,uv).r + texture(in_foamTexture,uv2).r);
    }
    #endif
    #ifdef USE_SPECULAR
    float spec = clamp(dot(
            2.0*nde*surfaceNormal - eyeNorm,
            in_lightDirection_Sun) * 0.5 + 0.5, 0.0, 1.0);
    spec = (1.0-fresnel) * in_specularFactor * pow(spec,64.0);
    spec += 25.0 * spec * clamp(in_shininess - 0.05, 0.0, 1.0);
    #endif
    #ifdef USE_FOAM && USE_SPECULAR
    color += max(spec,foam)*in_lightDiffuse_Sun.rgb;
    #elif USE_FOAM
    color += foam*in_lightDiffuse_Sun.rgb;
    #elif USE_SPECULAR
    color += spec*in_lightDiffuse_Sun.rgb;
    #endif

    #ifdef USE_FOAM
    //color = mix(refraction, color, clamp(depth1 * in_foamHardness, 0.0, 1.0));
    #endif
    color = mix(refraction, color, clamp(depth1 * in_shoreHardness, 0.0, 1.0));

    #ifdef HAS_planeSize
        #ifndef IS_WATER_PLANE
    // check if surface point lies within boundaries on xz plane
    // note: water plane case uses clip planes instead.
    color.rgb = mix(sceneColor, color, float(inBounds(surfacePoint.xz)));
        #endif
    #endif
    return color;
}
#endif

void main() {
    // compute G-buffer texco for fragment
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    // sample depth at pixel location and compute the position in world space
    float depth = texture(in_depthTexture, texco).x;
    vec3 posWorldSpace = transformTexcoToWorld(texco_2D, depth, in_layer);
    // initialize output color to scene color
    vec3 outColor = texture(in_refractionTexture, texco).rgb;
#ifdef IS_UNDER_WATER
    outColor = computeUnderWaterColor(posWorldSpace, depth, texco, outColor);
#else
    #ifdef IS_WATER_PLANE
    outColor = computeOverWaterColor(posWorldSpace, depth, outColor, texco);
    #else
    if(posWorldSpace.y<in_surfaceHeight) {
        outColor = computeOverWaterColor(posWorldSpace, depth, outColor, texco);
    }
    #endif
#endif
    // set output color
    out_color = vec4(outColor, 1.0);
}
