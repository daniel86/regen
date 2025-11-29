
-- sobel.vs
#include regen.filter.sampling.vs
-- sobel.gs
#include regen.filter.sampling.gs
-- sobel.fs
#include regen.states.camera.defines

out vec4 out_color;

uniform sampler2D in_inputTexture;
uniform vec2 in_inverseViewport;

#include regen.filter.sampling.computeTexco

#ifdef FLIP_X
    #ifdef FLIP_Y
vec2 uvFlip(vec2 uv) { return vec2(1.0 - uv.x, 1.0 - uv.y); }
    #else
vec2 uvFlip(vec2 uv) { return vec2(1.0 - uv.x, uv.y); }
    #endif
#elif defined(FLIP_Y)
vec2 uvFlip(vec2 uv) { return vec2(uv.x, 1.0 - uv.y); }
#else
#define uvFlip(uv) (uv)
#endif

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco uv = computeTexco(texco_2D);

    vec2 texel = vec2(TEX_TEXEL_X${TEX_ID_inputTexture}, TEX_TEXEL_Y${TEX_ID_inputTexture});
    float tl = texture(in_inputTexture, uvFlip(uv + texel * vec2(-1,  1))).r;
    float  t = texture(in_inputTexture, uvFlip(uv + texel * vec2( 0,  1))).r;
    float tr = texture(in_inputTexture, uvFlip(uv + texel * vec2( 1,  1))).r;
    float  l = texture(in_inputTexture, uvFlip(uv + texel * vec2(-1,  0))).r;
    float  c = texture(in_inputTexture, uvFlip(uv)).r;
    float  r = texture(in_inputTexture, uvFlip(uv + texel * vec2( 1,  0))).r;
    float bl = texture(in_inputTexture, uvFlip(uv + texel * vec2(-1, -1))).r;
    float  b = texture(in_inputTexture, uvFlip(uv + texel * vec2( 0, -1))).r;
    float br = texture(in_inputTexture, uvFlip(uv + texel * vec2( 1, -1))).r;

    float sum = tl + t + tr + l + c + r + bl + b + br;

    float gx = (tr + 2.0*r + br) - (tl + 2.0*l + bl);
    float gy = (tl + 2.0*t + tr) - (bl + 2.0*b + br);

    vec2 grad = vec2(gx, gy);
    grad = normalize(grad + 1e-5);
    grad *= step(1e-4, sum);

    out_color = vec4(-grad, c, 1.0);
}
