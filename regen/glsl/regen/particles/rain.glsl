
-- draw.vs
#include regen.particles.utility.vs.passThrough

-- draw.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include regen.particles.utility.gs.inputs

out vec2 out_spriteTexco;
out vec3 out_velocity;
out vec4 out_posEye;
out vec4 out_posWorld;

const vec2 in_streakSize = vec2(0.05,0.1);

#include regen.states.camera.input
#include regen.meshes.sprite.getSpritePoints
#include regen.meshes.sprite.emit2

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

-- draw.fs
#extension GL_EXT_gpu_shader4 : enable

#include regen.particles.utility.fs.header

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
