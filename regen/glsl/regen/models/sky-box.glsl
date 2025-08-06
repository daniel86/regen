--------------------------------
--------------------------------
----- A simple sky box mesh. Depth test should be done against scene depth
----- buffer. The cube faces are translated to the far plane. The depth
----- function should be less or equal.
--------------------------------
--------------------------------
-- emitVertex
#define HANDLE_IO(i)
void emitVertex(vec3 pos, int index, int layer) {
    vec4 posWorld = vec4(normalize(pos),1.0);
#ifdef VS_CAMERA_TRANSFORM
    out_posWorld = posWorld.xyz;
    vec4 posEye = transformWorldToEye(posWorld,layer);
    out_posEye = posEye.xyz;
    gl_Position = transformEyeToScreen(posEye,layer);
#else
    gl_Position = posWorld;
#endif
    VS_SelectLayer(layer);
    HANDLE_IO(index);
}

-- vs_include
#include regen.models.mesh.defines

in vec3 in_pos;

#include regen.states.camera.input
#ifdef VS_LAYER_SELECTION
flat out int out_layer;
#endif
#include regen.layered.VS_SelectLayer

#ifdef VS_CAMERA_TRANSFORM
out vec3 out_posWorld;
out vec3 out_posEye;
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#endif

-- vs
#include regen.models.sky-box.vs_include
#include regen.models.sky-box.emitVertex
void main() {
    int layer = regen_RenderLayer();
    emitVertex(in_pos.xyz, gl_VertexID, layer);
}

-- tcs
#include regen.models.mesh.tcs
-- tes
#include regen.models.mesh.tes
-- gs
#include regen.models.mesh.gs

-- fs
#include regen.models.mesh.defines
#include regen.states.textures.defines

out vec4 out_color;

#include regen.states.textures.input
#include regen.states.textures.mapToFragmentUnshaded

void main() {
    textureMappingFragmentUnshaded(gl_FragCoord.xyz, out_color);
}
