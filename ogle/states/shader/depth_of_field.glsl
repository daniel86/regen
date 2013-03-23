-- vs
in vec3 in_pos;
out vec2 out_texco;

void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
#define DRAW_FOCAL_RANGE 1

in vec2 in_texco;

uniform sampler2D in_inputTexture;
uniform sampler2D in_blurTexture;
uniform sampler2D in_depthTexture;

uniform float in_far;
uniform float in_near;

const float in_focalDistance = 0.0;
const vec2 in_focalWidth = vec2(0.1,0.2);

out vec4 out_color;

#include utility.linearizeDepth

void main() {
    vec4 original = texture(in_inputTexture, in_texco);
    vec4 blurred = texture(in_blurTexture, in_texco);
    // get the depth value at this pixel
    float depth = texture(in_depthTexture, in_texco).r;
    depth = linearizeDepth(depth, in_near, in_far);
    // distance to point with max sharpness
    float d = abs(in_focalDistance - depth);
    float focus = smoothstep(in_focalWidth.x, in_focalWidth.y, d);
    out_color = mix(original, blurred, focus);
}

