
--------------------------------------
---- T-buffer output targets depends on the used transparency mode.
--------------------------------------
-- writeOutputs
void writeOutputs(vec4 color) {
#ifdef USE_AVG_SUM_ALPHA || USE_SUM_ALPHA
    out_color = vec4(color.rgb*color.a,color.a);
#else
    out_color = color;
#endif
#ifdef USE_AVG_SUM_ALPHA
    // increase counter by 1 for each sample
    out_counter = vec2(1.0);
#endif
}

--------------------------------------
--------------------------------------
---- Computes average transparency from previously
---- computed T-buffer.
--------------------------------------
--------------------------------------
-- avgSum.vs
#include regen.post-pass.fullscreen.vs
-- avgSum.fs
out vec4 out_color;
in vec2 in_texco;

// T-buffer input
uniform sampler2D in_tColorTexture;
uniform sampler2D in_tCounterTexture;

void main()
{
    float alphaCount = texture(in_tCounterTexture, in_texco).x;
    vec4 alphaSum = texture(in_tColorTexture, in_texco);
    float T = pow(1.0 - alphaSum.a/alphaCount, alphaCount);
    out_color = vec4(alphaSum.rgb/alphaSum.a, (1.0 - T));
}

