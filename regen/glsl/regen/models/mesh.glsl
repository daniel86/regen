
-- defines
#include regen.states.camera.defines
#include regen.states.textures.defines
#include regen.defines.all
#ifndef OUTPUT_TYPE
#define OUTPUT_TYPE DEFERRED
#endif
#ifdef HAS_alphaClipkMin || HAS_alphaClipkMax || HAS_alphaClipThreshold
    #ifndef HAS_ALPHA_CLIP_COEFFICIENTS
#define HAS_ALPHA_CLIP_COEFFICIENTS
    #endif
#endif
#ifndef IGNORE_MATERIAL
    #include regen.states.material.defines
    #ifdef HAS_MATERIAL
    #define USE_MATERIAL
    #endif
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
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#define in_layer regen_RenderLayer()
#endif
#include regen.layered.VS_SelectLayer

#ifndef HAS_TESSELATION
    #ifdef HAS_VERTEX_MASK_MAP
out float out_mask;
    #endif
#endif // HAS_TESSELATION

#include regen.states.textures.input

#define HANDLE_IO(i)

#include regen.states.camera.input

#include regen.models.tf.transformModel
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

void main() {
    int layer = regen_RenderLayer();
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
    textureMappingVertex(posWorld.xyz,norWorld,layer);
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
    vec4 posEye  = transformWorldToEye(posWorld,layer);
    gl_Position  = transformEyeToScreen(posEye,layer);
    out_posWorld = posWorld.xyz;
    out_posEye   = posEye.xyz;
#else
    gl_Position = posWorld;
#endif
#ifdef HAS_TANGENT_SPACE
    vec4 tanw = transformModel( vec4(in_tan.xyz,0.0) );
    out_tangent = normalize( tanw.xyz );
    out_binormal = normalize( cross(norWorld.xyz, out_tangent.xyz) * in_tan.w );
#endif
#ifdef HAS_CUSTOM_HANDLE_IO
    customHandleIO(posWorld.xyz, posEye.xyz, norWorld.xyz);
#endif
#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID + gl_BaseInstance;
#endif // HAS_INSTANCES
    VS_SelectLayer(layer);

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
    // toggle on GS for layered rendering if requested
    // (better use indirect rendering with gl_DrawID!)
    #ifdef USE_GS_LAYERED_RENDERING
        #ifndef USE_GEOMETRY_SHADER
#define USE_GEOMETRY_SHADER
        #endif
    #endif
    // toggle on GS for writing to gl_Layer
    #ifndef ARB_shader_viewport_layer_array
        #ifndef USE_GEOMETRY_SHADER
#define USE_GEOMETRY_SHADER
        #endif
    #endif
#endif

#ifdef USE_GEOMETRY_SHADER
layout(triangles) in;
#ifdef USE_GS_LAYERED_RENDERING
#define2 GS_MAX_VERTICES ${${RENDER_LAYER}*3}
layout(triangle_strip, max_vertices=${GS_MAX_VERTICES}) out;
#else
layout(triangle_strip, max_vertices=3) out;
#endif

out vec3 out_posWorld;
out vec3 out_posEye;
#if RENDER_LAYER > 1 && USE_GS_LAYERED_RENDERING
flat out int out_layer;
#endif

#include regen.states.camera.input
#ifdef HAS_GS_TRANSFORM
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#endif

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, int index, int layer) {
#ifdef HAS_GS_TRANSFORM
    vec4 posEye = transformWorldToEye(posWorld,layer);
    out_posWorld = posWorld.xyz;
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
#else
    gl_Position = posWorld;
#endif
    HANDLE_IO(index);
    EmitVertex();
}

void emit(int layer) {
    emitVertex(gl_in[0].gl_Position, 0, layer);
    emitVertex(gl_in[1].gl_Position, 1, layer);
    emitVertex(gl_in[2].gl_Position, 2, layer);
    EndPrimitive();
}

void main() {
#ifdef USE_GS_LAYERED_RENDERING
    #for LAYER to ${RENDER_LAYER}
    #ifndef SKIP_LAYER${LAYER}
    #if RENDER_LAYER > 1
    gl_Layer = ${LAYER};
    #endif
    #ifdef USE_GS_LAYERED_RENDERING
    out_layer = ${LAYER};
    #endif
    emit(${LAYER});
    #endif // SKIP_LAYER
    #endfor
#else
    #if RENDER_LAYER > 1
    gl_Layer = in_layer[0];
    #ifdef USE_GS_LAYERED_RENDERING
    out_layer = in_layer[0];
    #endif
    emit(in_layer[0]);
    #else
    #ifdef USE_GS_LAYERED_RENDERING
    out_layer = 0;
    #endif
    emit(0);
    #endif
#endif
}
#endif

-- fs-outputs
#ifdef FS_EARLY_FRAGMENT_TEST
layout(early_fragment_tests) in;
#endif
#if OUTPUT_TYPE != DEPTH
layout(location = 0) out vec4 out_color;
#endif
#if OUTPUT_TYPE != DEPTH && OUTPUT_TYPE != BLACK && OUTPUT_TYPE != WHITE
    #ifdef HAS_ATTACHMENT_ambient
layout(location = ATTACHMENT_IDX_ambient) out vec4 out_ambient;
    #endif
    #ifdef HAS_ATTACHMENT_specular
layout(location = ATTACHMENT_IDX_specular) out vec4 out_specular;
    #endif
    #ifdef HAS_ATTACHMENT_normal
layout(location = ATTACHMENT_IDX_normal) out vec4 out_normal;
    #endif
    #ifdef HAS_ATTACHMENT_emission
layout(location = ATTACHMENT_IDX_emission) out vec3 out_emission;
    #endif
    #ifdef HAS_ATTACHMENT_counter
layout(location = ATTACHMENT_IDX_counter) out vec2 out_counter;
    #endif
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

-- clipAlpha
#ifdef HAS_ALPHA_CLIP_COEFFICIENTS
void clipAlpha(inout vec4 color) {
#ifdef HAS_alphaClipkMin
    #ifdef HAS_alphaClipkMax
    color.a = smoothstep(in_alphaClipMin, in_alphaClipMax, color.a);
    #else
    color.a = step(in_alphaClipMin, color.a);
    #endif
#else
    #ifdef HAS_alphaClipMax
    color.a = step(in_alphaClipMax, color.a);
    #endif
#endif
#ifdef HAS_alphaClipThreshold
    color.a = step(in_alphaClip, color.a);
#endif
}
#else
#define clipAlpha(color)
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

#if OUTPUT_TYPE == DEPTH
///// Depth only output
    #ifdef HAS_alphaDiscardThreshold
#define FS_NO_OUTPUT
#include regen.models.mesh.fs-shading
    #else
layout(early_fragment_tests) in;
void main() {
    // enforce depth writing. This seems redundant, but is necessary
    // for some drivers (i.e. NVIDIA) that may prune the fragment shader entirely
    // as a form of optimization. Without this, I experienced square-pattern noise
    // in the FBO used for depth only writing.
    gl_FragDepth = gl_FragCoord.z;
}
    #endif
#endif
#if OUTPUT_TYPE == BLACK
///// Output plain black
void main() {
    out_color = vec4(0.0,0.0,0.0,1.0);
}
#endif
#if OUTPUT_TYPE == WHITE
///// Output plain white
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
#if RENDER_TARGET != 2D_ARRAY
#include regen.states.camera.linearizeDepth
#endif

void main() {
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
    #if HAS_flat_tangent
flat in vec3 in_tangent;
    #else
in vec3 in_tangent;
    #endif
    #if HAS_flat_binormal
flat in vec3 in_binormal;
    #else
in vec3 in_binormal;
    #endif
#endif
#ifdef HAS_nor
    #ifdef HAS_flat_nor
flat in vec3 in_norWorld;
    #else
in vec3 in_norWorld;
    #endif
#endif

#ifdef HAS_col
uniform vec4 in_col;
#endif
#include regen.states.camera.input
#include regen.states.material.input
#include regen.states.clipping.input
#include regen.states.textures.input

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
    #ifdef USE_MATERIAL
    vec4 color = vec4(in_matDiffuse, 1.0);
    #else
    vec4 color = vec4(1.0);
    #endif
#endif 
#ifdef HAS_matAlpha
    #ifdef USE_MATERIAL
    color.a *= in_matAlpha;
    #endif
#endif
#endif // HAS_COL
#ifdef HAS_CUSTOM_FRAGMENT_MAPPING
    customFragmentMapping(in_posWorld, color, norWorld);
#else
    textureMappingFragment(in_posWorld, color, norWorld);
#endif
#ifdef HAS_alphaDiscardThreshold
    if (color.a < in_alphaDiscardThreshold) discard;
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
#ifdef HAS_ALPHA_CLIP_COEFFICIENTS
#include regen.models.mesh.clipAlpha
#endif
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
    Material mat;
    mat.ambient = vec3(0.0);
    mat.diffuse = color.rgb;
    mat.specular = vec3(0.0);
    mat.shininess = 0.0;
    applyBrightness(mat);
    textureMappingLight(posWorld, norWorld, mat);
    out_color.rgb = mat.diffuse;
#ifdef HAS_ALPHA_CLIP_COEFFICIENTS
	clipAlpha(color);
#endif
    out_color.a = color.a;
}

-- writeOutput-direct
#if SHADING!=NONE
uniform vec3 in_ambientLight;
#include regen.shading.direct.shade
#endif
#include regen.models.mesh.applyBrightness
#ifdef HAS_ALPHA_CLIP_COEFFICIENTS
#include regen.models.mesh.clipAlpha
#endif
#ifdef HAS_fogDistance
#include regen.weather.fog.applyFogToColor
#endif
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
    Material mat;
    mat.occlusion = 0.0;
#ifdef USE_MATERIAL
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
#ifdef HAS_ALPHA_CLIP_COEFFICIENTS
    #ifdef HAS_fogDistance
    shadedColor = applyFogToColor(shadedColor, gl_FragCoord.z, posWorld);
    #endif
	clipAlpha(color);
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
    out_color = color;
}
#else
#include regen.models.mesh.applyBrightness
#ifdef HAS_ALPHA_CLIP_COEFFICIENTS
#include regen.models.mesh.clipAlpha
#endif
#ifdef USE_EYESPACE_NORMAL
#include regen.states.camera.transformWorldToEye
#endif
void writeOutput(vec3 posWorld, vec3 norWorld, vec4 color) {
    #ifdef HAS_ATTACHMENT_normal
    // TODO: only normalize when not using FLOAT textures!
    // map to [0,1] for rgba buffer
        #ifdef USE_EYESPACE_NORMAL
    // TODO: rather transform in VS/GS
    vec3 norEye = transformWorldToEye(vec4(norWorld,0),in_layer).xyz;
    out_normal.xyz = normalize(norEye)*0.5 + vec3(0.5);
        #else
    out_normal.xyz = normalize(norWorld)*0.5 + vec3(0.5);
        #endif
    out_normal.w = 1.0;
    #endif

    Material mat;
    mat.occlusion = 0.0;
    mat.diffuse = color.rgb;
#ifdef USE_MATERIAL
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
#endif // USE_MATERIAL
    applyBrightness(mat);
    textureMappingLight(in_posWorld, norWorld, mat);

    out_color.rgb = mat.diffuse.rgb;
#ifdef HAS_ALPHA_CLIP_COEFFICIENTS
    clipAlpha(color);
#endif
    out_color.a = color.a;
#ifdef HAS_ATTACHMENT_ambient
    out_ambient = vec4(mat.ambient,0.0);
#endif
#ifdef HAS_ATTACHMENT_specular
    out_specular.rgb = mat.specular;
    // normalize shininess to [0,1]
    // TODO: only normalize when not using FLOAT textures!
    out_specular.a = clamp(mat.shininess/256.0, 0.0, 1.0);
#endif
#ifdef HAS_ATTACHMENT_emission
    #ifdef HAS_MATERIAL_EMISSION
    out_emission = mat.emission;
    #else
    out_emission = vec3(0,0,0);
    #endif
#endif
    // TODO: handle the occlusion value. It might be best to encode it in the g-buffer,
    //       then use this info in deferred shading.
    //out_normal.w = mat.occlusion;
}
#endif // SHADING!=NONE
