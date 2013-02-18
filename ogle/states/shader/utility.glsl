
-- texcoToWorldSpace
#ifndef __texcoToWorldSpace_included__
#define __texcoToWorldSpace_included__
vec3 texcoToWorldSpace(vec2 texco, float depth) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = in_inverseViewProjectionMatrix*vec4(pos0,1.0);
    return D.xyz/D.w;
}
#endif

-- worldSpaceToTexco
#ifndef __worldSpaceToTexco_included__
#define __worldSpaceToTexco_included__
vec3 worldSpaceToTexco(vec4 ws)
{
    //vec4 ss = in_viewProjectionMatrix*ws; // XXX
    vec4 ss = (in_projectionMatrix * in_viewMatrix)*ws;
    return (ss.xyz/ss.w + vec3(1.0))*0.5;
}
#endif

-- linearizeDepth
#ifndef __linearizeDepth_included__
#define __linearizeDepth_included__
float linearizeDepth(float d) {
    float z_n = 2.0*d - 1.0;
    return 2.0*in_near*in_far/(in_far+in_near-z_n*(in_far-in_near));
}
#endif

-- rayVectorDistance
#ifndef __rayVectorDistance_included__
#define __rayVectorDistance_included__
float rayVectorDistance(vec3 p, vec3 l, vec3 d)
{
float pd = dot(p,d);
float pp = dot(p,p);
float dd = dot(d,d);
float ld = dot(l,d);
float lp = dot(l,p);

    return ( ld*pd - lp*dd ) / ( pd*pd - pp*dd );
}
#endif

-- pointVectorDistance
#ifndef __pointVectorDistance_included__
#define __pointVectorDistance_included__
float pointVectorDistance(vec3 dir, vec3 p)
{
    return dot(p,dir)/dot(dir,dir);
}
#endif

