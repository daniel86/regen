
-- texcoToWorldSpace
vec3 texcoToWorldSpace(vec2 texco, float depth) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = in_inverseViewProjectionMatrix*vec4(pos0,1.0);
    return D.xyz/D.w;
}

-- worldSpaceToTexco
vec3 worldSpaceToTexco(vec4 ws)
{
    //vec4 ss = in_viewProjectionMatrix*ws; // XXX
    vec4 ss = (in_projectionMatrix * in_viewMatrix)*ws;
    return (ss.xyz/ss.w + vec3(1.0))*0.5;
}

-- linearizeDepth
float linearizeDepth(float d) {
    float z_n = 2.0*d - 1.0;
    return 2.0*in_near*in_far/(in_far+in_near-z_n*(in_far-in_near));
}

-- rayVectorDistance
float rayVectorDistance(vec3 p, vec3 l, vec3 d)
{
float pd = dot(p,d);
float pp = dot(p,p);
float dd = dot(d,d);
float ld = dot(l,d);
float lp = dot(l,p);

    return ( ld*pd - lp*dd ) / ( pd*pd - pp*dd );
}

-- pointVectorDistance
float pointVectorDistance(vec3 dir, vec3 p)
{
    return dot(p,dir)/dot(dir,dir);
}

