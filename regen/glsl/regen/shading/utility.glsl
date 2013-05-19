
-- fetchNormal
vec3 fetchNormal(vec2 texco) {
    vec4 N = texture(in_gNorWorldTexture, texco);
    if( N.w==0.0 ) discard;
    return N.xyz*2.0 - vec3(1.0);
}

-- fetchPosition
#include regen.utility.utility.texcoToWorldSpace

vec3 fetchPosition(vec2 texco) {
    float depth = texture(in_gDepthTexture, in_texco).r;
    return texcoToWorldSpace(in_texco, depth);
}

-- radiusAttenuation
float radiusAttenuation(float d, float innerRadius, float outerRadius) {
    return 1.0 - smoothstep(innerRadius, outerRadius, d);
}

-- spotConeAttenuation
float spotConeAttenuation(vec3 L, vec3 dir, vec2 coneAngles) {
    float spotEffect = dot( -L, normalize(dir) );
    float spotFade = 1.0 - (spotEffect-coneAngles.x)/(coneAngles.y-coneAngles.x);
    return max(0.0, sign(spotEffect-coneAngles.y))*clamp(spotFade, 0.0, 1.0);
}

-- specularFactor
float specularFactor(vec3 P, vec3 L, vec3 N) {
    return max( dot(
            normalize( reflect( -L, N ) ),
            normalize( P - in_cameraPosition ) ), 0.0);
}
