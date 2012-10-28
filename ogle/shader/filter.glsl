
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

