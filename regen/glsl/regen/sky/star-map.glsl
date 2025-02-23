
--------------------------------
--------------------------------
----- Visualization of star map.
----- Code based on: https://code.google.com/p/osghimmel/
--------------------------------
--------------------------------

-- emitStarVertex
void emitStarVertex(vec3 pos, int index, int layer) {
    emitVertex(pos,index,layer);
    out_ray = (in_equToHorMatrix * vec4(pos,0.0)).xyz;
}

-- vs_include
#include regen.models.sky-box.vs_include
-- vs
#if RENDER_LAYER == 1
uniform mat4 in_equToHorMatrix;
out vec3 out_ray;
#endif
#include regen.sky.star-map.vs_include
#if RENDER_LAYER == 1
#include regen.sky.star-map.emitStarVertex
void main() {
    emitStarVertex(in_pos.xyz, gl_VertexID, 0);
}
#else
#define HANDLE_IO(i)
void main() {
    gl_Position = vec4(in_pos,0.0);
    HANDLE_IO(gl_VertexID);
}
#endif


-- tcs
#include regen.models.sky-box.tcs
-- tes
#include regen.models.sky-box.tes

-- gs_include
#include regen.models.sky-box.gs_include
#include regen.sky.star-map.emitStarVertex
-- gs
#if RENDER_LAYER > 1
uniform mat4 in_equToHorMatrix;
out vec3 out_ray;
#include regen.sky.star-map.gs_include
void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
    gl_Layer = ${LAYER};
    out_layer = ${LAYER};
    emitStarVertex(gl_in[0].gl_Position.xyz, 0, ${LAYER}); EmitVertex();
    emitStarVertex(gl_in[1].gl_Position.xyz, 1, ${LAYER}); EmitVertex();
    emitStarVertex(gl_in[2].gl_Position.xyz, 2, ${LAYER}); EmitVertex();
    EndPrimitive();
#endif // SKIP_LAYER
#endfor
}
#endif

-- fs
#include regen.models.mesh.defines
#include regen.states.textures.defines

out vec4 out_color;

in vec4 in_posWorld;
in vec4 in_posEye;
in vec3 in_ray;

uniform vec3 in_sunPosition;
uniform float in_q;
uniform vec4 in_cmn;
uniform vec2 in_inverseViewport;

const float in_deltaM = 1.0;
const float in_scattering = 1.0;
const float surfaceHeight = 0.99;

uniform samplerCube in_starmapCube;

#include regen.states.textures.input

#include regen.sky.utility.scatter
#include regen.sky.utility.computeEyeExtinction
#include regen.sky.utility.sunIntensity

void main(void) {
    vec3 eye = in_posWorld.xyz;
    float ext = computeEyeExtinction(in_posWorld.xyz);
    if(ext <= 0.0) discard;

    vec4 fc = texture(in_starmapCube, in_ray);
    fc *= 3e-2 / sqrt(in_q) * in_deltaM;

    float omega = acos(eye.y * 0.9998);

    out_color = smoothstep(0.0, 0.05, ext) *
        vec4(sunIntensity() * (fc.rgb - in_scattering*scatter(omega)), 1.0);
    //out_color = vec4(in_ray.xyz,1);
}
