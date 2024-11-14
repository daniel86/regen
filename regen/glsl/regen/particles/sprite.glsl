
-- softParticleScale
#ifdef USE_SOFT_PARTICLES
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
#include regen.models.sprite.emitSprite

void main() {
    if(in_lifetime[0]<=0) { return; }

    out_velocity = in_velocity[0];
    out_lifetime = in_lifetime[0];
#ifdef HAS_color
    out_color = in_color[0];
#endif
    
    vec4 centerEye = transformWorldToEye(vec4(in_pos[0],1.0),0);
    // TODO: consider supporting to stretch particles along their velocity.
    vec3 quadPos[4] = computeSpritePoints(centerEye.xyz, vec2(in_size[0]), vec3(0.0, 1.0, 0.0));
    emitSprite(quadPos,0);
}

-- fs
#include regen.models.mesh.defines
#include regen.states.textures.defines

layout(location = 0) out vec4 out_color;

in vec3 in_posEye;
in vec3 in_posWorld;
in vec3 in_velocity;
in vec2 in_texco0;
in float in_lifetime;
#ifdef HAS_color
in vec3 in_color;
#endif

const float in_particleBrightness = 0.5;
const float in_lifetimeSmoothstep = 1.0;
#ifdef USE_NEAR_CAMERA_SOFT_PARTICLES
const float in_cameraSmoothstep = 5.0;
#endif

uniform vec3 in_cameraPosition;
uniform vec2 in_viewport;

#ifdef USE_SOFT_PARTICLES
#include regen.states.camera.input
uniform sampler2D in_depthTexture;
#endif

#ifdef USE_SOFT_PARTICLES
#include regen.states.camera.linearizeDepth
#include regen.particles.sprite.softParticleScale
const float in_softParticleScale = 1.0;
#endif

#if ${NUM_LIGHTS} > 0
    #include regen.shading.direct.diffuse
#endif
#ifdef HAS_FRAGMENT_TEXTURE
    #include regen.states.textures.mapToFragmentUnshaded
#endif

void main() {
    vec3 P = in_posWorld.xyz;
    float opacity = 1.0;
    
#ifdef USE_SOFT_PARTICLES
    // fade out particles intersecting the world
    opacity *= softParticleScale();
#endif
#ifdef USE_NEAR_CAMERA_SOFT_PARTICLES
    // fade out particls near camera
    opacity *= smoothstep(0.0, in_cameraSmoothstep, distance(P, in_cameraPosition));
#endif
    // fade out based on lifetime
    opacity *= smoothstep(0.0, in_lifetimeSmoothstep, in_lifetime);
    if(opacity<0.0001) discard;

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
    vec3 diffuseColor = getDiffuseLight(P, gl_FragCoord.z);
    out_color.rgb *= diffuseColor;
#endif
    // apply the brightness of the particle
    out_color.rgb *= in_particleBrightness;
    out_color.a *= opacity;
#ifdef OPACITY_WEIGHTED_COLOR
    out_color.rgb *= opacity; // opacity weighted color
#endif
}
