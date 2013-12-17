
-- avgSum.vs
#include regen.filter.sampling.vs
-- avgSum.gs
#include regen.filter.sampling.gs
-- avgSum.fs
out vec4 out_color;

// T-buffer input
uniform sampler2D in_tColorTexture;
uniform sampler2D in_tCounterTexture;

#include regen.filter.sampling.computeTexco

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    float alphaCount = texture(in_tCounterTexture, texco).x;
    vec4 alphaSum = texture(in_tColorTexture, texco);
    float T = pow(1.0 - alphaSum.a/alphaCount, alphaCount);
    out_color = vec4(alphaSum.rgb/alphaSum.a, (1.0 - T));
}

