
// TODO: use http://www.opengl.org/registry/specs/ARB/draw_buffers_blend.txt ?

-- writeOutputs
void writeOutputs(vec4 color) {
#ifdef USE_AVG_SUM_ALPHA || USE_SUM_ALPHA
    out_color = vec4(color.rgb*color.a,color.a);
#else
    out_color = color;
#endif
#ifdef USE_AVG_SUM_ALPHA
    out_counter = vec2(1.0);
#endif
}

-- avgSum.vs
in vec3 in_pos;
out vec2 out_texco;
void main() {
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- avgSum.fs
out vec4 output;
in vec2 in_texco;

uniform sampler2D in_tColorTexture;
uniform sampler2D in_tCounterTexture;

void main() {
    float alphaCount = texture(in_tCounterTexture, in_texco).x;
    vec4 alphaSum    = texture(in_tColorTexture, in_texco);
    float T = pow(1.0 - alphaSum.a/alphaCount, alphaCount);
    output = vec4(alphaSum.rgb/alphaSum.a, (1.0 - T));
}

