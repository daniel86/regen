
-- debug.vs
#version 150

in vec3 in_pos;
out vec2 out_texco;
void main() {
    out_texco = (vec4(0.5)*vec4(in_pos,1.0) + vec4(0.5)).xy;
    gl_Position = vec4(in_pos,1.0);
}

-- debugDirectional.fs
#version 150
#extension GL_EXT_gpu_shader4 : enable

in vec2 in_texco;
out vec4 output;

uniform float in_shadowLayer;
uniform sampler2DArray in_shadowMap;

void main() {
    output = texture2DArray(in_shadowMap, vec3(in_texco, in_shadowLayer));
}

-- debugPoint.fs
#version 150
#extension GL_EXT_gpu_shader4 : enable

in vec2 in_texco;
out vec4 output;

uniform float in_shadowLayer;
uniform samplerCube in_shadowMap;

void main() {
    if(in_shadowLayer<1.0) {
        output = texture(in_shadowMap, vec3(1.0, in_texco.x, in_texco.y));
    } else if(in_shadowLayer<2.0) {
        output = texture(in_shadowMap, vec3(-1.0, in_texco.x, in_texco.y));
    } else if(in_shadowLayer<3.0) {
        output = texture(in_shadowMap, vec3(in_texco.y, 1.0, in_texco.x));
    } else if(in_shadowLayer<4.0) {
        output = texture(in_shadowMap, vec3(in_texco.y, -1.0, in_texco.x));
    } else if(in_shadowLayer<5.0) {
        output = texture(in_shadowMap, vec3(in_texco.x, in_texco.y, 1.0));
    } else {
        output = texture(in_shadowMap, vec3(in_texco.x, in_texco.y, -1.0));
    }
}

-- debugSpot.fs
#version 150
#extension GL_EXT_gpu_shader4 : enable

in vec2 in_texco;
out vec4 output;

uniform sampler2D in_shadowMap;

void main() {
    output = texture2D(in_shadowMap, in_texco);
}

-- sampling

//#define PCF_4TAB
#define PCF_GAUSSIAN
//#define SINGLE_LOOKUP

#ifdef NUM_SHADOW_MAP_SLICES
int getShadowLayer(float depth, float shadowFar[NUM_SHADOW_MAP_SLICES])
{
    for(int i=0; i<NUM_SHADOW_MAP_SLICES; ++i) {
        if(depth < shadowFar[i]) { return i; }
    }
    return 0;
}
float sampleDirectionalShadow(
        vec3 posWorld,
        float depth,
        sampler2DArrayShadow tex,
        float shadowFar[NUM_SHADOW_MAP_SLICES],
        mat4 shadowMatrices[NUM_SHADOW_MAP_SLICES]
        )
{
	float shadow;
    // shadow map selection is done by distance of pixel to the camera.
    int shadowMapIndex = getShadowLayer(depth, shadowFar);
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = shadowMatrices[shadowMapIndex]*vec4(posWorld,1.0);
    shadowCoord.w = shadowCoord.z;
    // tell glsl in which layer to do the look up
    shadowCoord.z = float(shadowMapIndex);

#ifdef PCF_4TAB
	// Bilinear weighted 4-tap filter
vec2 texSize = vec2(2048.0, 1.0/2048.0);
	vec2 pos = mod( shadowCoord.xy * texSize.x, 1.0);
	vec2 offset = (0.5 - step( 0.5, pos)) * texSize.y;
	shadow  = shadow2DArray(tex, shadowCoord + vec4( offset, 0, 0)).x * (pos.x) * (pos.y);
	shadow += shadow2DArray(tex, shadowCoord + vec4( offset.x, -offset.y, 0, 0)).x * (pos.x) * (1-pos.y);
	shadow += shadow2DArray(tex, shadowCoord + vec4(-offset.x,  offset.y, 0, 0)).x * (1-pos.x) * (pos.y);
	shadow += shadow2DArray(tex, shadowCoord + vec4(-offset.x, -offset.y, 0, 0)).x * (1-pos.x) * (1-pos.y);
#endif
#ifdef PCF_GAUSSIAN
	// Gaussian 3x3 filter
	shadow  = shadow2DArray(tex, shadowCoord).x * 0.25;
	shadow += shadow2DArrayOffset(tex, shadowCoord, ivec2( -1, -1)).x * 0.0625;
	shadow += shadow2DArrayOffset(tex, shadowCoord, ivec2( -1,  0)).x * 0.125;
	shadow += shadow2DArrayOffset(tex, shadowCoord, ivec2( -1,  1)).x * 0.0625;
	shadow += shadow2DArrayOffset(tex, shadowCoord, ivec2(  0, -1)).x * 0.125;
	shadow += shadow2DArrayOffset(tex, shadowCoord, ivec2(  0,  1)).x * 0.125;
	shadow += shadow2DArrayOffset(tex, shadowCoord, ivec2(  1, -1)).x * 0.0625;
	shadow += shadow2DArrayOffset(tex, shadowCoord, ivec2(  1,  0)).x * 0.125;
	shadow += shadow2DArrayOffset(tex, shadowCoord, ivec2(  1,  1)).x * 0.0625;
#endif
#ifdef SINGLE_LOOKUP
	// return the shadow contribution
	shadow = shadow2DArray(tex, shadowCoord).x;
#endif

    return shadow;
}
#endif

float sampleSpotShadow(
        vec3 posWorld,
        sampler2DShadow tex,
        mat4 shadowMatrix
        )
{
	float shadow;
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = shadowMatrix*vec4(posWorld,1.0);
    
    shadow = textureProj(tex, shadowCoord);

    return shadow;
}

int getShadowFace(vec3 texco)
{
    vec3 s = abs(texco);
    if( all( greaterThanEqual( s.xx, s.yz ) ) ) {
        return int(0.5*(1.0 - sign(texco.x)));
    }
    else if( all( greaterThanEqual( s.yy, s.xz ) ) ) {
        return int(0.5*(5.0 - sign(texco.y)));
    }
    else {
        return int(0.5*(9.0 - sign(texco.z)));
    }
}
float samplePointShadow(
        vec3 posWorld,
        vec3 lightVec,
        float depth,
        samplerCubeShadow tex,
        mat4 shadowMatrices[6]
        )
{
    vec3 texco = -lightVec;
	float shadow=0.0;
    int face = getShadowFace(texco);
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = shadowMatrices[face]*vec4(posWorld,1.0);
    float depthLS = shadowCoord.z/shadowCoord.w;
    // tell glsl in which layer to do the look up
    //shadowCoord.z = float(face);

	// return the shadow contribution
	//shadow = shadowCube(tex, shadowCoord).x;
	//shadow = shadowCube(tex, shadowCoord).x;
    shadow = shadowCube(tex, vec4(texco,depthLS)).x;

    return shadow;
}

