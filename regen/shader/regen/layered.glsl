
-- defines
#ifndef regen_layered_defines_defined_
#define regen_layered_defines_defined_
#ifndef HAS_layer
    #if RENDER_LAYER > 1
flat in int in_layer;
    #else
#define in_layer 0
    #endif
#endif
#endif // regen_layered_defines_defined_

-- VS_SelectLayer
#ifndef regen_VS_SelectLayer_defined_
#define2 regen_VS_SelectLayer_defined_
#ifdef VS_LAYER_SELECTION
void VS_SelectLayer(int layer) {
    out_layer = layer;
#ifdef ARB_shader_viewport_layer_array
    gl_Layer = layer;
#endif // ARB_shader_viewport_layer_array
}
#else // !defined(VS_LAYER_SELECTION)
#define VS_SelectLayer(x)
#endif // VS_LAYER_SELECTION
#endif
