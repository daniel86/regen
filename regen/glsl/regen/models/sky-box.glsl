--------------------------------
--------------------------------
----- A simple sky box mesh. Depth test should be done against scene depth
----- buffer. The cube faces are translated to the far plane. The depth
----- function should be less or equal.
--------------------------------
--------------------------------
-- vs
#extension GL_EXT_gpu_shader4 : enable
#include regen.models.mesh.defines

in vec3 in_pos;

#include regen.states.camera.input

#if RENDER_LAYER == 1
out vec4 out_posWorld;
out vec4 out_posEye;

#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#endif

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = vec4(normalize(in_pos.xyz)*__CAM_FAR__(0)*0.99,1.0);
#if RENDER_LAYER > 1
    gl_Position = posWorld;
#else
    out_posWorld = posWorld;
    out_posEye = transformWorldToEye(posWorld,0);
    gl_Position = transformEyeToScreen(out_posEye,0);
#endif
    HANDLE_IO(gl_VertexID);
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
