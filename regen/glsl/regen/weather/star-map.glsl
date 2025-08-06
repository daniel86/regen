
--------------------------------
--------------------------------
----- Visualization of star map.
----- Code based on: https://code.google.com/p/osghimmel/
--------------------------------
--------------------------------

-- vs
#include regen.models.sky-box.vs_include

uniform mat4 in_equToHorMatrix;
out vec3 out_ray;

#include regen.models.sky-box.emitVertex

void emitStarVertex(vec3 pos, int index, int layer) {
    out_ray = (in_equToHorMatrix * vec4(pos,0.0)).xyz;
    emitVertex(pos,index,layer);
}

void main() {
    emitStarVertex(in_pos.xyz, gl_VertexID, 0);
}

-- tcs
#include regen.models.sky-box.tcs
-- tes
#include regen.models.sky-box.tes
-- gs
#include regen.models.sky-box.gs

-- fs
#include regen.models.mesh.defines
#include regen.states.textures.defines

out vec4 out_color;

in vec3 in_posWorld;
in vec3 in_posEye;
in vec3 in_ray;

uniform float in_sqrt_q;
uniform vec4 in_cmn;
uniform vec2 in_inverseViewport;

const float in_deltaM = 1.0;
const float in_scattering = 1.0;
const float surfaceHeight = 0.99; // TODO: shouldn't this be a uniform?

uniform samplerCube in_starmapCube;

#include regen.states.textures.input

#include regen.weather.utility.scatter
#include regen.weather.utility.computeEyeExtinction
#include regen.weather.utility.sunIntensity

void main(void) {
    float ext = computeEyeExtinction(in_posWorld.xyz);
    float omega = acos(in_posWorld.y * 0.9998);

    vec4 fc = texture(in_starmapCube, in_ray);
    fc *= 3e-2 / in_sqrt_q * in_deltaM;

    out_color = smoothstep(0.0, 0.05, ext) *
        vec4(sunIntensity() * (fc.rgb - in_scattering*scatter(omega)), 1.0);
    //out_color = vec4(in_ray.xyz,1);
}
