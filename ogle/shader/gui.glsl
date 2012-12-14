// Shader for GUI rendering

--------------------------------------------
------------- Vertex Shader ----------------
--------------------------------------------

-- vs
#undef HAS_LIGHT
#undef HAS_MATERIAL

in vec3 in_pos;
in vec2 in_viewport;
#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif

#define HANDLE_IO()

void main() {
    vec2 pos = 2.0*in_pos.xy;
#ifndef USE_NORMALIZED_COORDINATES
    pos.x -= in_viewport.x;
    pos.y += in_viewport.y;
#endif
#ifdef HAS_MODELMAT
    pos.x += in_modelMatrix[3].x;
    pos.y -= in_modelMatrix[3].y;
#endif
#ifndef USE_NORMALIZED_COORDINATES
    pos /= in_viewport;
#endif

    gl_Position = vec4(pos, 0.0, 1.0);

    HANDLE_IO(gl_VertexID);
}

-- fs
#include textures.defines
#undef HAS_LIGHT
#undef HAS_MATERIAL

#include textures.input

#include textures.mapToFragment

out vec4 output;

void main() {
    float A = 0.0;
    vec3 N = vec3(0.0);
    output = vec4(1,1,1,1);
    textureMappingFragment(gl_FragCoord.xyz,N,output,A);
}

