
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
    vec3 dir = in_userPosition - pos.xyz;
    float d = length(dir);
    d = abs(dot(dir/d, in_userDirection)) * d;
    d = 0.5 * (in_userProjection[3][2] / d - in_userProjection[2][2]) + 0.5;
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
