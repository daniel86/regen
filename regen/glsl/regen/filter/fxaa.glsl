--------------------------------------
--------------------------------------
---- FXAA shader, GLSL code adapted from:
---- http://horde3d.org/wiki/index.php5?title=Shading_Technique_-_FXAA
--------------------------------------
--------------------------------------
-- vs
#include regen.filter.sampling.vs
-- vs
#include regen.filter.sampling.gs
-- fs
#include regen.states.camera.defines
out vec4 out_color;

uniform sampler2D in_inputTexture;
// camera input
uniform vec2 in_inverseViewport;
// fxaa input
const float in_spanMax = 8.0;
const float in_reduceMul = 1.0/8.0;
const float in_reduceMin = 1.0/128.0;
const vec3 in_luma = vec3(0.299,0.587,0.114);

#include regen.filter.sampling.computeTexco

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vec3 rgbM  = texture(in_inputTexture,
	computeTexco(texco_2D)).xyz;
    vec3 rgbNW = texture(in_inputTexture,
	computeTexco(texco_2D - in_inverseViewport)).xyz;
    vec3 rgbSE = texture(in_inputTexture,
	computeTexco(texco_2D + in_inverseViewport)).xyz;
    vec3 rgbNE = texture(in_inputTexture,
        computeTexco(texco_2D.xy + vec2( in_inverseViewport.x, -in_inverseViewport.y))).xyz;
    vec3 rgbSW = texture(in_inputTexture,
        computeTexco(texco_2D.xy + vec2(-in_inverseViewport.x,  in_inverseViewport.y))).xyz;

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

    dir = in_inverseViewport * min(vec2(in_spanMax), max(vec2(-in_spanMax), dir * rcpDirMin));

    vec3 rgbA = 0.5*(
        texture(in_inputTexture, computeTexco(texco_2D.xy - 0.166666666666666*dir)).xyz +
        texture(in_inputTexture, computeTexco(texco_2D.xy + 0.166666666666666*dir)).xyz);
    vec3 rgbB = 0.5*rgbA + 0.25*(
        texture(in_inputTexture, computeTexco(texco_2D.xy - 0.5*dir)).xyz +
        texture(in_inputTexture, computeTexco(texco_2D.xy + 0.5*dir)).xyz);
    float lumaB = dot(rgbB, in_luma);

    if((lumaB < lumaMin) || (lumaB > lumaMax)){
        out_color.xyz=rgbA;
    } else {
        out_color.xyz=rgbB;
    }
    out_color.a = 1.0;
}
