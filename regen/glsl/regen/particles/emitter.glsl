
-- defines
#define M_PI 3.1415926535897932384626433832795
const float in_deltaT = 0.1;
// limit for born particles per frame
const int in_maxNumEmits = 20;
// emitter position
const vec3 in_emitterPosition = vec3(0.0, 10.0, 0.0);
// the size of the emitter shape
const float in_emitterSize = 20.0;
// the opening of the cone in which particles are emitted relative
// to their starting position on the emitter shape
const float in_emitterOpening = 1.0;
// damping reduces velocity each frame
const float in_dampingFactor = 0.1;
// xy-velocity noise for emitting
const float in_noiseFactor = 0.1;
// planet gravity
const vec3 in_gravity = vec3(0.0, -9.81, 0.0);
// base mass + mass variance for each particle
const vec2 in_massVariance = vec2(0.75, 0.25);
// base size + size variance for each particle
const vec2 in_sizeVariance = vec2(0.1, 0.05);
// particles with y pos below die
const float in_surfaceHeight = 0.0;

-- inputs
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

-- particleUpdate
#ifndef REGEN_particleUpdate_defined_
#define2 REGEN_particleUpdate_defined_

#for INDEX to NUM_PARTICLE_ATTRIBUTES
    #ifdef PARTICLE_ATTRIBUTE${INDEX}_ADVANCE_KEY
#include ${PARTICLE_ATTRIBUTE${INDEX}_ADVANCE_KEY}
    #endif
#endfor

void particleUpdateVelocity(float dt)
{
    vec3 velocityChange = vec3(0.0);
#ifdef HAS_mass
    velocityChange += in_gravity * in_mass;
#else
    #ifdef HAS_gravity
    velocityChange += in_gravity;
    #endif
#endif
#ifdef HAS_dampingFactor
    velocityChange -= out_velocity * in_dampingFactor;
#endif
    out_velocity += velocityChange * dt;
}

void particleUpdate(float dt, inout uint seed)
{
    // TODO: why scale time?
    float dt_ms = dt*0.001;

    out_lifetime -= dt_ms;
    particleUpdateVelocity(dt_ms);
    // allow attributes to configure their advance function
#for INDEX to NUM_PARTICLE_ATTRIBUTES
    #ifdef PARTICLE_ATTRIBUTE${INDEX}_ADVANCE
    #define2 _ADVANCE ${PARTICLE_ATTRIBUTE${INDEX}_ADVANCE}
    #define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    ${_ADVANCE}( out_${_NAME}, dt_ms, seed );
    #endif
#endfor
    out_pos += out_velocity * dt_ms;
}
#endif

-- particleOffset
#ifndef REGEN_particleOffset_defined_
#define2 REGEN_particleOffset_defined_
void particleOffset(inout uint seed, out vec3 pos, out vec3 dir)
{
#if ${PARTICLE_EMITTER_SHAPE} == DISC
    pos = (0.5*in_emitterSize*random(seed))*normalize(vec3(
                random(seed)-0.5,
                0.0,
                random(seed)-0.5));
    dir = vec3(0.0,1.0,0.0);
#else
#if ${PARTICLE_EMITTER_SHAPE} == CUBE
    // first choose a random face
    float face = random(seed)*6.0;
    // then choose a random position on that face
    vec3 randomPos = vec3(
                0.5*in_emitterSize*(random(seed)-0.5),
                0.5*in_emitterSize*(random(seed)-0.5),
                0.5*in_emitterSize);
    if(face<1.0) {
        pos = vec3(randomPos.x,randomPos.y,randomPos.z);
        dir = vec3(0.0,0.0,1.0);
    } else if(face<2.0) {
        pos = vec3(randomPos.x,randomPos.y,-randomPos.z);
        dir = vec3(0.0,0.0,-1.0);
    } else if(face<3.0) {
        pos = vec3(randomPos.x,randomPos.z,randomPos.y);
        dir = vec3(0.0,1.0,0.0);
    } else if(face<4.0) {
        pos = vec3(randomPos.x,-randomPos.z,randomPos.y);
        dir = vec3(0.0,-1.0,0.0);
    } else if(face<5.0) {
        pos = vec3(randomPos.z,randomPos.x,randomPos.y);
        dir = vec3(1.0,0.0,0.0);
    } else {
        pos = vec3(-randomPos.z,randomPos.x,randomPos.y);
        dir = vec3(-1.0,0.0,0.0);
    }
#else
#if ${PARTICLE_EMITTER_SHAPE} == SPHERE
    dir = normalize(vec3(random(seed)-0.5,random(seed)-0.5,random(seed)-0.5));
    pos = 0.5*in_emitterSize*dir;
#endif
#endif
#endif

#if ${PARTICLE_EMITTER_MODE} == INOUT
    dir = dir * (random(seed)>0.5 ? 1.0 : -1.0);
#endif
#if ${PARTICLE_EMITTER_MODE} == IN
    dir = -dir;
#endif
}
#endif

-- particlePosition
#ifndef REGEN_particlePosition_defined_
#define2 REGEN_particlePosition_defined_
void particlePosition(inout uint seed, inout vec3 pos)
{
    // apply the position of the emitter.
    // Note that this is done here rather in the draw function such that particles
    // cannot be dragged away in case emitter moves.
#if ${EMITTER_POSITION_MODE} == CAMERA_RELATIVE
    pos += in_cameraPosition;
#endif
#ifdef HAS_emitterPosition
    // the position is given in form of a uniform vec3 in world space.
    pos += in_emitterPosition;
#endif
#ifdef HAS_modelMatrix
    // the position is given in form of a uniform mat4 in model transformation.
    pos = (in_modelMatrix * vec4(pos,1.0)).xyz;
#endif
}
#endif

-- particleVelocity
#ifndef REGEN_particleVelocity_defined_
#define2 REGEN_particleVelocity_defined_
vec3 particleVelocity(inout uint seed, in vec3 dir)
{
    // Generate a random point within a circle in the XY plane
    float angle = 2.0 * M_PI * random(seed);
    float radius = in_emitterOpening * sqrt(random(seed));
    float x = radius * cos(angle);
    float y = radius * sin(angle);
    // Create a random offset vector in the XY plane
    vec3 offset = vec3(x, y, 0.0);
    // Find a vector perpendicular to dir
    vec3 perpDir = normalize(cross(dir, vec3(0.0, 0.0, 1.0)));
    if (length(perpDir) < 0.001) {
        perpDir = normalize(cross(dir, vec3(0.0, 1.0, 0.0)));
    }
    // Rotate the offset vector to be perpendicular to dir
    vec3 randomOffset = perpDir * offset.x + cross(perpDir, dir) * offset.y;
    // Add the random offset to the direction vector
#ifdef HAS_velocityFactorMinMax
    float velocityFactor = in_velocityFactorMinMax.x + random(seed) * (in_velocityFactorMinMax.y - in_velocityFactorMinMax.x);
#else
    #ifdef HAS_velocityFactor
    float velocityFactor = in_velocityFactor;
    #else
    float velocityFactor = 1.0;
    #endif
#endif
    return velocityFactor * normalize(dir + 2.0*randomOffset);
}
#endif

-- particleValue
#ifndef REGEN_particleValue_defined_
#define2 REGEN_particleValue_defined_
#include regen.noise.variance
float particleValue(float value, float maxVariance, inout uint seed)
{
    return value + variance(maxVariance, seed);
}
vec2 particleValue(vec2 value, vec2 maxVariance, inout uint seed)
{
    return value + variance(maxVariance, seed);
}
vec3 particleValue(vec3 value, vec3 maxVariance, inout uint seed)
{
    return value + variance(maxVariance, seed);
}
vec4 particleValue(vec4 value, vec4 maxVariance, inout uint seed)
{
    return value + variance(maxVariance, seed);
}
#endif

-- particleEmit
#ifndef REGEN_particleEmit_defined_
#define2 REGEN_particleEmit_defined_
#include regen.particles.emitter.particleValue
#include regen.particles.emitter.particleOffset
#include regen.particles.emitter.particlePosition
#include regen.particles.emitter.particleVelocity

void particleEmit(inout uint seed)
{
    vec3 direction;
    particleOffset(seed, out_pos, direction);
    out_velocity = particleVelocity(seed, direction);
    particlePosition(seed, out_pos);
    // set lifetime to default value, but can be randomized below!
    out_lifetime = 20.0;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#ifdef HAS_${_NAME}Variance
    #if ${_TYPE} == float || ${_TYPE} == int
    out_${_NAME} = particleValue(in_${_NAME}Variance.x, in_${_NAME}Variance.y, seed);
    #elif ${_TYPE} == vec2 || ${_TYPE} == ivec2
    out_${_NAME} = particleValue(in_${_NAME}Variance.xy, in_${_NAME}Variance.zw, seed);
    #else
    out_${_NAME} = particleValue(in_${_NAME}Default, in_${_NAME}Variance, seed);
    #endif
#elif HAS_${_NAME}Value
    out_${_NAME} = in_${_NAME}Value;
#endif
#endfor
}
#endif


-- vs
#include regen.particles.emitter.inputs
#include regen.particles.emitter.defines
uniform vec3 in_cameraPosition;

#include regen.particles.emitter.particleUpdate
#include regen.particles.emitter.particleEmit

bool isParticleDead()
{
    if(out_lifetime<=0.0) return true; // not born yet
#ifdef HAS_surfaceHeight
    if(out_pos.y<in_surfaceHeight) return true; // below surface
#endif
    return false;
}

void main() {
    // NOTE: maxNumEmits per frame cannot be handled here!
    // --> We could have a jumping window that moves each frame to emit particles
    //       only in this window by testing if vertex index lies within,
    //       but seems randomizing lifetime is enough for now.
    //int remainingEmits = in_maxNumEmits;

    // init outputs to input values
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
    if (isParticleDead()) {
        particleEmit(out_randomSeed);
    } else {
        particleUpdate(in_deltaT, out_randomSeed);
    }
}
