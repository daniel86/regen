
-- update.vs
const vec3 in_gravity = vec3(0.0,-9.81,0.0);

const vec2 in_rainDropMass = vec2(0.85,0.15);
const vec2 in_snowFlakeSize = vec2(0.5,0.15);
const vec3 in_cloudPosition = vec3(0.0,10.0,0.0);
const float in_cloudRadius = 10.0;

const float in_dampingFactor = 0.1;
const float in_noiseFactor = 0.1;

const int in_maxNumParticleEmits = 20;

uniform float in_deltaT;
uniform vec3 in_cameraPosition;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor

#include particles.randomize

bool isParticleDead()
{
    // XXX cfg
    return out_lifetime<=0.0 ||
        out_pos.y<-1.0 ||
        distance(in_cameraPosition,out_pos)>50.0;
}
        
bool updateParticle(float dt, inout uint seed)
{
    if(isParticleDead()) {
        return false;
    }
    out_lifetime += dt;
    out_velocity += in_mass * in_gravity * dt;
    out_velocity -= in_dampingFactor * out_velocity * dt;
    out_pos += out_velocity * dt;
    return true;
}

void emitParticle(float dt, inout uint seed)
{
    vec2 pos = variance(vec2(in_cloudRadius), seed);
    vec2 vel = vec2(
        in_noiseFactor*2.0*(random(seed)-0.5),
        in_noiseFactor*2.0*(random(seed)-0.5));
    float mass = in_rainDropMass.x + variance(in_rainDropMass.y, seed);
    float size = in_snowFlakeSize.x + variance(in_snowFlakeSize.y, seed);
    
    out_pos = in_cameraPosition;
    out_velocity = vec3(0.0);
    out_lifetime = dt;
    out_pos.y += in_cloudPosition.y;
    out_pos.xz += pos;
    out_velocity.xz += vel;
    out_mass = mass;
    out_size = size;
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

-- draw.vs
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

-- draw.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME}[1];
#endfor

out vec3 out_posWorld;
out vec2 out_spriteTexco;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
uniform vec2 in_viewport;

#include sky.sprite

void main() {
    if(in_lifetime[0]<=0) { return; }

    vec4 centerEye = in_viewMatrix * vec4(in_pos[0],1.0);
    vec3 quadPos[4] = getSpritePoints(centerEye.xyz, vec2(in_size[0]));

    out_spriteTexco = vec2(1.0,0.0);
    gl_Position = in_projectionMatrix * vec4(quadPos[0],1.0);
    EmitVertex();
    
    out_spriteTexco = vec2(1.0,1.0);
    gl_Position = in_projectionMatrix * vec4(quadPos[1],1.0);
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,0.0);
    gl_Position = in_projectionMatrix * vec4(quadPos[2],1.0);
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,1.0);
    gl_Position = in_projectionMatrix * vec4(quadPos[3],1.0);
    EmitVertex();
    
    EndPrimitive();
}

-- draw.fs
layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

const float in_softParticleScale = 1.0;

in vec2 in_spriteTexco;
uniform sampler2D in_snowFlakeTexture;

uniform vec2 in_viewport;
uniform float in_near;
uniform float in_far;
uniform sampler2D in_depthTexture;

float linearizeDepth(float expDepth)
{
    return (2 * in_near) / (in_far + in_near - expDepth * (in_far - in_near));
}
float softParticleFade()
{
    vec2 depthTexco = gl_FragCoord.xy/in_viewport.xy;
    float sceneDepth = linearizeDepth(texture(in_depthTexture, depthTexco).r);
    float fragmentDepth = linearizeDepth(gl_FragCoord.z);
    return clamp(in_softParticleScale*(sceneDepth - fragmentDepth), 0.0, 1.0);	
}

void main() {
    out_color.rgb = texture(in_snowFlakeTexture,in_spriteTexco).rgb;
    
    float softParticleFade = softParticleFade();	
    // for add blending
    out_color.rgb *= softParticleFade;
    // for alpha blending
    //out_color.a = length(out_color.rgb)*softParticleFade;
    
    out_posWorld = vec3(0.0);
    out_specular = vec4(0.0);
    out_norWorld = vec4(0.0);
}
