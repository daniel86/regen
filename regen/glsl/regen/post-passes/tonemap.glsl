--------------------------------------
--------------------------------------
---- Tonemapping shader. Based on hdr example from nvidia sdk.
--------------------------------------
--------------------------------------
-- vs
#include regen.post-passes.fullscreen.vs
-- fs
in vec2 in_texco;

uniform sampler2D in_inputTexture;
uniform sampler2D in_blurTexture;

const float in_blurAmount = 0.5;
const float in_effectAmount = 0.2;
const float in_exposure = 16.0;
const float in_gamma = 0.5;
const float in_radialBlurSamples = 30.0;
const float in_radialBlurStartScale = 1.0;
const float in_radialBlurScaleMul = 0.9;
const float in_vignetteInner = 0.7;
const float in_vignetteOuter = 1.5;

out vec4 out_color;

float vignette(vec2 pos, float inner, float outer)
{
  return 1.0 - smoothstep(inner, outer, length(pos));
}

vec4 radialBlur(sampler2D tex, vec2 texcoord, int samples,
        float startScale, float scaleMul)
{
    vec4 c = vec4(0);
    float scale = startScale;
    for(int i=0; i<samples; i++) {
        vec2 texco = ((texcoord-vec2(0.5))*scale)+vec2(0.5);
        vec4 s = texture(tex, texco);
        c += s;
        scale *= scaleMul;
    }
    c /= samples;
    return c;
}

void main() {
    // sum original and blurred image
    out_color = mix(
        texture(in_inputTexture, in_texco),
        texture(in_blurTexture, in_texco), in_blurAmount
    );
    out_color += in_effectAmount * radialBlur(
            in_blurTexture, in_texco,
            int(in_radialBlurSamples),
            in_radialBlurStartScale,
            in_radialBlurScaleMul);
    // exposure and vignette effect
    out_color *= in_exposure * vignette(
        in_texco*2.0-vec2(1.0),
        in_vignetteInner,
        in_vignetteOuter);
    // gamma correction
    out_color.rgb = pow(out_color.rgb, vec3(in_gamma));
}
