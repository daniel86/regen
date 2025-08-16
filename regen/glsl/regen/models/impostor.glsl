
-- selectViewIdx
#ifndef REGEN_selectViewIdx_defined_
#define2 REGEN_selectViewIdx_defined_
uint selectViewIdx(vec3 viewDirLocal) {
    float maxDot = 0, d;
    uint bestIndex = 0;
    /**
    #for VIEW_I to NUM_IMPOSTOR_VIEWS
    d = dot(viewDirLocal, in_snapshotDirs[${VIEW_I}].xyz);
    if (d > maxDot) {
        maxDot = d;
        bestIndex = ${VIEW_I};
    }
    #endfor
    **/
    for (uint i = 0; i < NUM_IMPOSTOR_VIEWS; ++i) {
        d = dot(viewDirLocal, in_snapshotDirs[i].xyz);
        if (d > maxDot) {
            maxDot = d;
            bestIndex = i;
        }
    }
    return bestIndex;
}
#endif

-- getViewIdx
#ifndef REGEN_getViewIdx_defined_
#define2 REGEN_getViewIdx_defined_
#include regen.models.impostor.selectViewIdx
uint getViewIdx(int layer, vec3 centerWorld) {
    // Find the best impostor view index based on the view direction.
#ifdef HAS_modelMatrix
    vec3 viewDirWorld = normalize(REGEN_CAM_POS_(layer) - centerWorld.xyz);
    vec3 viewDirLocal = transpose(mat3(in_modelMatrix)) * viewDirWorld;
#else
    vec3 viewDirLocal = normalize(REGEN_CAM_POS_(layer) - centerWorld.xyz);
#endif
    return selectViewIdx(viewDirLocal);
}
#endif

-- getSpriteSize
#ifndef REGEN_getSpriteSize_defined_
#define2 REGEN_getSpriteSize_defined_
vec2 getSpriteSize(inout vec4 centerEye, vec3 zAxis, uint viewIdx, float scale) {
    vec4 orthoBounds = in_snapshotOrthoBounds[viewIdx];
#ifndef DEPTH_CORRECT
    vec2 depthRange = in_snapshotDepthRanges[viewIdx];
#endif
    // Compute size of the quad in world space, based on the ortho bounds of the selected view.
    vec2 spriteSize = vec2(orthoBounds.y - orthoBounds.x, orthoBounds.w - orthoBounds.z) * scale;
#ifndef DEPTH_CORRECT
    float zCenter = centerEye.z;
    #if OUTPUT_TYPE == DEPTH
    centerEye.xyz += zAxis * 0.5 * (depthRange.y - depthRange.x) * scale;
    #else
    centerEye.xyz -= zAxis * 0.5 * (depthRange.y - depthRange.x) * scale;
    #endif
    spriteSize *= abs(centerEye.z / zCenter);
#endif
    return spriteSize;
}
#endif

/**
 * This shader renders different views of a billboard impostor using 2D texture arrays.
 * Only one draw call is needed to render all views of the impostor thanks to layered rendering
 * and geometry shaders.
 * The geometry shader takes as input a triangle, and emits it into the different layers of
 * the array textures.
 **/
-- update.defines
// the model should be centered at origin, so we need
// to ignore the model matrix.
#define IGNORE_modelMatrix
#define IGNORE_modelOffset
// material parameters are handled by billboard to
// allow per-instance variations.
#define IGNORE_MATERIAL

-- update.vs
#include regen.models.impostor.update.defines
#include regen.models.mesh.defines
#include regen.models.mesh.vs

-- update.gs
#include regen.models.impostor.update.defines
#include regen.models.mesh.gs

-- update.fs
#include regen.models.impostor.update.defines
#include regen.models.mesh.fs

-- quad.vs
#include regen.models.mesh.defines

in vec3 in_pos;
out vec3 out_posWorld;
out vec3 out_texco0;
#if OUTPUT_TYPE != DEPTH
out vec3 out_posEye;
#endif
#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif
#if RENDER_LAYER > 1
flat out int out_layer;
#define in_layer regen_RenderLayer()
#endif

#include regen.states.camera.transformEyeToScreen
#include regen.states.camera.transformEyeToWorld
#include regen.states.camera.transformWorldToEye
#include regen.layered.VS_SelectLayer

#ifdef HAS_windFlow
#include regen.models.sprite.applyForceBase
#include regen.weather.wind.windAtPosition
#endif

#define HANDLE_IO(i)

#include regen.models.impostor.getSpriteSize
#include regen.models.impostor.getViewIdx

void main() {
    int layer = regen_RenderLayer();
#ifdef HAS_modelMatrix
    float scale = length(in_modelMatrix[0].xyz);
#else
    float scale = 1.0;
#endif
#ifdef HAS_modelOrigin
    vec3 centerWorld = (in_modelMatrix * vec4(in_modelOrigin, 1.0)).xyz;
#else
    vec3 centerWorld = (in_modelMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
#endif
#ifdef HAS_modelOffset
    centerWorld += in_modelOffset.xyz;
#endif
    // Find the best impostor view index based on the view direction.
    vec4 centerEye = transformWorldToEye(centerWorld, layer);

    // Find the best impostor view index based on the view direction.
    uint viewIdx = getViewIdx(layer, centerWorld.xyz);
    float viewCoord = float(viewIdx);
    // Compute the size of the quad in eye space, based on the ortho bounds of the selected view.
    vec3 zAxis = normalize(centerEye.xyz);
    vec2 spriteSize = getSpriteSize(centerEye, zAxis, viewIdx, scale);

    // note: in_pos.xy is one of the four corners of the quad, in range [-0.5, 0.5]
    vec4 posEye = vec4(
        centerEye.xy + spriteSize * in_pos.xy,
        centerEye.z, 1.0);

#ifdef HAS_windFlow && HAS_undefined
    // TODO: support wind here
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
#else
    out_posWorld = transformEyeToWorld(posEye,layer).xyz;
#endif
#if OUTPUT_TYPE != DEPTH
    out_posEye = posEye.xyz;
#endif
    // move from range [-0.5, 0.5] to [0, 1], add viewCoord for texture array lookup
    out_texco0 = vec3(in_pos.xy + vec2(0.5), viewCoord);
#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID + gl_BaseInstance;
#endif // HAS_INSTANCES
    gl_Position = transformEyeToScreen(posEye,layer);
    VS_SelectLayer(layer);
    HANDLE_IO(gl_VertexID);
}
-- quad.gs
#include regen.models.mesh.gs

-- extrude.vs
#include regen.models.mesh.defines

in vec3 in_pos;
#ifdef HAS_INSTANCES
flat out int out_instanceID;
#endif
#if RENDER_LAYER > 1
flat out int out_layer;
#define in_layer regen_RenderLayer()
#endif

#define HANDLE_IO(i)

void main() {
    int layer = regen_RenderLayer();
    vec4 pos = vec4(in_pos,1.0);
#ifdef HAS_modelOrigin
    // Translate the center of the quad to the center of the original mesh,
    // for the case where the mesh is not centered at the origin.
    pos.xyz += in_modelOrigin;
#endif
#ifdef HAS_modelMatrix
    pos = in_modelMatrix * pos;
#endif
#ifdef HAS_modelOffset
    pos.xyz += in_modelOffset.xyz;
#endif
    gl_Position = pos;
#ifdef HAS_INSTANCES
    out_instanceID = gl_InstanceID + gl_BaseInstance;
#endif // HAS_INSTANCES
#if RENDER_LAYER > 1
    out_layer = layer;
#endif
    HANDLE_IO(gl_VertexID);
}
-- extrude.gs
#include regen.models.mesh.defines

layout(points) in;
layout(triangle_strip, max_vertices=6) out;

#include regen.states.camera.input

flat out uint out_impostorIdx;
out vec3 out_texco0;
out vec3 out_posEye;
out vec3 out_posWorld;

buffer vec4 in_snapshotDirs[];
buffer vec4 in_snapshotOrthoBounds[];
buffer vec2 in_snapshotDepthRanges[];

const float in_depthOffset = 0.5f;

#include regen.states.camera.transformEyeToScreen
#include regen.states.camera.transformEyeToWorld
#include regen.states.camera.transformWorldToEye
#include regen.math.computeSpritePoints

#ifdef HAS_windFlow
#include regen.models.sprite.applyForce
#include regen.weather.wind.windAtPosition
#endif

#define HANDLE_IO(i)

#include regen.models.impostor.getSpriteSize
#include regen.models.impostor.getViewIdx

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
    uint viewIdx = getViewIdx(layer, centerWorld.xyz);
    float viewCoord = float(viewIdx);
    // Compute the size of the quad in eye space, based on the ortho bounds of the selected view.
    vec3 zAxis = normalize(centerEye.xyz);
    vec2 spriteSize = getSpriteSize(centerEye, zAxis, viewIdx, scale);
    // Finally, compute the four corners of the quad in eye space.
    vec3 up = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), float(abs(zAxis.y) < 0.99));
    vec3 quadPos[4] = computeSpritePoints(centerEye.xyz, spriteSize, zAxis, up);

#ifdef HAS_windFlow
    vec3 bottomCenter = 0.5*(quadPos[0] + quadPos[2]);
    vec2 wind = windAtPosition(bottomCenter);
    applyForce(quadPos, wind);
#endif

    // Emit the quad as two triangles.
    out_impostorIdx = viewIdx;
    // bottom-left, top-left, bottom-right
    emitVertex(vec4(quadPos[2],1.0), vec3(1.0,0.0,viewCoord), layer);
    emitVertex(vec4(quadPos[1],1.0), vec3(0.0,1.0,viewCoord), layer);
    emitVertex(vec4(quadPos[0],1.0), vec3(0.0,0.0,viewCoord), layer);
    EndPrimitive();
    // bottom-right, top-left, top-right
    emitVertex(vec4(quadPos[3],1.0), vec3(1.0,1.0,viewCoord), layer);
    emitVertex(vec4(quadPos[1],1.0), vec3(0.0,1.0,viewCoord), layer);
    emitVertex(vec4(quadPos[2],1.0), vec3(1.0,0.0,viewCoord), layer);
    EndPrimitive();
}

void main() {
#ifdef HAS_modelMatrix
    float scale = length(in_modelMatrix[0].xyz);
#else
    float scale = 1.0;
#endif
#if RENDER_LAYER > 1
    int layer = in_layer[0];
    gl_Layer = layer;
#else
    int layer = 0;
#endif
    emitLayer(layer, scale);
}

/**
 * This shader renders a billboard impostor using 2D texture arrays.
 * The arrays are used for mapping to diffuse, normal, specular components,
 * and can be combined with material properties in the fragment shader.
 **/
-- vs
#ifdef USE_POINT_EXTRUSION
#include regen.models.impostor.extrude.vs
#else
#include regen.models.impostor.quad.vs
#endif
-- gs
#ifdef USE_POINT_EXTRUSION
#include regen.models.impostor.extrude.gs
#else
#include regen.models.impostor.quad.gs
#endif
-- fs
#include regen.models.mesh.fs
