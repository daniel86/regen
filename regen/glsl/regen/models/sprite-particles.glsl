
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
out vec4 out_posEye;
out vec4 out_posWorld;
out vec2 out_spriteTexco;

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
    
    vec4 centerEye = transformWorldToEye(vec4(in_pos[0],1.0),0);
    vec3 quadPos[4] = computeSpritePoints(centerEye.xyz, vec2(in_size[0]), vec3(0.0, 1.0, 0.0));
    emitSprite(quadPos,0);
}

-- fs
#include regen.models.mesh.defines

layout(location = 0) out vec4 out_color;

in vec4 in_posEye;
in vec4 in_posWorld;
in vec3 in_velocity;
in vec2 in_spriteTexco;

const float in_softParticleScale = 1.0;
const float in_particleBrightness = 0.5;

uniform sampler2D in_particleTexture;

uniform vec3 in_cameraPosition;
uniform vec2 in_viewport;
#ifdef USE_SOFT_PARTICLES
#include regen.states.camera.input
uniform sampler2D in_depthTexture;
#endif

#include regen.shading.direct.diffuse
#include regen.states.camera.linearizeDepth
#include regen.models.sprite-particles.softParticleScale

void main() {
    vec3 P = in_posWorld.xyz;
    float opacity = in_particleBrightness;
    
#ifdef USE_SOFT_PARTICLES
    // fade out particles intersecting the world
    opacity *= softParticleScale();
#endif
#ifdef USE_NEAR_CAMERA_SOFT_PARTICLES
    // fade out particls near camera
    opacity *= smoothstep(0.0, 5.0, distance(P, in_cameraPosition));
#endif
    // fade out based on texture intensity
    opacity *= texture(in_particleTexture, in_spriteTexco).x;
    if(opacity<0.0001) discard;
    
    vec3 diffuseColor = getDiffuseLight(P, gl_FragCoord.z);
    out_color = vec4(diffuseColor,1.0);
    out_color.rgb *= opacity; // opacity weighted color
}
