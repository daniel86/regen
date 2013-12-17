
-- draw.vs
#include regen.particles.utility.vs.passThrough

-- draw.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include regen.particles.utility.gs.inputs

out vec3 out_velocity;
out vec4 out_posEye;
out vec4 out_posWorld;
out vec2 out_spriteTexco;

#include regen.states.camera.input
uniform vec2 in_viewport;

#include regen.models.sprite.getSpritePoints
#include regen.models.sprite.emit2

void main() {
    if(in_lifetime[0]<=0) { return; }

    out_velocity = in_velocity[0];    
    
    vec4 centerEye = in_viewMatrix * vec4(in_pos[0],1.0);
    vec3 quadPos[4] = getSpritePoints(centerEye.xyz, vec2(in_size[0]), vec3(0.0, 1.0, 0.0));
    emitSprite(in_inverseViewMatrix, in_projectionMatrix, quadPos);
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
    opacity *= smoothstep(0.0, 5.0, distance(P, in_cameraPosition));
#endif
    // fade out based on texture intensity
    opacity *= texture(in_particleTexture, in_spriteTexco).x;
    if(opacity<0.0001) discard;
    
    vec3 diffuseColor = getDiffuseLight(P, gl_FragCoord.z);
    out_color = vec4(diffuseColor,1.0);
    out_color.rgb *= opacity; // opacity weighted color
}
