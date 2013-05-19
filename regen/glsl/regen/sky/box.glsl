--------------------------------
--------------------------------
----- A simple sky box mesh. Depth test should be done against scene depth
----- buffer. The cube faces are translated to the far plane. The depth
----- function should be less or equal.
--------------------------------
--------------------------------
-- vs
#extension GL_EXT_gpu_shader4 : enable
#include regen.meshes.mesh.defines

in vec3 in_pos;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
uniform float in_far;

#include regen.meshes.mesh.transformation

#define HANDLE_IO(i)

void main() {
    vec4 posWorld = vec4(in_pos.xyz*in_far*0.99,1.0);
    vec4 posScreen = in_projectionMatrix * posEyeSpace(posWorld);
    // push to far plane. needs less or equal check
    posScreen.z = posScreen.w;
    
    gl_Position = posScreen;
    HANDLE_IO(gl_VertexID);
}

-- fs
#include regen.meshes.mesh.defines
#include regen.utility.textures.defines

out vec4 out_color;

#include regen.utility.textures.input
#include regen.utility.textures.mapToFragmentUnshaded

void main() {
    textureMappingFragmentUnshaded(gl_FragCoord.xyz, out_color);
}
