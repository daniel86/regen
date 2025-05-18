
-- selectViewIdx
#ifndef REGEN_selectViewIdx_defined_
#define2 REGEN_selectViewIdx_defined_
uint selectViewIdx(vec3 viewDirLocal) {
    float maxDot = 0, d;
    uint bestIndex = 0;
    /**
    #for VIEW_I to NUM_IMPOSTOR_VIEWS
    d = dot(viewDirLocal, in_snapshotDirs[${VIEW_I}]);
    if (d > maxDot) {
        maxDot = d;
        bestIndex = ${VIEW_I};
    }
    #endfor
    **/
    for (uint i = 0; i < NUM_IMPOSTOR_VIEWS; ++i) {
        d = dot(viewDirLocal, in_snapshotDirs[i]);
        if (d > maxDot) {
            maxDot = d;
            bestIndex = i;
        }
    }
    return bestIndex;
}
#endif

/**
 * This shader renders a billboard impostor using a 2D texture array.
 **/
-- vs
#include regen.models.mesh.defines
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

in vec3 in_pos;
#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif

#define HANDLE_IO(i)

void main() {
#ifdef HAS_modelMatrix
    // TODO: only offset is needed
    vec4 pos = in_modelMatrix * vec4(in_pos,1.0);
#else
    vec4 pos = vec4(in_pos,1.0);
#endif
#ifdef HAS_modelOffset
    pos.xyz += in_modelOffset;
#endif
    gl_Position = pos;
#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID;
#endif // HAS_INSTANCES
    HANDLE_IO(gl_VertexID);
}

-- gs
#include regen.models.mesh.defines
#define2 REGEN_MAX_VERTICES ${${RENDER_LAYER}*6}

layout(points) in;
layout(triangle_strip, max_vertices=${REGEN_MAX_VERTICES}) out;

#include regen.states.camera.input

#if RENDER_LAYER > 1
flat out int out_layer;
#endif
flat out uint out_impostorIdx;

flat out vec3 out_norWorld;
flat out vec3 out_tangent;
flat out vec3 out_binormal;

out vec3 out_texco0;
out vec3 out_posEye;
out vec3 out_posWorld;

const float in_depthOffset = 0.5f;
const vec3 in_modelOrigin = vec3(0.0f);

#include regen.states.camera.transformEyeToScreen
#include regen.states.camera.transformEyeToWorld
#include regen.states.camera.transformWorldToEye
#include regen.math.computeSpritePoints
#include regen.layered.gs.computeVisibleLayers
#include regen.models.impostor.selectViewIdx

#define HANDLE_IO(i)

void writeFlatOutput(int layer, uint viewIdx, vec3 N, vec3 T, vec3 B) {
    #if RENDER_LAYER > 1
    out_layer = layer;
    #endif
    out_impostorIdx = viewIdx;
    out_norWorld = N;
    out_tangent = T;
    out_binormal = B;
}

void emitVertex(vec4 posEye, vec3 texco, int layer) {
    out_texco0 = texco;
    out_posEye = posEye.xyz;
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
    HANDLE_IO(0);
    EmitVertex();
}

void emitLayer(int layer, float scale) {
    vec4 centerWorld = gl_in[0].gl_Position;
    vec4 centerEye = transformWorldToEye(centerWorld, layer);
    // Find the best impostor view index based on the view direction.
    vec3 viewDirLocal = normalize(centerWorld.xyz - REGEN_CAM_POS_(0));
    uint viewIdx = selectViewIdx(viewDirLocal);
    // Read impostor data from SSBO.
    vec4 orthoBounds = in_snapshotOrthoBounds[viewIdx];
#ifndef DEPTH_CORRECT
    vec2 depthRange = in_snapshotDepthRanges[viewIdx];
#endif
    // Compute size of the quad in world space, based on the ortho bounds of the selected view.
    vec2 spriteSize = vec2(orthoBounds.y - orthoBounds.x, orthoBounds.w - orthoBounds.z) * scale;
    // Translate the center of the quad to the center of the original mesh,
    // for the case where the mesh is not centered at the origin.
    centerEye.xyz += in_modelOrigin * scale;
#ifndef DEPTH_CORRECT
    // Pull the mesh closer to the camera to avoid z-fighting issues when it is placed in
    // the center of the original mesh.
    // e.g. in case of a tree, there is also a trunk in the center of the mesh and we might want
    // to pull the impostor closer to the camera (i.e. using in_depthOffset=0.5)
    centerEye.z += in_depthOffset * (depthRange.y - depthRange.x) * scale;
#endif

    // build a coordinate system for the quad
    vec3 zAxis = normalize(centerEye.xyz);
    vec3 up = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), float(abs(zAxis.y) < 0.99));
    vec3 quadPos[4] = computeSpritePoints(centerEye.xyz, spriteSize, zAxis, up);
    float viewCoord = float(viewIdx);

    // construct tangent space
    vec3 N = -viewDirLocal;
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);

    // bottom-left, top-left, bottom-right
    {
        writeFlatOutput(layer, viewIdx, N, T, B);
        emitVertex(vec4(quadPos[2],1.0), vec3(1.0,0.0,viewCoord), layer);
    }
    {
        writeFlatOutput(layer, viewIdx, N, T, B);
        emitVertex(vec4(quadPos[1],1.0), vec3(1.0,1.0,viewCoord), layer);
    }
    {
        writeFlatOutput(layer, viewIdx, N, T, B);
        emitVertex(vec4(quadPos[0],1.0), vec3(0.0,0.0,viewCoord), layer);
    }
    EndPrimitive();
    // bottom-right, top-left, top-right
    {
        writeFlatOutput(layer, viewIdx, N, T, B);
        emitVertex(vec4(quadPos[3],1.0), vec3(0.0,0.0,viewCoord), layer);
    }
    {
        writeFlatOutput(layer, viewIdx, N, T, B);
        emitVertex(vec4(quadPos[1],1.0), vec3(1.0,1.0,viewCoord), layer);
    }
    {
        writeFlatOutput(layer, viewIdx, N, T, B);
        emitVertex(vec4(quadPos[2],1.0), vec3(0.0,1.0,viewCoord), layer);
    }
    EndPrimitive();
}

float scaleFromMatrix(mat4 model) {
    // NOTE: we assume uniform scaling of the original mesh
    return length(model[0].xyz);
}

void main() {
    // the original mesh might be scaled on per-instance basis
    float scale = scaleFromMatrix(in_modelMatrix);
#ifdef COMPUTE_LAYER_VISIBILITY
    bool visibleLayers[RENDER_LAYER];
    computeVisibleLayers(visibleLayers);
#endif
#for LAYER to ${RENDER_LAYER}
    #ifndef SKIP_LAYER${LAYER}
        #ifdef COMPUTE_LAYER_VISIBILITY
    if (visibleLayers[${LAYER}]) {
        #endif // COMPUTE_LAYER_VISIBILITY
        #if RENDER_LAYER > 1
        gl_Layer = ${LAYER};
        #endif
        emitLayer(${LAYER}, scale);
        #ifdef COMPUTE_LAYER_VISIBILITY
    }
        #endif // COMPUTE_LAYER_VISIBILITY
    #endif
#endfor
}

-- fs
#define HAS_TANGENT_SPACE
#define HAS_nor
// TODO: reconsider how we can force FS to use flat inputs
#define HAS_flat_nor
#define HAS_flat_tangent
#define HAS_flat_binormal
#include regen.models.mesh.fs
