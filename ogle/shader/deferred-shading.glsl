
-- ssao.vs
in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- ssao.fs
out float output;

in vec2 in_texco;

// FIXME: perspective camera not a parent
//uniform float in_far;
const float in_far = 200.0;

uniform sampler2D randomNormalTexture;

uniform sampler2D norWorldTexture;
uniform sampler2D posWorldTexture;

const float in_aoSampleRad = 1.0;
const float in_aoBias = 0.0;
const float in_aoConstAttenuation = 1.0;
const float in_aoLinearAttenuation = 1.0;

const float sin45 = 0.707107; // = sin(pi/4)

float calcAO(vec2 texco, vec3 srcPosition, vec3 srcNormal)
{
    // Get the 3D position of the destination pixel
    vec3 dstPosition = texture(posWorldTexture, texco).xyz;
    // Calculate ambient occlusion amount between these two points
    // It is simular to diffuse lighting. Objects directly above the fragment cast
    // the hardest shadow and objects closer to the horizon have minimal effect.
    vec3 positionVec = dstPosition - srcPosition;
    float intensity = max(dot(normalize(positionVec), srcNormal) - in_aoBias, 0.0);
    // Attenuate the occlusion, similar to how you attenuate a light source.
    // The further the distance between points, the less effect AO has on the fragment.
    float dist = length(positionVec);
    float attenuation = 1.0 / (
        in_aoConstAttenuation +
        dist*in_aoLinearAttenuation);
    return intensity * attenuation;
}

void main()
{
    // accumulates occlusion
	output = 0.0;

    vec4 N = texture(norWorldTexture, in_texco);
    // w channel of normal texture is used as mask for GBuffer
    if(N.w<0.1) { return; }
    // map from rgba to [-1,1]
    vec3 srcNormal = N.xyz * 2.0 - vec3(1.0);
    vec3 srcPosition = texture(posWorldTexture, in_texco).xyz;
    float distanceAttenuation = srcPosition.z/in_far;
    // map from rgba to [-1,1]
	vec2 randVec = normalize(texture(randomNormalTexture, in_texco).xy*2.0 - vec2(1.0));

    // sample neighbouring pixels
    // pixels far off in the distance will not sample as many pixels as those close up.
 	vec2 kernelRadius = vec2(in_aoSampleRad * (1.0 - distanceAttenuation));///in_viewport;
	vec2 kernel[4];
	kernel[0] = vec2( kernelRadius.x,  0); // right
	kernel[1] = vec2(-kernelRadius.x,  0); // left
	kernel[2] = vec2( 0,  kernelRadius.y); // top
	kernel[3] = vec2( 0, -kernelRadius.y); // bottom

	// sample max 16 pixels
	int iterations = int(mix(4.0, 1.0, distanceAttenuation));
	for (int i = 0; i < iterations; ++i) {
		vec2 k1 = reflect(kernel[i], randVec);
		vec2 k2 = vec2(k1.x*sin45 - k1.y*sin45, k1.x*sin45 + k1.y*sin45);
		output += calcAO(in_texco + k1,      srcPosition, srcNormal);
		output += calcAO(in_texco + k2*0.75, srcPosition, srcNormal);
		output += calcAO(in_texco + k1*0.5,  srcPosition, srcNormal);
		output += calcAO(in_texco + k2*0.25, srcPosition, srcNormal);
	}
    // average ambient occlusion
    output /= iterations*4.0;
}

-- vs

in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
#include light.defines

//#define DRAW_OCCLUSION
//#define USE_AMBIENT_OCCLUSION

out vec4 output;

in vec2 in_texco;

uniform sampler2D colorTexture;
uniform sampler2D specularTexture;
uniform sampler2D norWorldTexture;
#ifdef HAS_POS_TEXTURE
uniform sampler2D posWorldTexture;
#else
// TODO: add depthTexture uniform....
uniform sampler2D depthTexture;
// TODO: add camera uniforms....
uniform mat4 in_inverseViewProjectionMatrix;
#endif
#ifdef USE_AMBIENT_OCCLUSION
uniform sampler2D aoTexture;
#endif
#ifdef HAS_FOG
// TODO: fog texture
uniform vec4 in_fogColor;
uniform float in_fogEnd;
uniform float in_fogScale;
#endif

#ifdef HAS_LIGHT
#include light.input

#include light.init

#include light.apply
#endif

#ifndef HAS_POS_TEXTURE
vec4 worldPosFromDepth()
{
    // get the depth value at this pixel
    float depth = texture(depthTexture, in_texco).r;
    vec4 pos0 = vec4(in_texco.x*2 - 1, (1-in_texco.y)*2 - 1, depth, 1);
    // Transform viewport position by the view-projection inverse.
    vec4 D = in_inverseViewProjectionMatrix*pos0;
    // Divide by w to get the world position.
    return D/D.w;
}
#endif

void main() {
#ifdef USE_AMBIENT_OCCLUSION
    float ambientOcclusion = 1.0-texture(aoTexture, in_texco).x;
/*
    if(ambientOcclusion<0.001) {
        output = vec4(0.0);
        return;
    }
*/
#endif

    output = texture(colorTexture, in_texco);

#ifdef HAS_LIGHT
    vec4 N = texture(norWorldTexture, in_texco);
    if(N.w<0.1) { return; }
    // map to [-1,1]
    vec3 norWorld = N.xyz * 2.0 - vec3(1.0);

#ifdef HAS_POS_TEXTURE
    vec3 posWorld = texture(posWorldTexture, in_texco).xyz;
#else
    vec3 posWorld = worldPosFromDepth().xyz;
#endif
    vec4 matSpecular = texture(specularTexture, in_texco);
    vec4 specular = vec4(0.0);
    vec4 emission = vec4(0.0);
    vec4 ambient = vec4(0.0);
    vec4 diffuse = vec4(0.0);
    vec3 lightVec = vec3(0.0);
    float attenuation = 0.0;
    float shininess = matSpecular.a;
    specular.a = 0.0;

#for NUM_LIGHTS
#define2 __ID ${LIGHT${FOR_INDEX}_ID}
    shadeTransfer${__ID}(lightVec, attenuation, posWorld);
    shade${__ID}(lightVec, attenuation,
        ambient, diffuse, specular, emission, shininess,
        posWorld, norWorld.xyz);
#endfor

  #ifdef DRAW_OCCLUSION && USE_AMBIENT_OCCLUSION
    output = vec4(ambientOcclusion);
  #else
    output = output*(emission + ambient + diffuse) + vec4(matSpecular.rgb,1.0)*specular;
    #ifdef USE_AMBIENT_OCCLUSION
    output *= ambientOcclusion;
    #endif
  #endif

#endif // HAS_LIGHT

#ifdef HAS_FOG
    // apply fog
    float fogVar = clamp(in_fogScale*(in_fogEnd + gl_FragCoord.z), 0.0, 1.0);
    out_color = mix(in_fogColor, out_color, fogVar);
#endif
}

