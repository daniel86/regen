
--------------------------------
--------------------------------
----- Update step for particles emitted from a cloud.
--------------------------------
--------------------------------
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

uniform float in_deltaT;
uniform vec3 in_cameraPosition;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
layout( location=${INDEX} ) in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

#include particles.randomize

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


--------------------------------
--------------------------------
----- Rain falling from a cloud.
--------------------------------
--------------------------------
-- rain.draw.vs
#include particles.vs.passThrough

-- rain.draw.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include particles.gs.inputs

out vec2 out_spriteTexco;
out vec3 out_velocity;
out vec4 out_posEye;
out vec4 out_posWorld;

const vec2 in_streakSize = vec2(0.05,0.1);

uniform mat4 in_viewMatrix;
uniform mat4 in_inverseViewMatrix;
uniform mat4 in_projectionMatrix;

#include sprite.getSpritePoints
#include sprite.emit2

void main() {
    if(in_lifetime[0]<=0) { return; }


    vec4 v = vec4(in_velocity[0],0.0);
    vec4 vDirection = normalize(v);
    float vAmountFactor = 1.0;
    float vAmount = clamp(length(v)*vAmountFactor, 0.0, 1.0);
    vec4 p = vec4(in_pos[0],1.0);
    out_velocity = v.xyz;
    
    float streakLength = in_streakSize.y*0.01*in_size[0];
    float streakWidth = in_streakSize.x*in_size[0];
    
    // extrude point in velocity direction
    vec4 centerStartEye = in_viewMatrix * p;
    vec4 centerEndEye = in_viewMatrix * (p + streakLength*v);
    // sprite z vertex coordinates are the same for each point
    vec4 offset = vec4(0.5*streakWidth,0.0,0.0,0.0);
    
    out_spriteTexco = vec2(1.0,0.0);
    out_posEye = centerStartEye + offset;
    out_posWorld = in_inverseViewMatrix * out_posEye;
    gl_Position = in_projectionMatrix * out_posEye;
    EmitVertex();

    out_spriteTexco = vec2(1.0,1.0);
    out_posEye = centerStartEye - offset;
    out_posWorld = in_inverseViewMatrix * out_posEye;
    gl_Position = in_projectionMatrix * out_posEye;
    EmitVertex();

    out_spriteTexco = vec2(0.0,0.0);
    out_posEye = centerEndEye + offset;
    out_posWorld = in_inverseViewMatrix * out_posEye;
    gl_Position = in_projectionMatrix * out_posEye;
    EmitVertex();

    out_spriteTexco = vec2(0.0,1.0);
    out_posEye = centerEndEye - offset;
    out_posWorld = in_inverseViewMatrix * out_posEye;
    gl_Position = in_projectionMatrix * out_posEye;
    EmitVertex();

    EndPrimitive();
}

-- rain.draw.fs
#extension GL_EXT_gpu_shader4 : enable

#include particles.fs.header

void main() {
    vec3 P = in_posWorld.xyz;
    float opacity = in_particleBrightness;
#ifdef USE_SOFT_PARTICLES
    // fade out particles intersecting the world
    opacity *= softParticleOpacity();
#endif
#ifdef USE_NEAR_CAMERA_SOFT_PARTICLES
    // fade out particls near camera
    opacity *= smoothstep(0.0, 2.0, distance(P, in_cameraPosition));
#endif
#ifdef USE_PARTICLE_SAMPLER2D
    opacity *= texture(in_particleTexture, in_spriteTexco).x;
#endif
    if(opacity<0.0001) discard;
    vec3 diffuseColor = getDiffuseLight(P, gl_FragCoord.z);
    out_color = vec4(diffuseColor,1.0);
    out_color.rgb *= opacity; // opacity weighted color

    out_posWorld = vec3(0.0);
    out_specular = vec4(0.0);
    out_norWorld = vec4(0.0);
}

--------------------------------
--------------------------------
----- Snow falling from a cloud.
--------------------------------
--------------------------------
-- snow.draw.vs
#include particles.vs.passThrough

-- snow.draw.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include particles.gs.inputs

out vec3 out_velocity;
out vec4 out_posEye;
out vec4 out_posWorld;
out vec2 out_spriteTexco;

uniform mat4 in_viewMatrix;
uniform mat4 in_inverseViewMatrix;
uniform mat4 in_projectionMatrix;
uniform vec2 in_viewport;

#include sprite.getSpritePoints
#include sprite.emit2

void main() {
    if(in_lifetime[0]<=0) { return; }

    out_velocity = in_velocity[0];    
    
    vec4 centerEye = in_viewMatrix * vec4(in_pos[0],1.0);
    vec3 quadPos[4] = getSpritePoints(centerEye.xyz, vec2(in_size[0]), vec3(0.0, 1.0, 0.0));
    emitSprite(in_inverseViewMatrix, in_projectionMatrix, quadPos);
}

-- snow.draw.fs
#extension GL_EXT_gpu_shader4 : enable

#include particles.fs.header

void main() {
    vec3 P = in_posWorld.xyz;
    float opacity = in_particleBrightness;
    
#ifdef USE_SOFT_PARTICLES
    // fade out particles intersecting the world
    opacity *= softParticleOpacity();
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
    
    out_posWorld = vec3(0.0);
    out_specular = vec4(0.0);
    out_norWorld = vec4(0.0);
}

