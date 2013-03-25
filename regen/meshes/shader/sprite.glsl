
-- getSpritePoints
vec3[4] getSpritePoints(vec3 p, vec2 size, vec3 upVector)
{
    vec3 zAxis = normalize(-p);
    vec3 xAxis = normalize(cross(zAxis, upVector));
    vec3 yAxis = normalize(cross(xAxis, zAxis));
    vec3 x = xAxis*0.5*size.x;
    vec3 y = yAxis*0.5*size.y;
    return vec3[](
        p - x - y,
        p - x + y,
        p + x - y,
        p + x + y
    );
}

-- getSpriteLayer
// Calculate cube map layers a sprite could affect.
// Should be done because a sprite can intersect with 3 cube faces at the same time.
int[3] getSpriteLayer(vec3 p)
{
    return int[](
        1 - int(sign(p.x)*0.5 + 0.5), //0 or 1
        3 - int(sign(p.y)*0.5 + 0.5), //2 or 3
        5 - int(sign(p.z)*0.5 + 0.5)  //4 or 5
    );
}

-- emit0
void emitSprite(mat4 proj, vec3 quadPos[4])
{
    out_spriteTexco = vec2(1.0,0.0);
    gl_Position = proj*vec4(quadPos[0],1.0);
    EmitVertex();

    out_spriteTexco = vec2(1.0,1.0);
    gl_Position = proj*vec4(quadPos[1],1.0);
    EmitVertex();

    out_spriteTexco = vec2(0.0,0.0);
    gl_Position = proj*vec4(quadPos[2],1.0);
    EmitVertex();

    out_spriteTexco = vec2(0.0,1.0);
    gl_Position = proj*vec4(quadPos[3],1.0);
    EmitVertex();
    EndPrimitive();
}

-- emit1
void emitSprite(mat4 proj, vec3 quadPos[4])
{
    out_spriteTexco = vec2(1.0,0.0);
    out_posEye = vec4(quadPos[0],1.0);
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(1.0,1.0);
    out_posEye = vec4(quadPos[1],1.0);
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,0.0);
    out_posEye = vec4(quadPos[2],1.0);
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(0.0,1.0);
    out_posEye = vec4(quadPos[3],1.0);
    gl_Position = proj * out_posEye;
    EmitVertex();
    
    EndPrimitive();
}

-- emit2
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

--------------------------------
--------------------------------
----- A sprite that fakes a sphere by calculating sphere normals and
----- discarding fragments outside the sphere radius.
--------------------------------
--------------------------------
-- sphere.vs
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
-- sphere.gs
#extension GL_EXT_geometry_shader4 : enable

layout(points) in;
layout(triangle_strip, max_vertices=8) out;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
uniform mat4 in_inverseViewMatrix;

out vec3 out_posWorld;
out vec3 out_posEye;
out vec2 out_spriteTexco;

in float in_sphereRadius[1];

#include sprite.getSpritePoints

void main() {
    vec3 centerWorld = gl_PositionIn[0].xyz;
    vec4 centerEye = in_viewMatrix * vec4(centerWorld,1.0);
    vec3 quadPos[4] = getSpritePoints(
        centerEye.xyz, vec2(in_sphereRadius[0]), vec3(0.0,1.0,0.0));

    out_spriteTexco = vec2(1.0,0.0);
    vec4 posEye = vec4(quadPos[0],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = (in_inverseViewMatrix * posEye).xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
    
    out_spriteTexco = vec2(1.0,1.0);
    posEye = vec4(quadPos[1],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = (in_inverseViewMatrix * posEye).xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
        
    out_spriteTexco = vec2(0.0,0.0);
    posEye = vec4(quadPos[2],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = (in_inverseViewMatrix * posEye).xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
        
    out_spriteTexco = vec2(0.0,1.0);
    posEye = vec4(quadPos[3],1.0);
    out_posEye = posEye.xyz;
    out_posWorld = (in_inverseViewMatrix * posEye).xyz;
    gl_Position = in_projectionMatrix * posEye;
    EmitVertex();
    EndPrimitive();
}
-- sphere.fs
#include mesh.defines
#include textures.defines

#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif

#if SHADING==NONE
out vec4 out_diffuse;
#else
layout(location = 0) out vec4 out_diffuse;
layout(location = 1) out vec4 out_specular;
layout(location = 2) out vec4 out_norWorld;
#endif

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
uniform mat4 in_inverseViewMatrix;

in vec3 in_posWorld;
in vec3 in_posEye;
in vec2 in_spriteTexco;

#ifdef HAS_col
uniform vec4 in_col;
#endif
uniform vec3 in_cameraPosition;

#include material.declaration
#include textures.input

#include textures.mapToFragment
#include textures.mapToLight

void main()
{
    vec2 spriteTexco = in_spriteTexco*2.0 - vec2(1.0);
    // 0.99 because color at edges was wrong.
    if(length(spriteTexco)>=0.99) discard;
    
    // TODO: Support depth correction aka. write to gl_FragDepth.
    // Note that early depth test is disabled then and this can have
    // bad consequences for performance.
    vec3 normal = vec3(spriteTexco, sqrt(1.0 - dot(spriteTexco,spriteTexco)));
    vec4 norWorld = normalize(in_inverseViewMatrix * vec4(normal,0.0));

#ifdef HAS_col
    out_diffuse = in_col;
#else
    out_diffuse = vec4(1.0);
#endif
    textureMappingFragment(in_posWorld, out_diffuse, norWorld.xyz);
    
#if SHADING!=NONE
    // map to [0,1] for rgba buffer
    out_norWorld.xyz = norWorld.xyz*0.5 + vec3(0.5);
    out_norWorld.w = 1.0;
  #ifdef HAS_MATERIAL
    out_diffuse.rgb *= (in_matAmbient + in_matDiffuse);
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
#endif
}

