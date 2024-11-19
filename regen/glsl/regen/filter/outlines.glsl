--------------------------------------
--------------------------------------
---- A shader that draws outlines around objects in a post-processing step.
---- Supports using depth and normal buffers.
--------------------------------------
--------------------------------------
-- defines
#ifndef OUTLINE_MODE_DEPTH
    #ifdef HAS_gDepthTexture
#define OUTLINE_MODE_DEPTH
    #endif
#endif
#ifndef OUTLINE_MODE_NORMAL
    #ifdef HAS_gNorWorldTexture
#define OUTLINE_MODE_NORMAL
    #endif
#endif
-- vs
#include regen.filter.sampling.vs
-- vs
#include regen.filter.sampling.gs
-- fs
#include regen.filter.outlines.defines
#include regen.states.camera.defines

// input from the vertex shader
out vec4 out_color;
// user input
const float in_outlineWidth = 1.0;
const vec3 in_outlineColor = vec3(0.0, 0.0, 0.0);

#include regen.filter.sampling.computeTexco
#ifdef OUTLINE_MODE_NORMAL
    #include regen.filter.outlines.outline_nor.simple
#endif // OUTLINE_MODE_NORMAL
#ifdef OUTLINE_MODE_DEPTH
    #include regen.filter.outlines.outline_depth.simple
#endif // OUTLINE_MODE_DEPTH

void main()
{
    vecTexco texco = computeTexco(gl_FragCoord.xy*in_inverseViewport);
    float outline = 0.0;
#ifdef HAS_gDepthTexture
    // adjust the outline width based on the depth, make outlines thinner in the distance.
    // also clamp away the far distance, as we only want to transition the outline width
    // in the near distance, also make sure the outline does not disappear too early.
    const float outlineFarCutoff = 0.7;
    const float outlineMinWidth = 0.1;
    // linearizedDepth is in the range [0, 1], 0 is near, 1 is far
    float linearizedDepth = clamp(depthSample(texco), 0.0, outlineFarCutoff)/outlineFarCutoff;
    // closenessFactor is in the range [outlineMinWidth, 1.0], 1.0 is near, outlineMinWidth is far
    float closenessFactor = (1.0-linearizedDepth)*(1.0-outlineMinWidth) + outlineMinWidth;
    vec2 step = in_inverseViewport * in_outlineWidth * closenessFactor;
#else
    vec2 step = in_inverseViewport * in_outlineWidth;
#endif

#ifdef OUTLINE_MODE_DEPTH
	outline = max(outline, outline_depth(texco, step));
#endif // OUTLINE_MODE_DEPTH
#ifdef OUTLINE_MODE_NORMAL
	outline = max(outline, outline_nor(texco, step));
#endif // OUTLINE_MODE_NORMAL
#ifdef OUTLINE_MODE_COLOR
	outline = max(outline, outline_col(texco, step));
#endif // OUTLINE_MODE_COLOR

    if (outline > 0.004) {
        out_color = vec4(in_outlineColor, outline);
    } else {
        discard;
    }
}

-- outline_depth.defs
#ifndef REGEN_outline_depth_defs_Included_
#define2 REGEN_outline_depth_defs_Included_
uniform sampler2D in_gDepthTexture;
const float in_outlineThresholdDepth = 0.4;
// depth is stored non-linearly, need to convert to linear
#include regen.states.camera.linearizeDepth
#define depthSample(uv) linearizeDepth(\
        texture(in_gDepthTexture, uv).r, REGEN_CAM_NEAR_(in_layer), REGEN_CAM_FAR_(in_layer))
#endif

-- outline_depth.simple
#ifndef REGEN_outline_depth_simple_Included_
#define2 REGEN_outline_depth_simple_Included_
#include regen.filter.outlines.outline_depth.defs

float outline_depth(vec2 texco, vec2 offset) {
    // fetch depth values from 4 neighboring pixels, and linearize them
    float d0 = depthSample(texco - offset);
    float d1 = depthSample(texco + offset);
    float d2 = depthSample(texco + vec2( offset.x, -offset.y));
    float d3 = depthSample(texco + vec2(-offset.x,  offset.y));
    // compute the "Roberts cross" edge detection, and compare with threshold
    return float(
        sqrt(pow(d1 - d0, 2) + pow(d3 - d2, 2)) >
        in_outlineThresholdDepth / 100.0);
}
#endif

-- outline_nor.defs
#ifndef REGEN_outline_nor_defs_Included_
#define2 REGEN_outline_nor_defs_Included_
uniform sampler2D in_gNorWorldTexture;
const float in_outlineThresholdNor = 0.04;
// normal is stored in the range [0, 1], need to convert to [-1, 1]
#define norSample(uv) 2.0*texture(in_gNorWorldTexture, uv).rgb - vec3(1.0)
#endif

-- outline_nor.simple
#ifndef REGEN_outline_nor_simple_Included_
#define2 REGEN_outline_nor_simple_Included_
#include regen.filter.outlines.outline_nor.defs

float outline_nor(vec2 texco, vec2 offset) {
    // fetch normal values from 4 neighboring pixels, and linearize them
    vec3 n0 = norSample(texco - offset);
    vec3 n1 = norSample(texco + offset);
    vec3 n2 = norSample(texco + vec2( offset.x, -offset.y));
    vec3 n3 = norSample(texco + vec2(-offset.x,  offset.y));
    vec3 dn0 = n1 - n0;
    vec3 dn1 = n3 - n2;
    float edgeNormal = sqrt(dot(dn0, dn0) + dot(dn1, dn1));
    return float(edgeNormal > in_outlineThresholdNor);
}
#endif

-- outline_nor.sobel
#ifndef REGEN_outline_nor_sobel_Included_
#define2 REGEN_outline_nor_sobel_Included_
#include regen.filter.outlines.outline_nor.defs

const mat3 in_sobelOutline_y = mat3(
	vec3(1.0, 0.0, -1.0),
	vec3(2.0, 0.0, -2.0),
	vec3(1.0, 0.0, -1.0));
const mat3 in_sobelOutline_x = mat3(
	vec3(1.0,   2.0,  1.0),
	vec3(0.0,   0.0,  0.0),
	vec3(-1.0, -2.0, -1.0));

float outline_nor(vec2 uv, vec2 offset0) {
    vec2 offset = in_norOutlineScale*offset0;
    vec3 normal = norSample(uv);
    // sample 8 neighboring pixels
    vec3 nor_nw = norSample(uv - offset);
    vec3 nor_ne = norSample(uv + vec2( offset.x, -offset.y));
    vec3 nor_sw = norSample(uv + vec2(-offset.x,  offset.y));
    vec3 nor_se = norSample(uv + offset);
    vec3 nor_n = norSample(uv - vec2(0.0, offset.y));
    vec3 nor_e = norSample(uv + vec2(offset.x, 0.0));
    vec3 nor_s = norSample(uv + vec2(0.0, offset.y));
    vec3 nor_w = norSample(uv - vec2(offset.x, 0.0));

    mat3 surrounding_pixels = mat3(
		vec3(length(nor_nw-normal), length(nor_n-normal),  length(nor_ne-normal)),
		vec3(length(nor_w-normal),  length(normal-normal), length(nor_e-normal)),
		vec3(length(nor_sw-normal), length(nor_s-normal),  length(nor_se-normal))
	);
	float edge_x =
        dot(in_sobelOutline_x[0], surrounding_pixels[0]) +
        dot(in_sobelOutline_x[1], surrounding_pixels[1]) +
        dot(in_sobelOutline_x[2], surrounding_pixels[2]);
	float edge_y =
        dot(in_sobelOutline_y[0], surrounding_pixels[0]) +
        dot(in_sobelOutline_y[1], surrounding_pixels[1]) +
        dot(in_sobelOutline_y[2], surrounding_pixels[2]);
	float edge = sqrt(pow(edge_x, 2.0)+pow(edge_y, 2.0));

	return float(edge > in_norOutlineThreshold);
}
#endif

-- outline_col.simple
#ifndef REGEN_outline_col_simple_Included_
#define2 REGEN_outline_col_simple_Included_
uniform sampler2D in_gColorTexture;

float intensity(in vec4 color) {
    return sqrt((color.x*color.x)+(color.y*color.y)+(color.z*color.z));
}

float outline_col(vec2 texco, vec2 step) {
}
