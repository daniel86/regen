
--------------------------------------
--------------------------------------
---- Update AO texture
--------------------------------------
--------------------------------------
-- vs
#include regen.filter.sampling.vs
-- gs
#include regen.filter.sampling.gs
-- fs
#include regen.states.camera.defines

out float occlusion;

uniform sampler2D in_gDepthTexture;
uniform sampler2D in_gNorWorldTexture;

#include regen.states.camera.input

const float in_aoSamplingRadius = 20.0;
const float in_aoBias = 0.05;
const vec2 in_aoAttenuation = vec2(1.0,5.0);

uniform sampler2D in_aoNoiseTexture;

#include regen.states.camera.transformTexcoToWorld
#include regen.states.camera.linearizeDepth
#include regen.filter.sampling.computeTexco

#include regen.shading.deferred.fetchNormal

#ifndef SIN_45
// 45 degrees = sin(PI / 4)
#define SIN_45 0.707107
#endif

float computeAO(vec2 texco, vec3 pos0, vec3 nor)
{
    vecTexco _texco = computeTexco(texco);
    vec3 pos1 = transformTexcoToWorld(texco, texture(in_gDepthTexture,_texco).r);
    vec3 dir = pos1 - pos0;
    float dist = length(dir);
    // calculate occlusion intensity
    float i = max(0.0, dot(normalize(dir), nor) - in_aoBias);
    // distance attenuate intensity
    return i / (in_aoAttenuation.x + (in_aoAttenuation.y * dist));
}

void main() {
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    vecTexco texco = computeTexco(texco_2D);
    
    vec3 N = fetchNormal(in_gNorWorldTexture,texco);
    float depth = texture(in_gDepthTexture, texco).r;
    vec3 P = transformTexcoToWorld(texco_2D, depth);
    depth = linearizeDepth(depth, __CAM_NEAR__, __CAM_FAR__);
    vec2 texelSize = in_inverseViewport*0.5;
    
    vec2 kernel[4] = vec2[](
        vec2( 1, 0), vec2(-1, 0),
        vec2( 0, 1), vec2( 0,-1)
    );
    vec2 kernelRadius = (in_aoSamplingRadius * (1.0 - depth)) * texelSize;

    vec2 randomVec = texture(in_aoNoiseTexture, texco_2D).xy;
    randomVec = normalize(randomVec*2.0 - 1.0);
    
    occlusion = 0.0;
    for (int i=0; i<4; ++i)
    {
        vec2 k = reflect(kernel[i], randomVec)*kernelRadius;
        occlusion += computeAO(texco_2D + k, P, N);
        occlusion += computeAO(texco_2D + k*0.5, P, N);

        k = vec2(k.x-k.y, k.x+k.y)*SIN_45;
        occlusion += computeAO(texco_2D + k*0.75, P, N);
        occlusion += computeAO(texco_2D + k*0.25, P, N);
    }
    occlusion = clamp(occlusion/16.0, 0.0, 1.0);
}

--------------------------------------
--------------------------------------
---- Sample AO texture
--------------------------------------
--------------------------------------
-- sample.vs
#include regen.filter.sampling.vs
-- sample.vs
#include regen.filter.sampling.gs
-- sample.fs
out vec3 out_color;

uniform sampler2D in_aoTexture;

#include regen.filter.sampling.computeTexco

void main()
{
    vec2 texco_2D = gl_FragCoord.xy*in_inverseViewport;
    out_color = vec3(1.0-texture(in_aoTexture, computeTexco(texco_2D)).x);
}

