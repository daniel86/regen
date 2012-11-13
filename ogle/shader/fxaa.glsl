-- vs
in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
// FXAA shader, GLSL code adapted from:
// http://horde3d.org/wiki/index.php5?title=Shading_Technique_-_FXAA

const float in_spanMax = 8.0;
const float in_reduceMul = 1.0/8.0;
const float in_reduceMin = 1.0/128.0;

const vec3 in_luma = vec3(0.299, 0.587, 0.114);

in vec2 in_texco;

uniform sampler2D in_inputTexture;
uniform vec2 in_viewport;

out vec4 output;

void main() {
    vec2 viewportInverse = 1.0/in_viewport;
    
    vec3 rgbM  = texture(in_inputTexture, in_texco.xy).xyz;
    vec3 rgbNW = texture(in_inputTexture, in_texco - viewportInverse).xyz;
    vec3 rgbSE = texture(in_inputTexture, in_texco + viewportInverse).xyz;
    vec3 rgbNE = texture(in_inputTexture,
        in_texco.xy + vec2( viewportInverse.x, -viewportInverse.y)).xyz;
    vec3 rgbSW = texture(in_inputTexture,
        in_texco.xy + vec2(-viewportInverse.x,  viewportInverse.y)).xyz;

    float lumaNW = dot(rgbNW, in_luma);
    float lumaNE = dot(rgbNE, in_luma);
    float lumaSW = dot(rgbSW, in_luma);
    float lumaSE = dot(rgbSE, in_luma);
    float lumaM = dot(rgbM, in_luma);
    float lumaMin = min(lumaM,
            min(min(lumaNW, lumaNE),
            min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM,
            max(max(lumaNW, lumaNE),
            max(lumaSW, lumaSE)));

    vec2 dir = vec2(
        -((lumaNW + lumaNE) - (lumaSW + lumaSE)),
         ((lumaNW + lumaSW) - (lumaNE + lumaSE))
    );
    float dirReduce = max(in_reduceMin,
        (lumaNW + lumaNE + lumaSW + lumaSE)*(0.25 * in_reduceMul));

    float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = viewportInverse * min(vec2(in_spanMax), max(vec2(-in_spanMax), dir * rcpDirMin));

    vec3 rgbA = 0.5*(
        texture(in_inputTexture, in_texco.xy - 0.166666666666666*dir).xyz +
        texture(in_inputTexture, in_texco.xy + 0.166666666666666*dir).xyz);
    vec3 rgbB = 0.5*rgbA + 0.25*(
        texture(in_inputTexture, in_texco.xy - 0.5*dir).xyz +
        texture(in_inputTexture, in_texco.xy + 0.5*dir).xyz);
    float lumaB = dot(rgbB, in_luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax)){
        output.xyz=rgbA;
    } else {
        output.xyz=rgbB;
    }
    output.a = 1.0;
}

