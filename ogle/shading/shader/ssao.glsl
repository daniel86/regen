-- vs
#include sampling.vs

-- fs
#include sampling.fsHeader

out float occlusion;

uniform sampler2D in_gDepthTexture;
uniform sampler2D in_gNorWorldTexture;

uniform float in_near;
uniform float in_far;
uniform vec2 in_viewport;
uniform mat4 in_inverseViewProjectionMatrix;

const float in_aoSamplingRadius = 20.0;
const float in_aoBias = 0.05;
const vec2 in_aoAttenuation = vec2(1.0,5.0);

uniform sampler2D in_aoNoiseTexture;

#include utility.texcoToWorldSpace
#include utility.linearizeDepth

#include shading.fetchNormal

#ifndef SIN_45
// 45 degrees = sin(PI / 4)
#define SIN_45 0.707107
#endif

float computeAO__(vec2 texco, vec3 pos0, vec3 nor)
{
    vec3 pos1 = texcoToWorldSpace(texco, texture(in_gDepthTexture,texco).r);
    vec3 dir = pos1 - pos0;
    float dist = length(dir);
    // calculate occlusion intensity
    float i = max(0.0, dot(normalize(dir), nor) - in_aoBias);
    // distance attenuate intensity
    return i / (in_aoAttenuation.x + (in_aoAttenuation.y * dist));
}

void main() {
    vec3 N = fetchNormal(in_texco);
    float depth = texture(in_gDepthTexture, in_texco).r;
    vec3 P = texcoToWorldSpace(in_texco, depth);
    depth = linearizeDepth(depth, in_near, in_far);
    // TODO: texel size uniform
    vec2 texelSize = 1.0/in_viewport*0.5;
    
	vec2 kernel[4] = vec2[](
        vec2( 1, 0), vec2(-1, 0),
        vec2( 0, 1), vec2( 0,-1)
    );
    vec2 kernelRadius = (in_aoSamplingRadius * (1.0 - depth)) * texelSize;

    vec2 randomVec = texture(in_aoNoiseTexture, in_texco).xy;
    randomVec = normalize(randomVec*2.0 - 1.0);
    
    occlusion = 0.0;
    for (int i=0; i<4; ++i)
    {
        vec2 k = reflect(kernel[i], randomVec)*kernelRadius;
        occlusion += computeAO__(in_texco + k, P, N);
        occlusion += computeAO__(in_texco + k*0.5, P, N);

        k = vec2(k.x-k.y, k.x+k.y)*SIN_45;
        occlusion += computeAO__(in_texco + k*0.75, P, N);
        occlusion += computeAO__(in_texco + k*0.25, P, N);
    }
    occlusion = clamp(occlusion/16.0, 0.0, 1.0);
}

