
-- regen_InstanceID
#ifndef regen_InstanceID_defined_
#define2 regen_InstanceID_defined_

#ifdef HAS_LOD && HAS_INSTANCES
    #ifndef HAS_instanceIDMap
#error "HAS_LOD + HAS_INSTANCES defined but HAS_instanceIDMap not defined"
    #endif
#endif

#if SHADER_STAGE==fs
    #ifdef HAS_instanceIDMap
#define regen_InstanceID in_instanceIDMap[in_instanceID]
    #else // HAS_instanceIDMap
#define regen_InstanceID in_instanceID
    #endif // HAS_instanceIDMap
#elif SHADER_STAGE==gs
    #ifdef HAS_instanceIDMap
#define regen_InstanceID in_instanceIDMap[in_instanceID[0]]
    #else // HAS_instanceIDMap
#define regen_InstanceID in_instanceID[0]
    #endif // HAS_instanceIDMap
#else
    #ifdef HAS_instanceIDMap
#define regen_InstanceID in_instanceIDMap[gl_InstanceID + gl_BaseInstance]
    #else
#define regen_InstanceID (gl_InstanceID + gl_BaseInstance)
    #endif
#endif
#endif // regen_InstanceID_defined_

-- regen_RenderLayer
#ifndef regen_RenderLayer_defined_
#define2 regen_RenderLayer_defined_
#if RENDER_LAYER > 1
    // Compute render layer from gl_DrawID.
    // Currently the mesh may have n indirect draw calls for n LOD levels,
    // with layered rendering we repeat each LOD level for each layer.
    #ifdef USE_GS_LAYERED_RENDERING
    #define regen_RenderLayer() 0
    #else
    #define regen_RenderLayer() (gl_DrawID % ${RENDER_LAYER})
    #endif
#else
    #define regen_RenderLayer() 0
#endif
#endif // regen_InstanceID_defined_

-- all
// enable GL_ARB_shader_viewport_layer_array if we do layered rendering.
// this allows us to select the render layer in the vertex shader.
#if SHADER_STAGE == vs && RENDER_LAYER > 1
    #ifdef ARB_shader_viewport_layer_array
        #ifndef USE_GS_LAYERED_RENDERING
            #ifndef USE_GEOMETRY_SHADER
#extension GL_ARB_shader_viewport_layer_array : require
            #endif
        #endif
    #endif
#endif
#if RENDER_LAYER > 1
    #ifndef USE_GS_LAYERED_RENDERING
#define VS_LAYER_SELECTION
    #endif
#endif
#ifdef HAS_nor && HAS_tan
#define HAS_TANGENT_SPACE
#endif
#if SHADER_STAGE == tes
#define SAMPLE(T,C) texture(T,INTERPOLATE_VALUE(C))
#else
#define SAMPLE(T,C) texture(T,C)
#endif
#ifndef PI
#define PI 3.14159265
#endif
