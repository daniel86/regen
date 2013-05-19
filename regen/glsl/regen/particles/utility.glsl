
-- randomize
// return pseudorandom float with values between 0 and 1.
float random(inout uint seed) {
    seed = (seed * 1103515245u + 12345u);
    return float(seed) / 4294967296.0;
}

// return pseudorandom vec3 with values between -1 and 1.
vec3 random3(inout uint seed) {
    vec3 result;
    seed = (seed * 1103515245u + 12345u); result.x = float(seed);
    seed = (seed * 1103515245u + 12345u); result.y = float(seed);
    seed = (seed * 1103515245u + 12345u); result.z = float(seed);
    return (result / 2147483648.0) - vec3(1,1,1);
}

/////////

float variance(float v, inout uint seed) {
    return v*2.0*(random(seed)-0.5);
}
vec2 variance(vec2 v, inout uint seed) {
    return vec2(
        variance(v.x,seed),
        variance(v.y,seed));
}
vec3 variance(vec3 v, inout uint seed) {
    return vec3(
        variance(v.x,seed),
        variance(v.y,seed),
        variance(v.z,seed));
}
vec4 variance(vec4 v, inout uint seed) {
    return vec4(
        variance(v.x,seed),
        variance(v.y,seed),
        variance(v.z,seed),
        variance(v.w,seed));
}

-- vs.passThrough
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
layout( location=${INDEX} ) in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

void main() {
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
}

-- gs.inputs
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME}[1];
#endfor

-- fs.header
#include regen.meshes.mesh.defines

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

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
uniform float in_near;
uniform float in_far;
uniform sampler2D in_depthTexture;
#endif

#include regen.shading.direct.diffuse
#include regen.utility.utility.linearizeDepth

#ifdef USE_SOFT_PARTICLES
// soft particles fade away where they intersect the scene
float softParticleOpacity()
{
    vec2 depthTexco = gl_FragCoord.xy/in_viewport.xy;
    float sceneDepth = linearizeDepth(texture(in_depthTexture, depthTexco).r, in_near, in_far);
    float fragmentDepth = linearizeDepth(gl_FragCoord.z, in_near, in_far);
    return clamp(in_softParticleScale*(sceneDepth - fragmentDepth), 0.0, 1.0);	
}
#endif

