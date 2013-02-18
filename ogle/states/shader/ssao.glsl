
-- vs
in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
out vec3 output;
in vec2 in_texco;

uniform sampler2D in_randomNorTexture;

uniform sampler2D in_gNorWorldTexture;
uniform sampler2D in_gDepthTexture;

uniform vec3 in_cameraPosition;
uniform mat4 in_inverseViewProjectionMatrix;

const float in_far = 200.0;

const float in_aoSampleRad = 1.0;
const float in_aoBias = 0.0;
const float in_aoConstAttenuation = 1.0;
const float in_aoLinearAttenuation = 1.0;

const float sin45 = 0.707107; // = sin(pi/4)

#include shading.fetchNormal
#include shading.fetchPosition

float calcAO(vec2 texco, vec3 srcPosition, vec3 srcNormal)
{
    // Get the 3D position of the destination pixel
    vec3 dstPosition = fetchPosition(texco);
    // Calculate ambient occlusion amount between these two points
    // It is simular to diffuse lighting. Objects directly above the fragment cast
    // the hardest shadow and objects closer to the horizon have minimal effect.
    vec3 positionVec = dstPosition - srcPosition;
    float intensity = max(dot(normalize(positionVec), srcNormal) - in_aoBias, 0.0);
    // Attenuate the occlusion, similar to how you attenuate a light source.
    // The further the distance between points, the less effect AO has on the fragment.
    float dist = length(positionVec);
    float attenuation = 1.0 / (in_aoConstAttenuation + dist*in_aoLinearAttenuation);
    return intensity * attenuation;
}

void main()
{
    vec3 srcNormal = fetchNormal(in_texco);
    vec3 srcPosition = fetchPosition(in_texco);
    float distanceAttenuation = srcPosition.z/in_far;
	vec2 randVec = normalize(
        texture(in_randomNorTexture, in_texco).xy*2.0 - vec2(1.0));

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
	float ao = 0.0;
	for (int i = 0; i < iterations; ++i) {
		vec2 k1 = reflect(kernel[i], randVec);
		vec2 k2 = vec2(k1.x*sin45 - k1.y*sin45, k1.x*sin45 + k1.y*sin45);
		ao += calcAO(in_texco + k1,      srcPosition, srcNormal);
		ao += calcAO(in_texco + k2*0.75, srcPosition, srcNormal);
		ao += calcAO(in_texco + k1*0.5,  srcPosition, srcNormal);
		ao += calcAO(in_texco + k2*0.25, srcPosition, srcNormal);
	}
    // average ambient occlusion
    ao /= iterations*4.0;
    output = vec3(1.0-ao);
}

