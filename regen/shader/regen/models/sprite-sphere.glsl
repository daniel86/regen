
--------------------------------
--------------------------------
----- A sprite that fakes a sphere by calculating sphere normals and
----- discarding fragments outside the sphere radius.
--------------------------------
--------------------------------
-- vs
#include regen.models.mesh.defines

in vec3 in_pos;
#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#endif

in float in_sphereRadius;
out float out_sphereRadius;

#include regen.models.tf.transformModel
#include regen.layered.VS_SelectLayer

#define HANDLE_IO(i)

void main() {
    vec4 pos = transformModel(vec4(in_pos,1.0));
    gl_Position = pos;
    out_sphereRadius = in_sphereRadius;
#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID + gl_BaseInstance;
#endif // HAS_INSTANCES
    VS_SelectLayer(regen_RenderLayer());
    HANDLE_IO(gl_VertexID);
}
-- tcs
#include regen.models.mesh.tcs
-- tes
#include regen.models.mesh.tes
-- gs
#include regen.models.mesh.defines

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

#include regen.states.camera.input

out vec3 out_posWorld;
out vec3 out_posEye;
out vec2 out_texco0;

in float in_sphereRadius[1];
#ifdef DEPTH_CORRECT
out float out_sphereRadius;
#endif
#if RENDER_LAYER > 1
flat in int in_layer[ ];
flat out int out_layer;
#endif

#include regen.states.camera.transformEyeToScreen
#include regen.states.camera.transformEyeToWorld
#include regen.states.camera.transformWorldToEye
#include regen.math.computeSpritePoints

#define HANDLE_IO(i)

void emitVertex(vec4 posEye, int layer) {
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
#ifdef DEPTH_CORRECT
    out_sphereRadius = in_sphereRadius[0];
#endif
    gl_Position = transformEyeToScreen(posEye,layer);
#if RENDER_LAYER > 1
    out_layer = layer;
    gl_Layer = layer;
#endif
    HANDLE_IO(0);
    EmitVertex();
}

void emitSpriteSphere(int layer) {
    vec4 centerEye = transformWorldToEye(gl_in[0].gl_Position,layer);
#ifdef HAS_VERTEX_sphereRadius
    float sphereRadius = in_sphereRadius[0];
#else
    float sphereRadius = in_sphereRadius;
#endif
    vec3 quadPos[4] = computeSpritePoints(
        centerEye.xyz, vec2(sphereRadius), vec3(0.0,1.0,0.0));

    out_texco0 = vec2(1.0,0.0);
    emitVertex(vec4(quadPos[0],1.0), layer);
    out_texco0 = vec2(1.0,1.0);
    emitVertex(vec4(quadPos[1],1.0), layer);
    out_texco0 = vec2(0.0,0.0);
    emitVertex(vec4(quadPos[2],1.0), layer);
    out_texco0 = vec2(0.0,1.0);
    emitVertex(vec4(quadPos[3],1.0), layer);
    EndPrimitive();
}

void main() {
    #if RENDER_LAYER > 1
    emitSpriteSphere(in_layer[0]);
    #else
    emitSpriteSphere(0);
    #endif
}

-- fs
#include regen.models.mesh.defines
#include regen.states.textures.defines
#include regen.models.mesh.fs-outputs
#include regen.layered.defines

in vec3 in_posWorld;
in vec3 in_posEye;
in vec2 in_texco0;
#ifdef DEPTH_CORRECT
in float in_sphereRadius;
#endif
#ifdef HAS_INSTANCES
flat in int in_instanceID;
#endif

#ifdef HAS_col
uniform vec4 in_col;
#endif

#include regen.states.camera.input
#include regen.states.material.input
#include regen.states.textures.input

#include regen.states.camera.transformEyeToWorld
#ifdef DEPTH_CORRECT
    #include regen.states.camera.depthCorrection
#endif
#include regen.states.textures.mapToFragment
#include regen.states.textures.mapToLight
#include regen.models.mesh.writeOutput

#ifdef DISCARD_EMISSION
    #ifndef DISCARD_EMISSION_THRESHOLD
#define DISCARD_EMISSION_THRESHOLD 0.01
    #endif
#endif

void main()
{
    vec2 spriteTexco = in_texco0*2.0 - vec2(1.0);
    float texcoMagnitude = length(spriteTexco);
    // 0.99 because color at edges was wrong.
    if(texcoMagnitude>=0.99) discard;

    vec3 normal = vec3(spriteTexco, sqrt(1.0 - dot(spriteTexco,spriteTexco)));
    vec4 norWorld = normalize(transformEyeToWorld(vec4(normal,0.0),in_layer));
#ifdef DEPTH_CORRECT
    // Note that early depth test is disabled with DEPTH_CORRECT defined and this can have
    // bad consequences for performance.
    depthCorrection(in_sphereRadius*(1.0-texcoMagnitude), in_layer);
#endif
#ifdef HAS_col
    vec4 color = in_col;
#else
    vec4 color = vec4(1.0);
#endif
    textureMappingFragment(in_posWorld,color,norWorld.xyz);
#ifdef HAS_alphaDiscardThreshold
    if (color.a < in_alphaDiscardThreshold) discard;
#endif
    writeOutput(in_posWorld,norWorld.xyz,color);
#ifdef DISCARD_EMISSION
    if (dot(out_emission,out_emission) < DISCARD_EMISSION_THRESHOLD) discard;
#endif
}

