
-- sepia
const float in_sepiaDesaturate = 0.0;
const float in_sepiaToning = 1.0;
const vec3 in_lightColor = vec3(1.0, 0.9, 0.5);
const vec3 in_darkColor = vec3(0.2, 0.05, 0.0);
const vec3 in_grayXfer = vec3(0.3, 0.59, 0.11);

vec4 sepia(sampler2D tex, vec2 texco)
{
    vec3 scnColor = in_lightColor * texel(tex, texco).rgb;
    float gray = dot( in_grayXfer, scnColor );
    vec3 muted = mix( scnColor, vec3(gray), in_sepiaDesaturate );
    vec3 sepia = mix( in_darkColor, in_lightColor, gray );
    return vec4( mix( muted, sepia, in_sepiaToning ), 1.0 );
}

-- greyScale
void greyScale(sampler2D tex, vec2 texco)
{
    vec4 texcolor = texel(tex, texco);
    float gray = dot( texcolor.rgb, vec3(0.299, 0.587, 0.114) );
    vec4 col = vec4(gray, gray, gray, texcolor.a);
    return col;
}

-- tiles
uniform float in_tilesVal;
void tiles(sampler2D tex, vec2 texco)
{
    vec2 tiledUV = texco - mod(texco, vec2(in_tilesVal)) + 0.5*vec2(in_tilesVal);
    return texel(tex, tiledUV);
}


-- bloom
vec4 bloom(sampler2D tex, vec2 texco)
{
    vec4 center = texel(tex, texco);
    vec4 col = vec4(0.0);
  s << convolutionCode_ << endl;
    if (col.r < 0.3) {
        col = col*col*0.06 + center;
    } else if (col.r < 0.5) {
        col = col*col*0.045 + center;
    } else {
        col = col*col*0.037 + center;
    }
    return col;
}

-- tonemap

uniform sampler2D in_blurTexture;
uniform sampler2D in_sceneTexture;
uniform float in_blurAmount;
uniform float in_effectAmount;
uniform float in_exposure;
uniform float in_gamma;

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

void tonemap(sampler2D originalTex, sampler2D blurTex, out vec4 color)
{
    // sum original and blurred image
    vec4 c = mix( texture(originalTex, in_texco), texture(blurTex, in_texco), in_blurAmount );
    c += radialBlur(blurTex, in_texco, 30, 1.0, 0.95)*in_effectAmount;
    // exposure and vignette effect
    c *= in_exposure * vignette(in_texco*2.0-vec2(1.0), 0.7, 1.5);
    // gamma correction
    c.rgb = pow(c.rgb, vec3(in_gamma));
    color = c;
}


