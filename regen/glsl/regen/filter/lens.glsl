
----------------------------------------
-- Lens flare effect as a post-process, i.e. a screen-space effect.
-- Input: Scene color (downsampled), and ghost gradient texture.
-- Output: Lens flare effect.
--
-- The output should be blurred for a more realistic effect, e.g. with size 12 and step 1.5.
-- Then it can be combined with a dirt texture and a starburst texture.
-- The result can be added to the scene color, i.e. using the additive blending mode.
--
-- @see https://john-chapman.github.io/2017/11/05/pseudo-lens-flare.html
----------------------------------------
-- flare.vs
#include regen.filter.sampling.vs
-- flare.gs
#include regen.filter.sampling.gs
-- flare.fs
#include regen.states.camera.defines
//#define DISABLE_CHROMATIC_ABERRATION
//#define DISABLE_HALO_ASPECT_RATIO
#define GHOST_TINT_PER_SAMPLE

uniform sampler2D in_inputTexture;
#ifdef GHOST_TINT_PER_SAMPLE
uniform sampler2D in_ghostGradient;
#endif

// lod index
const float in_downsample = 2;
// number of ghost samples
const int in_numGhosts = 5;
const float in_ghostSpacing = 0.4;
const float in_ghostThreshold = 9.0;
const float in_haloThickness = 0.1;
const float in_haloRadius = 0.6;
const float in_haloThreshold = 9.0;
const float in_chromaticAberration = 0.003;

out vec3 out_color;

#include regen.filter.sampling.computeTexco

vec3 SampleSceneColor(in vec2 uv) {
#if DISABLE_CHROMATIC_ABERRATION
	return textureLod(in_inputTexture, uv, in_downsample).rgb;
#else
	vec2 offset = normalize(vec2(0.5) - uv) * in_chromaticAberration;
	return vec3(
		textureLod(in_inputTexture, uv + offset, in_downsample).r,
		textureLod(in_inputTexture, uv, in_downsample).g,
		textureLod(in_inputTexture, uv - offset, in_downsample).b
		);
#endif
}

vec3 ApplyThreshold(in vec3 rgb, in float threshold) {
    // isolate bright areas in the source image
	return max(rgb - vec3(threshold), vec3(0.0));
}

vec3 SampleGhosts(in vec2 uv, in float threshold) {
	vec3 ret = vec3(0.0);
	vec2 ghostVec = (vec2(0.5) - uv) * in_ghostSpacing;
	for (int i = 0; i < in_numGhosts; ++i) {
	    // sample scene color
		vec2 suv = fract(uv + ghostVec * vec2(i));
		vec3 s = SampleSceneColor(suv);
		s = ApplyThreshold(s, threshold);
		// tint/weight
		float distanceToCenter = distance(suv, vec2(0.5));
#ifdef GHOST_TINT_PER_SAMPLE
        // incorporate weight into tint gradient
        s *= texture(in_ghostGradient, vec2(distanceToCenter, 0.5)).rgb;
#else
        // analytical weight
        float weight = 1.0 - smoothstep(0.0, 0.75, distanceToCenter);
        s *= weight;
#endif
		ret += s;
	}
#ifndef GHOST_TINT_PER_SAMPLE
    ret *= texture(in_ghostGradient, vec2(distance(uv, vec2(0.5)), 0.5)).rgb;
#endif
	return ret;
}

// Cubic window; map [0, r] in [1, 0] as a cubic falloff from c.
float Window_Cubic(float x, float c, float r) {
	x = min(abs(x - c) / r, 1.0);
	return 1.0 - x * x * (3.0 - 2.0 * x);
}

vec3 SampleHalo(in vec2 uv, in float radius, in float aspectRatio, in float threshold) {
	vec2 haloVec = vec2(0.5) - uv;
#ifdef DISABLE_HALO_ASPECT_RATIO
    haloVec = normalize(haloVec);
    float haloWeight = distance(uv, vec2(0.5));
#else
    haloVec.x /= aspectRatio;
    haloVec = normalize(haloVec);
    haloVec.x *= aspectRatio;
    vec2 wuv = (uv - vec2(0.5, 0.0)) / vec2(aspectRatio, 1.0) + vec2(0.5, 0.0);
    float haloWeight = distance(wuv, vec2(0.5));
#endif
	haloVec *= radius;
	haloWeight = Window_Cubic(haloWeight, radius, in_haloThickness);
	return ApplyThreshold(SampleSceneColor(uv + haloVec), threshold) * haloWeight;
}

void main() {
    vecTexco uv = computeTexco(gl_FragCoord.xy*in_inverseViewport);
    // flip the texture coordinates
	uv = vec2(1.0) - uv;

	vec3 ret = vec3(0.0);
	float aspect = REGEN_CAM_PARAMS_(0).z;
	ret += SampleGhosts(uv, in_ghostThreshold);
	ret += SampleHalo(uv, in_haloRadius, 1.0/aspect, in_haloThreshold);

	out_color = ret;
}

-- dirt.vs
#include regen.filter.sampling.vs
-- dirt.gs
#include regen.filter.sampling.gs
-- dirt.fs
#include regen.states.camera.defines
uniform sampler2D in_inputTexture;
uniform sampler2D in_lensDirtTexture;
uniform sampler2D in_starburstTexture;

const float in_globalBrightness = 0.05;

out vec3 out_color;

#include regen.filter.sampling.computeTexco

void main() {
    vecTexco uv = computeTexco(gl_FragCoord.xy*in_inverseViewport);
    float starburstOffset = in_cameraPosition.x + in_cameraPosition.y + in_cameraPosition.z;
    // starburst
	vec2 centerVec = uv - vec2(0.5);
	float d = length(centerVec);
	float radial = acos(centerVec.x / d);
	float mask =
		  texture(in_starburstTexture, vec2(radial + starburstOffset * 1.0, 0.0)).r
		* texture(in_starburstTexture, vec2(radial - starburstOffset * 0.5, 0.0)).r;
	mask = clamp(mask + (1.0 - smoothstep(0.0, 0.3, d)), 0.0, 1.0);
	// lens dirt
	mask *= textureLod(in_lensDirtTexture, uv, 0.0).r;
	// features
	out_color = mask * in_globalBrightness * textureLod(in_inputTexture, uv, 0.0).rgb;
}
