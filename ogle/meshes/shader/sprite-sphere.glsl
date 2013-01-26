
-- vs
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

in vec3 in_pos;

in float in_sphereRadius;
out float out_sphereRadius;

#define HANDLE_IO(i)

void main() {
#ifdef HAS_modelMatrix
    gl_Position = in_modelMatrix * vec4(in_pos,1.0);
#else
    gl_Position = vec4(in_pos,1.0);
#endif
    out_sphereRadius = in_sphereRadius;

    HANDLE_IO(gl_VertexID);
}


-- gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=8) out;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

out vec3 out_posWorld;
out vec3 out_posEye;
out vec2 out_spriteTexco;
flat out mat4 out_invView;

in float in_sphereRadius[1];

#include sky.sprite

void main() {
    vec3 centerWorld = gl_PositionIn[0].xyz;
    vec4 centerEye = in_viewMatrix * vec4(centerWorld,1.0);
    vec3 quadPos[4] = getSpritePoints(centerEye.xyz, in_sphereRadius[0]);
    out_invView = inverse(in_viewMatrix);

    out_spriteTexco = vec2(1.0,0.0);
    vec4 posEye = vec4(quadPos[0],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = (out_invView * posEye).xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(1.0,1.0);
    posEye = vec4(quadPos[1],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = (out_invView * posEye).xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
        
    out_spriteTexco = vec2(0.0,0.0);
    posEye = vec4(quadPos[2],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = (out_invView * posEye).xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
        
    out_spriteTexco = vec2(0.0,1.0);
    posEye = vec4(quadPos[3],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = (out_invView * posEye).xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
    EndPrimitive();
}

-- fs
#include mesh.defines
#include textures.defines

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
layout(location = 3) out vec3 out_posWorld;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;

in vec3 in_posWorld;
in vec3 in_posEye;
in vec2 in_spriteTexco;
flat in mat4 in_invView;

#ifdef HAS_col
uniform vec4 in_col;
#endif
uniform vec3 in_cameraPosition;
#include mesh.material
#include textures.input

#include textures.mapToFragment
#include textures.mapToLight

void main()
{
    vec2 spriteTexco = in_spriteTexco*2.0 - vec2(1.0);
    if(length(spriteTexco)>1.0) discard;

    vec3 normal = vec3(spriteTexco, sqrt(1.0 - dot(spriteTexco,spriteTexco)));
    vec4 norWorld = in_invView * vec4(normal,0.0);

#ifdef HAS_col
    out_color = in_col;
#else
    out_color = vec4(1.0);
#endif
    float alpha = 1.0; // XXX: no alpha
    textureMappingFragment(in_posWorld, norWorld.xyz, out_color, alpha);
    
    // map to [0,1] for rgba buffer
    out_norWorld.xyz = normalize(norWorld.xyz)*0.5 + vec3(0.5);
    out_norWorld.w = 1.0;
    out_posWorld = in_posWorld;
  #ifdef HAS_MATERIAL && SHADING!=NONE
    out_color.rgb *= (in_matAmbient + in_matDiffuse);
    out_specular = vec4(in_matSpecular,0.0);
    float shininess = in_matShininess;
  #else
    out_specular = vec4(0.0);
    float shininess = 0.0;
  #endif
    textureMappingLight(
        in_posWorld,
        norWorld.xyz,
        out_color.rgb,
        out_specular.rgb,
        shininess);
  #ifdef HAS_MATERIAL
    out_specular.a = (in_matShininess * shininess)/256.0;
  #else
    out_specular.a = shininess/256.0;
  #endif
}



