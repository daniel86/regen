
-- vs
in vec3 in_pos;
out vec2 out_texco;
void main()
{
    out_texco = 0.5*(in_pos.xy+vec2(1.0));
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- fs
out float output;
in vec2 in_texco;

uniform sampler2D in_randomNorTexture;
uniform sampler2D in_norWorldTexture;
uniform sampler2D in_posWorldTexture;

const float in_far = 200.0;

const float in_aoSampleRad = 1.0;
const float in_aoBias = 0.0;
const float in_aoConstAttenuation = 1.0;
const float in_aoLinearAttenuation = 1.0;

const float sin45 = 0.707107; // = sin(pi/4)

float calcAO(vec2 texco, vec3 srcPosition, vec3 srcNormal)
{
    // Get the 3D position of the destination pixel
    vec3 dstPosition = texture(in_posWorldTexture, texco).xyz;
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
    vec3 srcPosition = texture(in_posWorldTexture, in_texco).xyz;
    float distanceAttenuation = srcPosition.z/in_far;
    // map from rgba to [-1,1]
	vec2 randVec = normalize(texture(in_randomNorTexture, in_texco).xy*2.0 - vec2(1.0));

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

