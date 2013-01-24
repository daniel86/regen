-- vs

in vec3 in_pos;
out vec2 out_texco;

void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
#extension GL_EXT_gpu_shader4 : enable

out vec4 output;
in vec2 in_texco;

uniform sampler2D alphaColorTexture;
#ifdef USE_AVG_SUM_ALPHA
uniform sampler2D alphaCounterTexture;
#endif

void main() {
#ifdef USE_SUM_ALPHA
    output = texture(alphaColorTexture, in_texco);
#elif USE_AVG_SUM_ALPHA
    float alphaCount = texture(alphaCounterTexture, in_texco).x;
	vec4 alphaSum = texture(alphaColorTexture, in_texco);
	if(alphaCount>0.9 && alphaSum.a>0.01) {
	    vec3 colorAvg = alphaSum.rgb / alphaSum.a;
	    float alphaAvg = alphaSum.a / alphaCount;
	    float T = pow(1.0-alphaAvg, alphaCount);
	    output = vec4(colorAvg, (1.0 - T));
    } else {
        discard;
    }
#else
    output = texture(alphaColorTexture, in_texco);
#endif
}

