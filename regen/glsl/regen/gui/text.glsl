
--------------------------------
--------------------------------
----- Maps font textures to geometry.
----- Font is saved as texture array and can indexed by the characters.
--------------------------------
--------------------------------
-- vs
#include regen.gui.widget.vs
-- fs
#include regen.states.textures.defines
#include regen.states.textures.input
#include regen.states.textures.mapToFragmentUnshaded
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
-- outline.vs
#include regen.gui.widget.vs
-- outline.gs
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

-- outline.fs
#include regen.states.textures.defines
#include regen.states.textures.input
#include regen.states.textures.mapToFragmentUnshaded

out vec4 out_color;

void main() {
    out_color = vec4(0.0, 0.0, 0.0, 1.0);
    textureMappingFragmentUnshaded(gl_FragCoord.xyz,out_color);
}

