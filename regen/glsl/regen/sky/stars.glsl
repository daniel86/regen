-- vs
#include regen.sky.cube.vs

-- gs
#include regen.sky.cube.gs

-- fs
out vec4 out_color;
in vec3 in_pos;

uniform float in_starMapBrightness;
uniform mat4 in_starMapRotation;
uniform samplerCube starMap;

uniform vec3 in_sunDir;

#include regen.sky.utility.all

void main() {
    vec4 eyedir = in_starMapRotation * vec4(normalize(in_pos),1.0);
    float eyeExtinction = getEyeExtinction(eyedir.xyz);
    vec4 starMapColor = texture(starMap,eyedir.xyz);
    out_color.rgb = starMapColor.rgb*in_starMapBrightness*eyeExtinction;
    out_color.a = 1.0;
}
