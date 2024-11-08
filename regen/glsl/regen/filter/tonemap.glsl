--------------------------------------
--------------------------------------
---- Tone mapping is a technique to map one set of colors
---- to another in order to approximate the appearance of high dynamic range images
---- in a medium that has a more limited dynamic range.
---- Based on HDR example from nvidia SDK.
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
uniform vec2 in_inverseViewport;
const float in_exposure = 16.0;
const float in_gamma = 0.5;

#ifdef HAS_blurTexture
uniform sampler2D in_blurTexture;
const float in_blurAmount = 0.5;
#endif
#ifdef USE_RADIAL_BLUR
const float in_radialBlurSamples = 30.0;
const float in_radialBlurStartScale = 1.0;
const float in_radialBlurScaleMul = 0.9;
const float in_effectAmount = 0.2;
#endif
#ifdef USE_VIGNETTE
const float in_vignetteInner = 0.7;
const float in_vignetteOuter = 1.5;
#endif

#include regen.filter.sampling.computeTexco

#ifdef USE_VIGNETTE
float vignette(vec2 pos, float inner, float outer)
{
  return 1.0 - smoothstep(inner, outer, length(pos));
}
#endif

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    // sum original and blurred image
    out_color = texture(in_inputTexture, texco);
#ifdef HAS_blurTexture
    out_color.rgb = mix(out_color.rgb,
        texture(in_blurTexture, texco).rgb,
        in_blurAmount);
#endif
#ifdef USE_RADIAL_BLUR
    // Radial Blur
    vec4 blurColor = vec4(0);
    int blurSamples = int(in_radialBlurSamples);
    float blurScale = in_radialBlurStartScale;
    for(int i=0; i<blurSamples; i++) {
        vec2 texco = ((texco_2D-vec2(0.5))*blurScale)+vec2(0.5);
        vec4 s = texture(in_blurTexture, computeTexco(texco));
        blurColor += s;
        blurScale *= in_radialBlurScaleMul;
    }
    blurColor /= blurSamples;
    out_color += in_effectAmount * blurColor;
#endif
#ifdef USE_VIGNETTE
    // Exposure and Vignette effect
    out_color *= in_exposure * vignette(
        texco_2D*2.0-vec2(1.0),
        in_vignetteInner,
        in_vignetteOuter);
#else
    // Overall Exposure
    //out_color *= in_exposure;
    out_color.rgb = vec3(1.0) - exp(-out_color.rgb * in_exposure);
#endif
    // Gamma Correction
    out_color.rgb = pow(out_color.rgb, vec3(in_gamma));
}

