
--------------------------------------
--------------------------------------
---- Simple Depth-Of-Field shader. Combines blurred and original
---- texture by distance to focal plane.
--------------------------------------
--------------------------------------
-- vs
#include regen.filter.sampling.vs
-- gs
#include regen.filter.sampling.gs
-- fs
#include regen.states.camera.defines

out vec4 out_color;

uniform sampler2D in_inputTexture;
uniform sampler2D in_blurTexture;
uniform sampler2D in_depthTexture;
uniform vec2 in_inverseViewport;

#include regen.states.camera.input

// DoF input
const float in_focalDistance = 0.0;
const vec2 in_focalWidth = vec2(0.1,0.2);

#include regen.states.camera.linearizeDepth
#include regen.filter.sampling.computeTexco

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    vec4 original = texture(in_inputTexture, texco);
    vec4 blurred = texture(in_blurTexture, texco);
    // get the depth value at this pixel
    float depth = texture(in_depthTexture, texco).r;
    depth = linearizeDepth(depth, __CAM_NEAR__, __CAM_FAR__);
    // distance to point with max sharpness
    float d = abs(in_focalDistance - depth);
    float focus = smoothstep(in_focalWidth.x, in_focalWidth.y, d);
    out_color = mix(original, blurred, focus);
}
