
----------
-- Based on https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom
----------
-- downsample.vs
#include regen.filter.sampling.vs
-- downsample.fs
#version 430 core

layout (location = 0) out vec3 out_color;
layout (binding = 0) uniform sampler2D in_inputTexture;
uniform vec2 in_inverseInputSize;
uniform vec2 in_inverseViewport;

void main()
{
    vec2 texCoord = gl_FragCoord.xy*in_inverseViewport;
    float x = in_inverseInputSize.x*1.5;
    float y = in_inverseInputSize.y*1.5;

    // Take 13 samples around current texel:
    vec3 a = texture(in_inputTexture, vec2(texCoord.x - 2*x, texCoord.y + 2*y)).rgb;
    vec3 b = texture(in_inputTexture, vec2(texCoord.x,       texCoord.y + 2*y)).rgb;
    vec3 c = texture(in_inputTexture, vec2(texCoord.x + 2*x, texCoord.y + 2*y)).rgb;
    vec3 d = texture(in_inputTexture, vec2(texCoord.x - 2*x, texCoord.y)).rgb;
    vec3 e = texture(in_inputTexture, vec2(texCoord.x,       texCoord.y)).rgb;
    vec3 f = texture(in_inputTexture, vec2(texCoord.x + 2*x, texCoord.y)).rgb;
    vec3 g = texture(in_inputTexture, vec2(texCoord.x - 2*x, texCoord.y - 2*y)).rgb;
    vec3 h = texture(in_inputTexture, vec2(texCoord.x,       texCoord.y - 2*y)).rgb;
    vec3 i = texture(in_inputTexture, vec2(texCoord.x + 2*x, texCoord.y - 2*y)).rgb;
    vec3 j = texture(in_inputTexture, vec2(texCoord.x - x, texCoord.y + y)).rgb;
    vec3 k = texture(in_inputTexture, vec2(texCoord.x + x, texCoord.y + y)).rgb;
    vec3 l = texture(in_inputTexture, vec2(texCoord.x - x, texCoord.y - y)).rgb;
    vec3 m = texture(in_inputTexture, vec2(texCoord.x + x, texCoord.y - y)).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
    vec3 downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;
    downsample = max(downsample, 0.0001f);

    out_color = downsample;
}

----------
-- Based on https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom
----------
-- upsample.vs
#include regen.filter.sampling.vs
-- upsample.gs
#include regen.filter.sampling.gs
-- upsample.fs
#version 430 core

layout (location = 0) out vec3 out_color;
layout (binding = 0) uniform sampler2D in_inputTexture;
const float in_filterRadius = 1.0f;
uniform vec2 in_inverseViewport;

void main()
{
    vec2 texCoord = gl_FragCoord.xy*in_inverseViewport;

    // The filter kernel is applied with a radius, specified in texture
    // coordinates, so that the radius will vary across mip resolutions.
    const float aspectRatio = in_inverseViewport.y / in_inverseViewport.x;
    float x = in_filterRadius;
    float y = in_filterRadius * aspectRatio;

    // Take 9 samples around current texel:
    vec3 a = texture(in_inputTexture, vec2(texCoord.x - x, texCoord.y + y)).rgb;
    vec3 b = texture(in_inputTexture, vec2(texCoord.x,     texCoord.y + y)).rgb;
    vec3 c = texture(in_inputTexture, vec2(texCoord.x + x, texCoord.y + y)).rgb;
    vec3 d = texture(in_inputTexture, vec2(texCoord.x - x, texCoord.y)).rgb;
    vec3 e = texture(in_inputTexture, vec2(texCoord.x,     texCoord.y)).rgb;
    vec3 f = texture(in_inputTexture, vec2(texCoord.x + x, texCoord.y)).rgb;
    vec3 g = texture(in_inputTexture, vec2(texCoord.x - x, texCoord.y - y)).rgb;
    vec3 h = texture(in_inputTexture, vec2(texCoord.x,     texCoord.y - y)).rgb;
    vec3 i = texture(in_inputTexture, vec2(texCoord.x + x, texCoord.y - y)).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    vec3 upsample = e*4.0;
    upsample += (b+d+f+h)*2.0;
    upsample += (a+c+g+i);
    upsample *= 1.0 / 16.0;

    out_color = upsample;
}


--------------------------------------
--------------------------------------
-- threshold.vs
#include regen.filter.sampling.vs
-- threshold.gs
#include regen.filter.sampling.gs
-- threshold.fs
out vec4 out_color;

uniform sampler2D in_colorTexture;
uniform vec2 in_inverseViewport;

#include regen.filter.sampling.computeTexco

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    vec3 original = texture(in_colorTexture, texco).rgb;
    // check whether fragment output is higher than threshold, if so output as brightness color
    float brightness = dot(original, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 0.9) {
        out_color = vec4(original, 1.0);
    }
    else {
        out_color = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
