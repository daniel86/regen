
-- update.vs

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor
in vec3 in_nor;
in vec3 in_pos;

const vec3 in_gravity = vec3(0.0,-9.81,0.0);
const vec2 in_hairLengthFuzzy = vec2(0.1,0.05);
const float in_hairMass = 0.1;
const float in_hairRigidity = 0.1;

uniform float in_deltaT;

#include particles.randomize

bool isParticleDead()
{
    return out_lifetime<=0.0;
}
        
bool updateParticle(float dt, inout uint seed)
{
    if(isParticleDead()) {
        return false;
    }
    
/*
    // approximated hair end position relative to hair root.
    // idle hair end
    vec3 hairEndIdle = in_hairLength*in_nor;
    vec3 hairEnd = hairEndIdle;
    // external forces
    hairEnd += 1000.0 * in_hairMass * in_gravity;// * dt;
    // hair internal forces
    vec3 internalForce = normalize(hairEndIdle-hairEnd);
    hairEnd += 0.0001 * in_hairRigidity * internalForce;// * dt;
    hairEnd = in_hairLength*normalize(hairEnd);
    
    // if we just connect root with end we may end up with longer
    // hair then expected. Next make sure the hair length stays constant.
    float phi = 0.5*length(in_hairLength*in_nor - hairEnd);
    // calculate quadratic bezier control and end point (start point is hair root).
    vec3 p1 = phi*in_nor;
    vec3 p2 = p1 + (in_hairLength - phi)*normalize(hairEnd - p1);
    // express points in world space
    out_controlPoint = in_pos + p1;
    out_endPoint = in_pos + p2;
*/

    out_controlPoint = in_pos + in_nor*in_hairLength;
    out_endPoint = out_controlPoint;
    out_endPoint += 0.01 * in_gravity;

    out_lifetime += dt;
    return true;
}

void emitParticle(float dt, inout uint seed)
{
    out_hairLength = in_hairLengthFuzzy.x + variance(in_hairLengthFuzzy.y,seed);
    out_controlPoint = in_pos + in_nor*out_hairLength;
    out_endPoint = out_controlPoint;
    out_lifetime = dt;
}

void main() {
    float dt = in_deltaT*0.001;

    // init outputs to input values
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
    
    if(!updateParticle(dt,out_randomSeed)) {
        emitParticle(dt,out_randomSeed);
    }
}

-- draw.vs
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME};
out ${_TYPE} out_${_NAME};
#endfor
in vec3 in_nor;
in vec3 in_pos;
out vec3 out_nor;
out vec3 out_pos;

void main() {
#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
    out_${_NAME} = in_${_NAME};
#endfor
    out_nor = in_nor;
    out_pos = in_pos;
}

-- draw.gs
#extension GL_EXT_geometry_shader4 : enable

#define NUM_HAIR_SAMPLES 10

layout(points) in;
layout(line_strip, max_vertices=NUM_HAIR_SAMPLES) out;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME}[1];
#endfor
in vec3 in_nor[1];
in vec3 in_pos[1];

const vec3 in_gravity = vec3(0.0,-9.81,0.0);
const float in_hairMass = 0.1;
const float in_hairRigidity = 0.1;

out vec3 out_posWorld;
out vec2 out_spriteTexco;

uniform mat4 in_modelMatrix;
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

#include sky.sprite

void emitHairSample(vec3 sampled) {
    vec4 posWorld = in_modelMatrix * vec4(sampled,1.0);
    vec4 posView = in_viewMatrix * posWorld;
    vec4 posScreen = in_projectionMatrix * posView;
    gl_Position = posScreen;
    EmitVertex();
}

void sampleHair(int numSamples) {
    float sampleStep = 1.0/float(numSamples);
    float sampleStepPow = pow(sampleStep,2);
    float lengthStep = sampleStep*in_hairLength[0];
    
    vec3 currentPosition = in_pos[0];
    vec3 currentVelocity = in_hairRigidity*in_nor[0];
    vec3 gravity = in_hairMass*in_gravity;
    
    for(float t=sampleStep; t<=1.0; t+=sampleStep)
    {
        emitHairSample(currentPosition);
        
        vec3 ds = lengthStep*normalize(
            gravity*sampleStepPow + currentVelocity*sampleStep);
        currentPosition += ds;
        currentVelocity += gravity*sampleStep;
    }
    EndPrimitive();
}

void main() {
    if(in_lifetime[0]<=0) { return; }
    sampleHair(NUM_HAIR_SAMPLES);
}

-- draw.fs
layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

void main() {
    out_color = vec4(1.0,1.0,1.0,1.0);
    out_posWorld = vec3(0.0);
    out_specular = vec4(0.0);
    out_norWorld = vec4(0.0);
}
