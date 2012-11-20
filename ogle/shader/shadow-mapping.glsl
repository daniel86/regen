
-- sampling

int getShadowFace()
{
    // TODO: ...
    return 0;
}
int getShadowLayer(float shadowFar[SM_NUM_SLICES])
{
    for(int i=0; i<SM_NUM_SLICES; ++i) {
        if(gl_FragCoord.z < shadowFar[i]) { return i; }
    }
    return 0;
}

float sampleSpotShadow(
        vec3 posWorld,
        sampler2DShadow tex,
        mat4 shadowMatrix
        )
{
	float shadow=0.0;
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = shadowMatrix*vec4(posWorld,1.0);

    shadow = texture(tex, shadowCoord.xyz);
    
    return shadow;
}

float sampleDirectionalShadow(
        vec3 posWorld,
        sampler2DArrayShadow tex,
        float shadowFar[SM_NUM_SLICES],
        mat4 shadowMatrices[SM_NUM_SLICES]
        )
{
	float shadow;
    // shadow map selection is done by distance of pixel to the camera.
    int shadowMapIndex = getShadowLayer(shadowFar);
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = shadowMatrices[shadowMapIndex]*vec4(posWorld,1.0);
    shadowCoord.w = shadowCoord.z;
    // tell glsl in which layer to do the look up
    shadowCoord.z = float(shadowMapIndex);

#ifdef PCF_4TAB
	// Bilinear weighted 4-tap filter
	vec2 pos = mod( shadowCoord.xy * texSize.x, 1.0);
	vec2 offset = (0.5 - step( 0.5, pos)) * texSize.y;
	shadow  = shadow2DArray(tex, shadowCoord + vec4( offset, 0, 0)).x * (pos.x) * (pos.y);
	shadow += shadow2DArray(tex, shadowCoord + vec4( offset.x, -offset.y, 0, 0)).x * (pos.x) * (1-pos.y);
	shadow += shadow2DArray(tex, shadowCoord + vec4(-offset.x,  offset.y, 0, 0)).x * (1-pos.x) * (pos.y);
	shadow += shadow2DArray(tex, shadowCoord + vec4(-offset.x, -offset.y, 0, 0)).x * (1-pos.x) * (1-pos.y);
#elif PCF_GAUSSIAN
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
#else
	// return the shadow contribution
	shadow = shadow2DArray(tex, shadowCoord).x;
#endif

    return shadow;
}

float samplePointShadow(
        vec3 posWorld,
        samplerCubeShadow tex,
        mat4 shadowMatrices[6]
        )
{
	float shadow=0.0;
    int face = getShadowFace();
    // transform this fragment's position from world space to scaled light clip space
    // such that the xy coordinates are in [0;1]
    vec4 shadowCoord = shadowMatrices[face]*vec4(posWorld,1.0);
    shadowCoord.w = shadowCoord.z;
    // tell glsl in which layer to do the look up
    shadowCoord.z = float(face);

	// return the shadow contribution
	shadow = shadowCube(tex, shadowCoord).x;

    return shadow;
}

