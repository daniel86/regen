-- vs
in vec3 in_pos;
out vec2 out_texco;

void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
#define DRAW_FOCAL_RANGE 0

in vec2 in_texco;

uniform sampler2D in_inputTexture;
uniform sampler2D in_blurTexture;
uniform sampler2D in_depthTexture;

uniform float in_far;
uniform float in_near;

const float in_focalDistance = 10.0;
const float in_focalWidth = 2.5;
const float in_blurRange = 5.0;

out vec4 output;

float linearize(float d, float far, float near) {
    float z_n = 2.0*d - 1.0;
    return 2.0*near*far/(far+near-z_n*(far-near));
}

void main() {
    // get the depth value at this pixel
    float depth = texture(in_depthTexture, in_texco).r;
    depth = linearize(depth, in_far, in_near);
    // get original pixel
    vec4 original = texture(in_inputTexture, in_texco);
#if DRAW_FOCAL_RANGE==1
    original *= vec4(0.0,1.0,0.0,1.0);
#endif

    float d = abs(in_focalDistance-depth);
    if(d<=in_focalWidth) {
        output = original;
    }
    else {
        // get blurred pixel
        vec4 blurred = texture(in_blurTexture, in_texco);
#if DRAW_FOCAL_RANGE==1
        blurred *= vec4(1.0,0.0,0.0,1.0);
#endif
        output = mix(original, blurred,
            min(1.0, (d-in_focalWidth)/in_blurRange));
    }
}

