// Shader for GUI rendering

--------------------------------------------
------------- Vertex Shader ----------------
--------------------------------------------

-- vs.header
#define SAMPLE(TEX,TEXCO) texture(TEX, TEXCO)
#undef HAS_LIGHT
#undef HAS_MATERIAL

in vec3 in_pos;
in vec2 in_viewport;
#ifdef HAS_MODELMAT
uniform mat4 in_modelMatrix;
#endif

-- vs.main

#define HANDLE_IO()

void main() {
    vec2 pos = 2.0*in_pos.xy;
    pos.x -= in_viewport.x;
    pos.y += in_viewport.y;
#ifdef HAS_MODELMAT
    pos.x += in_modelMatrix[3].x;
    pos.y -= in_modelMatrix[3].y;
#endif
    pos /= in_viewport;

    gl_Position = vec4(pos, 0.0, 1.0);

    HANDLE_IO(gl_VertexID);
}

--------------------------------------------
--------- Tesselation Control --------------
--------------------------------------------

-- tcs.header

-- tcs.main
void main() {}

--------------------------------------------
--------- Tesselation Evaluation -----------
--------------------------------------------

-- tes.header

-- tes.main
void main() {}

--------------------------------------------
--------- Geometry Shader ------------------
--------------------------------------------

-- gs.header

-- gs.main
void main() {}

--------------------------------------------
--------- Fragment Shader ------------------
--------------------------------------------

-- fs.header
#define SAMPLE(TEX,TEXCO) texture(TEX, TEXCO)
#undef HAS_LIGHT
#undef HAS_MATERIAL

out vec4 output;

-- fs.main
void main() {
    output = vec4(1,1,1,1);
    modifyColor(output);
}

