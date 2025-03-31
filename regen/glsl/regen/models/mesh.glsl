
-- defines
#include regen.states.camera.defines
#include regen.states.textures.defines
#include regen.defines.all
#ifndef OUTPUT_TYPE
#define OUTPUT_TYPE DEFERRED
#endif

-- vs
#include regen.models.mesh.defines

in vec3 in_pos;
#ifdef HAS_nor
in vec3 in_nor;
#endif
#ifdef HAS_tan
in vec4 in_tan;
#endif
#ifdef HAS_TANGENT_SPACE
out vec3 out_tangent;
out vec3 out_binormal;
#endif
#ifdef VS_CAMERA_TRANSFORM
out vec3 out_posWorld;
out vec3 out_posEye;
#endif
#ifdef HAS_nor
out vec3 out_norWorld;
#endif
#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif

#include regen.states.camera.input
#include regen.states.textures.input

#include regen.states.model.transformModel
#ifdef VS_CAMERA_TRANSFORM
    #include regen.states.camera.transformWorldToEye
    #include regen.states.camera.transformEyeToScreen
#endif
#ifdef POS_MODEL_TRANSFER_KEY
    #include ${POS_MODEL_TRANSFER_KEY}
#endif
#ifdef POS_WORLD_TRANSFER_KEY
    #include ${POS_WORLD_TRANSFER_KEY}
#endif
#ifdef HAS_nor
    #ifdef NOR_MODEL_TRANSFER_KEY
        #include ${NOR_MODEL_TRANSFER_KEY}
    #endif
    #ifdef NOR_WORLD_TRANSFER_KEY
        #include ${NOR_WORLD_TRANSFER_KEY}
    #endif
#endif

#ifndef HAS_TESSELATION
    #include regen.states.textures.mapToVertex
#endif

#define HANDLE_IO(i)

void main() {
    vec3 posModel = in_pos.xyz;
#ifdef HAS_nor
    vec3 norModel = in_nor.xyz;
#endif
    // let custom functions modify position/normal in model space
#ifdef POS_MODEL_TRANSFER_NAME
    ${POS_MODEL_TRANSFER_NAME}(posModel);
#endif // POS_MODEL_TRANSFER_NAME
#ifdef NOR_MODEL_TRANSFER_NAME
    #ifdef HAS_nor
    ${NOR_MODEL_TRANSFER_NAME}(norModel);
    #endif
#endif // NOR_MODEL_TRANSFER_NAME
    // transform position and normal to world space
    vec4 posWorld = transformModel(vec4(posModel,1.0));
#ifdef HAS_nor
    vec3 norWorld = normalize(transformModel(norModel));
#else
    vec3 norWorld = vec3(0,1,0);
#endif
#ifndef HAS_TESSELATION
    // allow textures to modify position/normal
    textureMappingVertex(posWorld.xyz,norWorld,0);
#endif // HAS_TESSELATION

    // let custom functions modify position/normal in world space
#ifdef POS_WORLD_TRANSFER_NAME
    ${POS_WORLD_TRANSFER_NAME}(posModel);
#endif // POS_WORLD_TRANSFER_NAME
#ifdef NOR_WORLD_TRANSFER_NAME
    ${NOR_WORLD_TRANSFER_NAME}(norModel);
#endif // NOR_WORLD_TRANSFER_NAME

#ifdef HAS_nor
    out_norWorld = norWorld;
#endif
#ifdef VS_CAMERA_TRANSFORM
    vec4 posEye  = transformWorldToEye(posWorld,0);
    gl_Position  = transformEyeToScreen(posEye,0);
    out_posWorld = posWorld.xyz;
    out_posEye   = posEye.xyz;
#else
    gl_Position = posWorld;
#endif
#ifdef HAS_TANGENT_SPACE
    vec4 tanw = transformModel( vec4(in_tan.xyz,0.0) );
    out_tangent = normalize( tanw.xyz );
    out_binormal = normalize( cross(out_norWorld.xyz, out_tangent.xyz) * in_tan.w );
#endif

#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID;
#endif // HAS_INSTANCES

    HANDLE_IO(gl_VertexID);
}

-- tcs
#ifdef HAS_tessellation_shader
#ifdef TESS_IS_ADAPTIVE
#include regen.models.mesh.defines

layout(vertices=TESS_NUM_VERTICES) out;

#define ID gl_InvocationID

#include regen.states.camera.input
uniform vec2 in_viewport;

#define HANDLE_IO(i)

#include regen.stages.tesselation.tesselationControl

void main() {
    tesselationControl();
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
    HANDLE_IO(gl_InvocationID);
}
#endif
#endif

-- tes
#ifdef HAS_tessellation_shader
#ifdef HAS_TESSELATION
#include regen.models.mesh.defines
#ifndef TESS_SPACING
    #define TESS_SPACING equal_spacing
#endif

layout(triangles, TESS_SPACING) in;

#ifdef HAS_nor
in vec3 in_norWorld[ ];
#endif
#ifdef TES_CAMERA_TRANSFORM
out vec3 out_posWorld;
out vec3 out_posEye;
#endif
#ifdef HAS_nor
out vec3 out_norWorld;
#endif
#ifdef HAS_VERTEX_MASK_MAP
out float out_mask;
#endif

#include regen.states.camera.input
#include regen.states.textures.input

#include regen.stages.tesselation.interpolate

#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#include regen.states.textures.mapToVertex

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = INTERPOLATE_STRUCT(gl_in,gl_Position);
    // allow textures to modify texture/normal
#ifdef HAS_nor
    out_norWorld = INTERPOLATE_VALUE(in_norWorld);
    textureMappingVertex(posWorld.xyz, out_norWorld, 0);
#else
    vec3 norWorld = vec3(0,1,0);
    textureMappingVertex(posWorld.xyz, norWorld, 0);
#endif
  
#ifdef TES_CAMERA_TRANSFORM
    vec4 posEye = transformWorldToEye(posWorld,0);
    gl_Position = transformEyeToScreen(posEye,0);
    out_posWorld = posWorld.xyz;
    out_posEye = posEye.xyz;
#endif
    HANDLE_IO(0);
}
#endif // HAS_TESSELATION
#endif // HAS_tessellation_shader

-- gs
#include regen.states.camera.defines
#include regen.defines.all
#if RENDER_LAYER > 1
#define USE_GEOMETRY_SHADER
#endif

#ifdef USE_GEOMETRY_SHADER
#define2 GS_MAX_VERTICES ${${RENDER_LAYER}*3}
layout(triangles) in;
layout(triangle_strip, max_vertices=${GS_MAX_VERTICES}) out;

out vec3 out_posWorld;
out vec3 out_posEye;
#if RENDER_LAYER > 1
flat out int out_layer;
#endif

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#define HANDLE_IO(i)

#ifndef COMPUTE_LAYER_VISIBILITY
    #if RENDER_TARGET == INSTANCE_SELF || RENDER_TARGET == INSTANCE_OTHER
#define COMPUTE_LAYER_VISIBILITY
    #endif
    #if RENDER_TARGET == INSTANCE_SELF || RENDER_TARGET == INSTANCE_OTHER
#define COMPUTE_LAYER_VISIBILITY
    #endif
#endif
#include regen.layered.gs.computeVisibleLayers

void emitVertex(vec4 posWorld, int index, int layer) {
    vec4 posEye = transformWorldToEye(posWorld,layer);
    out_posWorld = posWorld.xyz;
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    HANDLE_IO(index);
    EmitVertex();
}

void emit(int layer) {
    #if RENDER_LAYER > 1
    gl_Layer = layer;
    out_layer = layer;
    #endif
    emitVertex(gl_in[0].gl_Position, 0, layer);
    emitVertex(gl_in[1].gl_Position, 1, layer);
    emitVertex(gl_in[2].gl_Position, 2, layer);
    EndPrimitive();
}

void main() {
#ifdef COMPUTE_LAYER_VISIBILITY
    bool visibleLayers[RENDER_LAYER];
    computeVisibleLayers(visibleLayers);
#endif
#for LAYER to ${RENDER_LAYER}
    #ifndef SKIP_LAYER${LAYER}
        #ifdef COMPUTE_LAYER_VISIBILITY
    if (visibleLayers[${LAYER}]) {
        #endif // COMPUTE_LAYER_VISIBILITY
        // select framebuffer layer
        emit(${LAYER});
        #ifdef COMPUTE_LAYER_VISIBILITY
    }
        #endif // COMPUTE_LAYER_VISIBILITY
    #endif // SKIP_LAYER
#endfor
}
#endif

-- fs-outputs
#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif
#if OUTPUT_TYPE == DEFERRED
///// Deferred fragment shading
#if SHADING==NONE
out vec4 out_diffuse;
#else
layout(location = 0) out vec4 out_diffuse;
layout(location = 1) out vec4 out_ambient;
layout(location = 2) out vec4 out_specular;
layout(location = 3) out vec4 out_norWorld;
#ifdef FBO_ATTACHMENT_emission
layout(location = 4) out vec3 out_emission;
#endif
#endif
#endif
#if OUTPUT_TYPE == TRANSPARENCY
///// Direct fragment shading
layout(location = 0) out vec4 out_color;
#ifdef USE_AVG_SUM_ALPHA
layout(location = 1) out vec2 out_counter;
#endif
#endif
#if OUTPUT_TYPE == DIRECT
///// Direct fragment shading
out vec4 out_color;
#endif
#if OUTPUT_TYPE == COLOR
///// Plain color fragment shading
out vec4 out_color;
#endif

-- applyBrightness
#ifdef HAS_brightness
void applyBrightness(inout Material mat) {
    mat.diffuse.rgb *= in_brightness;
    #ifdef HAS_MATERIAL
        #ifdef HAS_MATERIAL_EMISSION
    mat.emission *= in_brightness;
        #endif
    #endif
}
#else
#define applyBrightness(mat)
#endif

-- fs
#include regen.models.mesh.defines
#include regen.models.mesh.fs-outputs

#if RENDER_LAYER > 1
flat in int in_layer;
#else
#define in_layer 0
#endif
#ifdef HAS_INSTANCES
flat in int in_instanceID;
#endif
#ifdef HAS_brightness
in float in_brightness;
#endif

#ifdef DISCARD_ALPHA
    #ifndef DISCARD_ALPHA_THRESHOLD
#define DISCARD_ALPHA_THRESHOLD 0.01
    #endif
#endif

#if OUTPUT_TYPE == DEPTH
///// Depth only output
    #ifdef DISCARD_ALPHA
#define FS_NO_OUTPUT
#include regen.models.mesh.fs-shading
    #else
void main() {}
    #endif
#endif
#if OUTPUT_TYPE == BLACK
///// Output plain black
out vec4 out_color;
void main() {
  out_color = vec4(0.0,0.0,0.0,1.0);
}
#endif
#if OUTPUT_TYPE == WHITE
///// Output plain white
out vec4 out_color;
void main() {
  out_color = vec4(1.0);
}
#endif
#if OUTPUT_TYPE == DEFERRED
#include regen.models.mesh.fs-shading
#endif
#if OUTPUT_TYPE == TRANSPARENCY
#include regen.models.mesh.fs-shading
#endif
#if OUTPUT_TYPE == DIRECT
#include regen.models.mesh.fs-shading
#endif
#if OUTPUT_TYPE == COLOR
#include regen.models.mesh.fs-shading
#endif
#if OUTPUT_TYPE == MOMENTS
#include regen.models.mesh.fs-moments
#endif

-- fs-moments
out vec4 out_color;

#if RENDER_TARGET != 2D_ARRAY
#include regen.states.camera.linearizeDepth
#endif

void main()
{
    float depth = gl_FragDepth;
#if RENDER_TARGET == 2D_ARRAY
    // no need to linearize for ortho projection
#else
    // Perspective projection saves depth none linear.
    // Linearize it for shadow comparison.
    depth = clamp( linearizeDepth(2.0*depth-1.0, __NEAR__, __FAR__), 0.0, 1.0 );
#endif
    // Rate of depth change in texture space.
    // This will actually compare local depth with the depth calculated for
    // neighbor texels.
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    out_color = vec4(depth, depth*depth + 0.25*(dx*dx+dy*dy), 1.0, 1.0);
}

-- fs-shading
in vec3 in_posWorld;
in vec3 in_posEye;
#ifdef HAS_TANGENT_SPACE
in vec3 in_tangent;
in vec3 in_binormal;
#endif
#ifdef HAS_nor
in vec3 in_norWorld;
#endif

#ifdef HAS_col
uniform vec4 in_col;
#endif
#include regen.states.camera.input
#include regen.states.material.input
#include regen.states.clipping.input
#include regen.states.textures.input
#ifndef FS_NO_OUTPUT
#include regen.states.material.defines
#endif

#ifdef HAS_CLIPPING
#include regen.states.clipping.isClipped
#endif
#include regen.states.textures.mapToFragment
#ifndef FS_NO_OUTPUT
#include regen.states.textures.mapToLight
#include regen.models.mesh.writeOutput
#endif

#define HANDLE_IO(i)

void main() {
#ifdef HAS_CLIPPING
    if(isClipped(in_posWorld)) discard;
#endif
#if HAS_nor && HAS_TWO_SIDES
    vec3 norWorld = (gl_FrontFacing ? in_norWorld : -in_norWorld);
#elif HAS_nor
    vec3 norWorld = in_norWorld;
#else
    vec3 norWorld = vec3(0.0,0.0,0.0);
#endif
#ifdef HAS_col
    vec4 color = in_col;
#else
    #ifdef HAS_matDiffuse
    vec4 color = vec4(in_matDiffuse, 1.0);
    #else
    vec4 color = vec4(1.0);
    #endif
#endif 
#ifdef HAS_matAlpha
    color.a *= in_matAlpha;
#endif
#endif // HAS_COL
    textureMappingFragment(in_posWorld, color, norWorld);
#ifdef DISCARD_ALPHA
    if (color.a < DISCARD_ALPHA_THRESHOLD) discard;
#endif
#ifndef FS_NO_OUTPUT
    writeOutput(in_posWorld, norWorld, color);
#endif
}

-----------------------
-----------------------
-----------------------
-- writeOutput
#if OUTPUT_TYPE == DEFERRED
#include regen.models.mesh.writeOutput-deferred
#endif
#if OUTPUT_TYPE == TRANSPARENCY || OUTPUT_TYPE == DIRECT
#include regen.models.mesh.writeOutput-direct
#endif
#if OUTPUT_TYPE == DEPTH
#define writeOutput(posWorld,norWorld,color)
#endif
#if OUTPUT_TYPE == COLOR
#include regen.models.mesh.writeOutput-color
#endif

-- writeOutput-color
#include regen.models.mesh.applyBrightness
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
    Material mat;
    mat.ambient = vec3(0.0);
    mat.diffuse = color.rgb;
    mat.specular = vec3(0.0);
    mat.shininess = 0.0;
    applyBrightness(mat);
    textureMappingLight(posWorld, norWorld, mat);
    out_color.rgb = mat.diffuse;
    out_color.a = color.a;
}

-- writeOutput-direct
#if SHADING!=NONE
uniform vec3 in_ambientLight;
#include regen.shading.direct.shade
#endif
#include regen.models.mesh.applyBrightness
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
    Material mat;
    mat.occlusion = 0.0;
#if SHADING!=NONE
#ifdef HAS_MATERIAL
    mat.ambient = in_matAmbient;
    mat.diffuse = color.rgb;
    mat.specular = in_matSpecular;
    mat.shininess = in_matShininess;
#else
    mat.ambient = vec3(0.0);
    mat.diffuse = color.rgb;
    mat.specular = vec3(0.0);
    mat.shininess = 0.0;
#endif
    applyBrightness(mat);
    textureMappingLight(posWorld, norWorld, mat);

    Shading shading = shade(posWorld, norWorld, gl_FragCoord.z, mat.shininess);
    vec3 shadedColor =
        mat.diffuse*shading.diffuse.rgb +
        mat.specular*shading.specular.rgb +
        mat.ambient*in_ambientLight;
#endif
#ifdef USE_AVG_SUM_ALPHA
    out_color = vec4(shadedColor*color.a, color.a);
    out_counter = vec2(1.0);
#elif USE_SUM_ALPHA
    out_color = vec4(shadedColor*color.a, color.a);
#else
    out_color = vec4(shadedColor, color.a);
#endif
}

-- writeOutput-deferred
#if SHADING==NONE
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
    out_diffuse = color;
}
#else
#include regen.models.mesh.applyBrightness
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
    // TODO: only normalize when not using FLOAT textures!
    // map to [0,1] for rgba buffer
    out_norWorld.xyz = normalize(norWorld)*0.5 + vec3(0.5);
    out_norWorld.w = 1.0;

    Material mat;
    mat.occlusion = 0.0;
    mat.diffuse = color.rgb;
#ifdef HAS_MATERIAL
    mat.ambient = in_matAmbient;
    mat.specular = in_matSpecular;
    mat.shininess = in_matShininess;
    #ifdef HAS_MATERIAL_EMISSION
    #ifdef HAS_matEmission
    mat.emission = in_matEmission;
    #else
    mat.emission = vec3(0,0,0);
    #endif
    #endif
#else
    mat.ambient = vec3(0.0);
    mat.specular = vec3(0.0);
    mat.shininess = 0.0;
    #ifdef HAS_MATERIAL_EMISSION
    mat.emission = vec3(0,0,0);
    #endif
#endif // HAS_MATERIAL
    applyBrightness(mat);
    textureMappingLight(in_posWorld, norWorld, mat);

    out_ambient = vec4(mat.ambient,0.0);
    out_diffuse.rgb = mat.diffuse.rgb;
    out_diffuse.a = color.a;
    out_specular.rgb = mat.specular;
    // normalize shininess to [0,1]
    // TODO: only normalize when not using FLOAT textures!
    out_specular.a = clamp(mat.shininess/256.0, 0.0, 1.0);
    #ifdef FBO_ATTACHMENT_emission
    #ifdef HAS_MATERIAL_EMISSION
    out_emission = mat.emission;
    #else
    out_emission = vec3(0,0,0);
    #endif
    #endif
    // TODO: handle the occlusion value. It might be best to encode it in the g-buffer,
    //       then use this info in deferred shading.
    //out_norWorld.w = mat.occlusion;
}
#endif // SHADING!=NONE
