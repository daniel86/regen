-- vs
in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
in vec2 in_texco;

uniform sampler2D in_inputTexture;
uniform sampler2D in_blurTexture;

const float in_blurAmount = 0.5;
const float in_effectAmount = 0.2;
const float in_exposure = 16.0;
const float in_gamma = 0.5;

out vec4 output;

float vignette(vec2 pos, float inner, float outer)
{
  float r = length(pos);
  r = 1.0 - smoothstep(inner, outer, r);
  return r;
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
    output = mix(
        texture(in_inputTexture, in_texco),
        texture(in_blurTexture, in_texco), in_blurAmount
    );
    output += in_effectAmount * radialBlur(in_blurTexture, in_texco, 30, 1.0, 0.9);
    // exposure and vignette effect
    output *= in_exposure * vignette(in_texco*2.0-vec2(1.0), 0.7, 1.5);
    // gamma correction
    output.rgb = pow(output.rgb, vec3(in_gamma));
}

