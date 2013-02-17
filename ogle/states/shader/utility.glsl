
-- texcoToWorldSpace
vec3 texcoToWorldSpace(vec2 texco, float depth) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = in_inverseViewProjectionMatrix*vec4(pos0,1.0);
    return D.xyz/D.w;
}

-- worldSpaceToTexco
vec3 worldSpaceToTexco(vec4 ws)
{
    //vec4 lightScreen = in_viewProjectionMatrix*ws; // XXX
    vec4 ss = (in_projectionMatrix * in_viewMatrix)*ws;
    return (ss.xyz/ss.w + vec3(1.0))*0.5;
}

-- linearizeDepth
float linearizeDepth(float d) {
    float z_n = 2.0*d - 1.0;
    return 2.0*in_near*in_far/(in_far+in_near-z_n*(in_far-in_near));
}

-- pointVectorDistance
float pointVectorDistance(vec3 dir, vec3 p)
{
    return dot(p,dir)/dot(dir,dir);
}

