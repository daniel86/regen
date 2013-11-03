
-- update.vs
// planet gravity
const vec3 in_gravity = vec3(0.0,-9.81,0.0);
// damping reduces velocity each frame
const float in_dampingFactor = 0.1;
// xy-velocity noise for emitting
const float in_noiseFactor = 0.1;
// limit for born particles per frame
const int in_maxNumParticleEmits = 20;

// base mass + mass variance for each particle
const vec2 in_particleMass = vec2(0.75,0.25);
// base size + size variance for each particle
const vec2 in_particleSize = vec2(0.1,0.05);
// emitter position
const vec3 in_cloudPosition = vec3(0.0,10.0,0.0);
// radius for emitting
const float in_cloudRadius = 10.0;
// particles with y pos below die
const float in_surfaceHeight = 0.0;

const float in_deltaT = 0.1;
uniform vec3 in_cameraPosition;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

#include regen.particles.utility.randomize

bool isParticleDead()
{
    if(out_lifetime<=0.0) return true; // not born yet
    if(out_pos.y<in_surfaceHeight) return true; // below surface
#if CLOUD_POSITION_MODE == CAMERA_RELATIVE
    float d = distance(in_cameraPosition,out_pos);
    if(d > in_cloudRadius) return true; // far away
#endif
    return false;
}
        
bool updateParticle(float dt, inout uint seed)
{
    if(isParticleDead()) return false;
    out_lifetime += dt;
    out_velocity += in_gravity * (in_mass * dt);
    out_velocity -= out_velocity * (in_dampingFactor * dt);
    out_pos += out_velocity * dt;
    return true;
}

void emitParticle(float dt, inout uint seed)
{
    vec2 randomPos = vec2(
        in_cloudRadius*2.0*(random(seed)-0.5),
        in_cloudRadius*2.0*(random(seed)-0.5));
    randomPos = random(seed)*in_cloudRadius*normalize(randomPos);
    
#if CLOUD_POSITION_MODE == CAMERA_RELATIVE
    out_pos = in_cameraPosition;
    out_pos.y += in_cloudPosition.y;
#else
    out_pos = in_cloudPosition;
#endif
    out_pos.xz += randomPos;

    out_velocity = vec3(
        in_noiseFactor*2.0*(random(seed)-0.5),
        in_noiseFactor*2.0*(random(seed)-0.5),
        0.0);
    out_lifetime = dt;
    
    out_mass = in_particleMass.x + variance(in_particleMass.y, seed);
    out_size = in_particleSize.x + variance(in_particleSize.y, seed);
}

void main() {
    float dt = in_deltaT*0.001;
    int remainingEmits = in_maxNumParticleEmits;

    // init outputs to input values
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
    
    if(!updateParticle(dt,out_randomSeed) && remainingEmits>0) {
        emitParticle(dt,out_randomSeed);
        remainingEmits -= 1;
    }
}
