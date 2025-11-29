
-- softParticleScale
#ifdef HAS_softParticleScale
// soft particles fade away where they intersect the scene
float softParticleScale()
{
    vec2 depthTexco = gl_FragCoord.xy/in_viewport.xy;
    float sceneDepth = linearizeDepth(
            texture(in_depthTexture, depthTexco).r,
            REGEN_CAM_NEAR_(in_layer), REGEN_CAM_FAR_(in_layer));
    float fragmentDepth = linearizeDepth(gl_FragCoord.z,
	    REGEN_CAM_NEAR_(in_layer), REGEN_CAM_FAR_(in_layer));
    return clamp(in_softParticleScale*(sceneDepth - fragmentDepth), 0.0, 1.0);
}
#else
#define softParticleScale()
#endif

-- vs
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

void main() {
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
}

-- gs
layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME}[1];
#endfor

out vec3 out_velocity;
out vec3 out_posEye;
out vec3 out_posWorld;
out vec2 out_texco0;
out float out_lifetime;
#ifdef HAS_color
out vec3 out_color;
#endif

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToWorld
#include regen.states.camera.transformEyeToScreen
uniform vec2 in_viewport;

#include regen.math.computeSpritePoints
#include regen.models.sprite.emitQuad_eye

void main() {
    if(in_lifetime[0]<=0) { return; }

    out_velocity = in_velocity[0];
    out_lifetime = in_lifetime[0];
#ifdef HAS_color
    out_color = in_color[0];
#endif
    
    vec4 centerEye = transformWorldToEye(vec4(in_pos[0],1.0),0);
    vec3 quadPos[4] = computeSpritePoints(centerEye.xyz, vec2(in_size[0]), vec3(0.0, 1.0, 0.0));
    emitQuad_eye(quadPos,0);
}

-- fs
#include regen.models.mesh.defines
#include regen.states.textures.defines

#ifdef HAS_DENSITY_OUTPUT
layout(location = 0) out float out_density;
#else
layout(location = 0) out vec4 out_color;
#endif

in vec3 in_posEye;
in vec3 in_posWorld;
in vec3 in_velocity;
in vec2 in_texco0;
in float in_lifetime;

#ifndef HAS_DENSITY_OUTPUT
    #ifdef HAS_color
in vec3 in_color;
    #endif
const float in_particleBrightness = 0.5;
#endif

const float in_lifetimeSmoothstep = 1.0;
#ifdef USE_NEAR_CAMERA_SOFT_PARTICLES
const float in_cameraSmoothstep = 5.0;
#endif

uniform vec4 in_cameraPosition;
uniform vec2 in_viewport;

#ifdef HAS_softParticleScale
#include regen.states.camera.input
uniform sampler2D in_depthTexture;
#endif

#ifdef HAS_softParticleScale
#include regen.states.camera.linearizeDepth
#include regen.particles.sprite.softParticleScale
const float in_softParticleScale = 1.0;
#endif

#ifndef HAS_DENSITY_OUTPUT
    #if ${NUM_LIGHTS} > 0
    #include regen.shading.direct.diffuse
    #endif
#endif
#ifdef HAS_FRAGMENT_TEXTURE
    #include regen.states.textures.mapToFragmentUnshaded
#endif
#ifdef HAS_fogDistance
    #include regen.weather.fog.fogIntensity
#endif

void main() {
    vec3 P = in_posWorld.xyz;
    vec3 N = vec3(0.0, 1.0, 0.0);
    //vec3 N = normalize(in_posWorld.xyz - in_cameraPosition.xyz);
    float opacity = 1.0;
    
#ifdef HAS_softParticleScale
    // fade out particles intersecting the world
    opacity *= softParticleScale();
#endif
#ifdef USE_NEAR_CAMERA_SOFT_PARTICLES
    // fade out particls near camera
    opacity *= smoothstep(0.0, in_cameraSmoothstep, distance(P, in_cameraPosition.xyz));
#endif
    // fade out based on lifetime
    opacity *= smoothstep(0.0, in_lifetimeSmoothstep, in_lifetime);
    if(opacity<0.1) discard;

#ifdef HAS_DENSITY_OUTPUT
    #ifdef HAS_FRAGMENT_TEXTURE
    vec4 color = vec4(1.0);
    textureMappingFragmentUnshaded(P, color);
    opacity *= color.a;
    #endif
    #ifdef HAS_fogDistance
    opacity *= fogIntensity(distance(P, in_cameraPosition.xyz));
    #endif
    out_density = opacity;
#else // HAS_DENSITY_OUTPUT
    #ifdef HAS_color
    out_color = vec4(in_color.rgb, 1.0);
    #elif HAS_particleColor
    out_color = vec4(in_particleColor.rgb, 1.0);
    #else
    out_color = vec4(1.0, 1.0, 1.0, 1.0);
    #endif
    #ifdef HAS_FRAGMENT_TEXTURE
    textureMappingFragmentUnshaded(P, out_color);
    #endif
    #if ${NUM_LIGHTS} > 0
    vec3 diffuseColor = getDiffuseLight(P, N, gl_FragCoord.z);
    out_color.rgb *= diffuseColor;
    #endif
    // apply the brightness of the particle
    // NOTE: This is not super useful with ADD blending, low values make the particle disappear
    //   so this is actually more of a density threshold than a brightness.
    //   not sure what the best strategy is here....
    out_color.rgb *= in_particleBrightness;
    out_color.a *= opacity;
    #ifdef OPACITY_WEIGHTED_COLOR
    out_color.rgb *= opacity; // opacity weighted color
    #endif
#endif // HAS_DENSITY_OUTPUT
}

-- density.vs
#include regen.particles.sprite.vs
-- density.gs
#include regen.particles.sprite.gs
-- density.fs
#define HAS_DENSITY_OUTPUT
#include regen.particles.sprite.fs

-- densityColor.vs
#include regen.filter.sampling.vs
-- densityColor.gs
#include regen.filter.sampling.gs
-- densityColor.fs
#include regen.states.camera.defines

out vec4 out_color;

uniform vec2 in_inverseViewport;
uniform sampler2D in_densityTexture;

const float in_gamma = 2.2;
const float in_exposure = 1.0;

#include regen.filter.sampling.computeTexco
#ifdef HAS_colorRamp
    #define2 _RAMP_ID ${TEX_ID_colorRamp}
    #include regen.states.textures.rampCoordinate
    #include ${TEX_BLEND_KEY${_RAMP_ID}}
    #ifdef TEX_TRANSFER_KEY${_RAMP_ID}
        #include ${TEX_TRANSFER_KEY${_RAMP_ID}}
    #endif
#endif

#include regen.colors.tonemap

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    // Sample the density texture. This gives a float value between 0 and MAX_FLOAT
    // created by particles rendered using ADD blending.
    // If we would use the density value directly, the result would be too bright.
    // Hence we apply a tonemapping function to make the density value more visually pleasing.
    float density = texture(in_densityTexture, computeTexco(texco_2D)).r;
    density = pow(tonemap(in_exposure * density), 1.0/in_gamma);
#ifdef DISCARD_DENSITY_THRESHOLD
    // Discard fragments with low density values
    if (density < DISCARD_DENSITY_THRESHOLD) discard;
#endif

#ifdef HAS_color
    vec4 color = in_color;
#else
    vec4 color = vec4(1.0);
#endif
#ifdef HAS_colorRamp
    #define2 _RAMP_ID ${TEX_ID_colorRamp}
    float ramped = rampCoordinate(1.0 - density, ${TEX_TEXEL_X${_RAMP_ID}}).x;
    vec4 rampColor = texture(in_colorRamp, ramped);
    #ifdef TEX_TRANSFER_NAME${_RAMP_ID}
    // use a custom transfer function for the texel
    ${TEX_TRANSFER_NAME${_RAMP_ID}}(rampColor);
    #endif
    ${TEX_BLEND_NAME${_RAMP_ID}}(rampColor, color, ${TEX_BLEND_FACTOR${_ID}});
#endif
    color.a *= density;
#ifdef HAS_brightness
    color.rgb *= in_brightness;
#endif
    out_color = color;
}
