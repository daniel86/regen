
--------------
----- Late culling in the geometry shader based on assumptions about
----- the render target.
--------------
-- gs.computeVisibleLayers
#ifndef REGEN_computeVisibleLayers_
#define2 REGEN_computeVisibleLayers_

#ifdef COMPUTE_LAYER_VISIBILITY
    #if RENDER_TARGET_MODE == CASCADE
int computeCascadeLayer(vec4 pos) {
    // TODO: Something is not working here. There are visible edges at the boundaries of layers.
    //       Note sure what the problem is. Pretty much the same code is used for shadow sampling:
    //       i.e. given a depth value, compute the layer where the depth value is located.
    //       However, in this case the depth value is read from the depth buffer, render target
    //       is different etc. Some ideas are precision issues, or problem with linear vs.
    //       non-linear depth values.
    vec3 dir = in_cameraPosition_User - pos.xyz;
    float d = length(dir);
    d = abs(dot(dir/d, in_cameraDirection_User)) * d;
    d = 0.5 * (in_cameraProjection_User[3][2] / d - in_cameraProjection_User[2][2]) + 0.5;
    // get the first layer where pos is located before the far plane
    int layer = ${RENDER_LAYER};
    #for LAYER to ${RENDER_LAYER}
    layer = min(layer, ${RENDER_LAYER} -
        int(d < in_lightProjParams[${LAYER}].y) * (${RENDER_LAYER} - ${LAYER}));
    #endfor
    return layer;
}
    #endif

void computeVisibleLayers(out bool visibilityFlags[RENDER_LAYER])
{
    #if RENDER_TARGET_MODE == CASCADE
    int vLayer0 = computeCascadeLayer(gl_in[0].gl_Position);
    int vLayer1 = computeCascadeLayer(gl_in[1].gl_Position);
    int vLayer2 = computeCascadeLayer(gl_in[2].gl_Position);
    int minLayer = min(vLayer0, min(vLayer1, vLayer2));
    int maxLayer = max(vLayer0, max(vLayer1, vLayer2));
	    #for LAYER to ${RENDER_LAYER}
    visibilityFlags[${LAYER}] = (minLayer <= ${LAYER} && maxLayer >= ${LAYER});
        #endfor
    #else
        #for LAYER to ${RENDER_LAYER}
            #if RENDER_TARGET == INSTANCE_SELF
    visibilityFlags[${LAYER}] = (in_instanceID[0] == ${LAYER});
            #elif RENDER_TARGET == INSTANCE_OTHER
    visibilityFlags[${LAYER}] = (in_instanceID[0] != ${LAYER});
            #else
    visibilityFlags[${LAYER}] = true;
            #endif
        #endfor
    #endif
}
#else
    #define computeVisibleLayers(layers)
#endif
#endif

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
