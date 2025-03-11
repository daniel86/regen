
-- vs
#include regen.filter.sampling.vs
-- gs
#include regen.filter.sampling.gs
-- fs
#include regen.states.camera.defines

uniform vec2 in_inverseViewport;
uniform sampler2D in_gDepthTexture;
uniform sampler2D in_rDepthTexture;
uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_rColorTexture;
out vec4 out_color;

const float in_refractionScale = 0.1;
const float in_refractionStrength = 0.5;

#include regen.filter.sampling.computeTexco

void main()
{
    vec2 uv = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(uv);
    float rd = texture(in_gDepthTexture, uv).r + 0.01;

    vec3 N = 2.0*texture(in_gNorWorldTexture, uv).rgb - vec3(1.0);
    // compute TBN matrix
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    if (abs(N.z) > 0.999) {
        up = vec3(1.0, 0.0, 0.0);
    }

    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    vec3 N_t = normalize(TBN * N);
    vec2 r_uv = uv + N_t.xy * in_refractionScale;

#define2 _RD_ID ${TEX_ID_rDepthTexture}
    vec3 rc = vec3(0.0);
    float rd_x;
    bool is_min;
#for RD_INDEX to ${TEX_DEPTH${_RD_ID}}
    rd_x = texture(in_rDepthTexture, vec3(uv,${RD_INDEX})).r;
    is_min = rd_x < rd;
    rd = mix(rd, rd_x, float(is_min));
    rc = mix(rc, texture(in_rColorTexture, vec3(r_uv,${RD_INDEX})).rgb, float(is_min));
#endfor
    // set alpha to zero in case rd is 1.0
    float alpha = in_refractionStrength*float(rd < 1.0);

    out_color = vec4(rc,alpha);
}
