
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
//float linearizeDepth(float d, float n, float f)
//{
//    float z_n = 2.0*d - 1.0;
//    return 2.0*n*f/(f+n-z_n*(f-n));
//}
float linearizeDepth(float expDepth, float n, float f)
{
    return (2.0*n)/(f+n - expDepth*(f-n));
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

-- computeCubeMapLayer
#ifndef __computeCubeMapLayer__included__
#define2 __computeCubeMapLayer__included__
int computeCubeMapLayer(vec3 normalizedDirection)
{
    vec3 absDir = abs(normalizedDirection);
    float magnitude = max(absDir.x, max(absDir.y, absDir.z));
    return
        (1-int(absDir.x<magnitude))*int(1.0 - (sign(normalizedDirection.x)+1.0)/2.0) +
        (1-int(absDir.y<magnitude))*int(3.0 - (sign(normalizedDirection.y)+1.0)/2.0) +
        (1-int(absDir.z<magnitude))*int(5.0 - (sign(normalizedDirection.z)+1.0)/2.0);
}
#endif

-- computeCubeMapDirection
#ifndef __computeCubeMapDirection__included__
#define2 __computeCubeMapDirection__included__
vec3 computeCubeMapDirection(vec2 uv, int layer)
{
    vec3 cubePoints[6] = vec3[](
        vec3(  1.0, uv.y,-uv.x), // +X
        vec3( -1.0, uv.y, uv.x), // -X
        vec3( uv.x,  1.0,-uv.y), // +Y
        vec3( uv.x, -1.0, uv.y), // -Y
        vec3( uv.x, uv.y,  1.0), // +Z
        vec3(-uv.x, uv.y, -1.0)  // -Z
    );
    return cubePoints[layer];
}
#endif

