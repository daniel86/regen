
-- vs
#include regen.models.sprite-particles.vs

-- gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#for INDEX to NUM_PARTICLE_ATTRIBUTES
#define2 _TYPE ${PARTICLE_ATTRIBUTE${INDEX}_TYPE}
#define2 _NAME ${PARTICLE_ATTRIBUTE${INDEX}_NAME}
in ${_TYPE} in_${_NAME}[1];
#endfor

out vec2 out_spriteTexco;
out vec3 out_velocity;
out vec4 out_posEye;
out vec4 out_posWorld;

const vec2 in_streakSize = vec2(0.05,0.1);

#include regen.states.camera.input
#include regen.math.computeSpritePoints

void emitSprite(mat4 invView, mat4 proj, vec3 quadPos[4])
{
    out_spriteTexco = vec2(1.0,0.0);
    out_posEye = vec4(quadPos[0],1.0);
    out_posWorld = invView * out_posEye;
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(1.0,1.0);
    out_posEye = vec4(quadPos[1],1.0);
    out_posWorld = invView * out_posEye;
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,0.0);
    out_posEye = vec4(quadPos[2],1.0);
    out_posWorld = invView * out_posEye;
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,1.0);
    out_posEye = vec4(quadPos[3],1.0);
    out_posWorld = invView * out_posEye;
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    EndPrimitive();
}

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
    vec4 centerStartEye = __VIEW__(0) * p;
    vec4 centerEndEye = __VIEW__(0) * (p + streakLength*v);
    // sprite z vertex coordinates are the same for each point
    vec4 offset = vec4(0.5*streakWidth,0.0,0.0,0.0);
    
    out_spriteTexco = vec2(1.0,0.0);
    out_posEye = centerStartEye + offset;
    out_posWorld = __VIEW_INV__(0) * out_posEye;
    gl_Position = __PROJ__(0) * out_posEye;
    EmitVertex();

    out_spriteTexco = vec2(1.0,1.0);
    out_posEye = centerStartEye - offset;
    out_posWorld = __VIEW_INV__(0) * out_posEye;
    gl_Position = __PROJ__(0) * out_posEye;
    EmitVertex();

    out_spriteTexco = vec2(0.0,0.0);
    out_posEye = centerEndEye + offset;
    out_posWorld = __VIEW_INV__(0) * out_posEye;
    gl_Position = __PROJ__(0) * out_posEye;
    EmitVertex();

    out_spriteTexco = vec2(0.0,1.0);
    out_posEye = centerEndEye - offset;
    out_posWorld = __VIEW_INV__(0) * out_posEye;
    gl_Position = __PROJ__(0) * out_posEye;
    EmitVertex();

    EndPrimitive();
}

-- draw.fs
#extension GL_EXT_gpu_shader4 : enable
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
