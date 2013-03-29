
--------------------------------
--------------------------------
----- Shader for GUI rendering
--------------------------------
--------------------------------
-- vs
in vec3 in_pos;
in vec2 in_viewport;
#ifdef HAS_modelMatrix
uniform mat4 in_modelMatrix;
#endif

#define HANDLE_IO()

void main() {
    vec2 pos = 2.0*in_pos.xy;
#ifndef USE_NORMALIZED_COORDINATES
    pos.x -= in_viewport.x;
    pos.y += in_viewport.y;
#endif
#ifdef HAS_modelMatrix
    pos.x += in_modelMatrix[3].x;
    pos.y -= in_modelMatrix[3].y;
#endif
#ifndef USE_NORMALIZED_COORDINATES
    pos /= in_viewport;
#endif
#ifdef INVERT_Y
    pos.y -= 2.0;
#endif
#ifdef INVERT_X
    pos.x += 2.0;
#endif

    gl_Position = vec4(pos, 0.0, 1.0);

    HANDLE_IO(gl_VertexID);
}

-- fs
#include textures.defines
#undef HAS_LIGHT
#undef HAS_MATERIAL

#include textures.input
#include textures.mapToFragmentUnshaded

out vec4 out_color;

void main() {
    out_color = vec4(1.0);
    textureMappingFragmentUnshaded(gl_FragCoord.xyz,out_color);
}

--------------------------------
--------------------------------
----- Maps font textures to geometry.
----- Font is saved as texture array and can indexed by the characters.
--------------------------------
--------------------------------
-- text.vs
#include gui.vs
-- text.fs
#include textures.defines
#include textures.input
#include textures.mapToFragmentUnshaded
uniform vec4 in_textColor;
out vec4 out_color;

void main() {
    vec4 color = in_textColor;
    textureMappingFragmentUnshaded(gl_FragCoord.xyz,color);
    out_color = color;
}


--------------------------------
--------------------------------
--------------------------------
--------------------------------
-- textOutline.vs
#include gui.vs
-- textOutline.gs
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=12) out;

uniform vec2 in_inverseViewport;
uniform vec4 in_textColor;
const float outlineOffset = 2.0;

#define HANDLE_IO

void main() {
    int i;
    vec2 offset = outlineOffset*in_inverseViewport;
    
    for(i = 0; i < 3; i++) {
        gl_Position = gl_PositionIn[i] + offset;
        HANDLE_IO(i);
        EmitVertex();
    }
    EndPrimitive();
    for(i = 0; i < 3; i++) {
        gl_Position = gl_PositionIn[i] - offset;
        HANDLE_IO(i);
        EmitVertex();
    }
    EndPrimitive();
    for(i = 0; i < 3; i++) {
        gl_Position = gl_PositionIn[i] + vec2(offset.x, -offset.y);
        HANDLE_IO(i);
        EmitVertex();
    }
    EndPrimitive();
    for(i = 0; i < 3; i++) {
        gl_Position = gl_PositionIn[i] + vec2(-offset.x, offset.y);
        HANDLE_IO(i);
        EmitVertex();
    }
    EndPrimitive();
}

-- textOutline.fs
#include textures.defines
#include textures.input
#include textures.mapToFragmentUnshaded

out vec4 out_color;

void main() {
    out_color = vec4(0.0, 0.0, 0.0, 1.0);
    textureMappingFragmentUnshaded(gl_FragCoord.xyz,out_color);
}

