

-- sky.vs
in vec3 in_pos;
out vec2 out_texco;
out vec2 out_lightTexco;

uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
uniform mat4 in_viewProjectionMatrix;

uniform vec3 in_lightDirection;

flat out float out_stepScale;
const float in_sunScatteringDensity = 1.0;
const float in_sunScatteringSamples = 40.0;

#include utility.worldSpaceToTexco

void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    vec3 dir = normalize(in_lightDirection);
    out_lightTexco = worldSpaceToTexco(vec4(dir,1.0)).xy;
    out_stepScale = 1.0/(in_sunScatteringSamples*in_sunScatteringDensity);
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- sky.fs
out vec3 output;
in vec2 in_texco;
in vec2 in_lightTexco;

uniform vec3 in_lightDirection;
uniform vec3 in_lightColor;

const float in_sunScatteringExposure = 0.6;
const float in_sunScatteringDecay = 0.9;
const float in_sunScatteringWeight = 0.1;
const float in_sunScatteringDensity = 1.0;
const float in_sunScatteringSamples = 40.0;

bool isBackground(vec2 texco)
{
    return texture(in_depthTexture, texco).x == 1.0;
}

void main()
{
    // ray step size
    vec2 dt = (in_texco-in_lightTexco)*in_stepScale;
    vec2 t = in_texco;
    // distance decay
    float decay = in_sunScatteringWeight;
    // get color at current fragment
    if(isBackground(t)) {
        output = texture(in_colorTexture, t).rgb*decay;
    } else {
        output = vec3(0.0);
    }
    // shoot screen space ray from texco to lightTexco
    for (int i=1; i<in_sunScatteringSamples; i++)
    {
        t -= dt;
        decay *= in_sunScatteringDecay;
        if(decay<0.01) break; // early exit
        if(isBackground(t)) {
            output += texture(in_colorTexture, t).rgb*decay;
        }
    } 
    output *= in_sunScatteringExposure;
}

