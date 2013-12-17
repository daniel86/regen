
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
            normalize( reflect( L, -N ) ),
            normalize( P - in_cameraPosition ) ), 0.0);
}
