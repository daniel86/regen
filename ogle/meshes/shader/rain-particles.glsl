
-- update.vs
const vec3 in_gravity = vec3(0.0,-9.81,0.0);

const vec2 in_rainDropMass = vec2(0.75,0.25);
const vec3 in_rainCloudPosition = vec3(0.0,10.0,0.0);
const float in_rainCloudRadius = 10.0;

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
    return out_lifetime<=0.0 || out_pos.y<0.0;
}
        
bool updateParticle(float dt, inout uint seed)
{
    if(isParticleDead()) {
        return false;
    }
    out_lifetime += dt;
    out_velocity += in_noiseFactor * dt * random3(seed);
    out_velocity += in_mass * in_gravity * dt;
    out_velocity -= in_dampingFactor * dt * out_velocity;
    out_pos += out_velocity*dt;
    return true;
}

void emitParticle(float dt, inout uint seed)
{
    out_pos = in_cameraPosition;
    out_pos.y += in_rainCloudPosition.y;
    out_pos.xz += variance(vec2(in_rainCloudRadius), seed);
    out_velocity = vec3(0.0);
    out_lifetime = dt;
    out_mass = in_rainDropMass.x + variance(in_rainDropMass.y, seed);
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
    // init outputs to input values
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
}

-- draw.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=8) out;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME}[1];
#endfor

out vec3 out_posWorld;
out vec2 out_spriteTexco;
out float out_strandHeight;

const vec2 in_strandSize = vec2(0.05,0.1);

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

#include sky.sprite

void main() {
    if(in_lifetime[0]<=0) { return; }

    vec4 centerEye = in_viewMatrix * vec4(in_pos[0],1.0);
    vec2 spriteSize = vec2(in_strandSize.x,
        length( in_velocity[0]*in_strandSize.y ));
    
    // TODO: offset center pos, extrude in velocity direction
    vec3 quadPos[4] = getSpritePoints(centerEye.xyz, spriteSize);

    gl_Position = in_projectionMatrix * vec4(quadPos[0],1.0);
    EmitVertex();
    gl_Position = in_projectionMatrix * vec4(quadPos[1],1.0);
    EmitVertex();
    gl_Position = in_projectionMatrix * vec4(quadPos[2],1.0);
    EmitVertex();
    gl_Position = in_projectionMatrix * vec4(quadPos[3],1.0);
    EmitVertex();
    EndPrimitive();
}

-- draw.fs
layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

void main() {
    out_color = vec4(1.0);
    out_posWorld = vec3(0.0);
    out_specular = vec4(0.0);
    out_norWorld = vec4(0.0);
}
