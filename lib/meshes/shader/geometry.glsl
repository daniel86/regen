
-- screen-space
bool isPointBetween(vec2 x, vec2 start, vec2 p0, vec2 p1)
{
    // point to start
    vec2 a = x-start;
    // start to first edge point
    vec2 b = start-p0;
    // second edge to first edge
    vec2 c = p1-p0;
    // find vector lengths
    float lambda = (b.y*a.x - b.x*a.y) / (c.y*a.x - c.x*a.y);
    float psi = (lambda*c.x - b.x)/a.x;
    return 0.0<=lambda && lambda<=1.0 && psi>=0.0;
}

-- distance.point-line
float pointLineDistance(vec3 p, vec3 pl, vec3 dirl)
{
    return length( cross(dirl, p-pl) ) / length(dirl);
}
float pointLineDistance(vec2 p, vec2 pl, vec2 dirl)
{
    return pointLineDistance(vec3(p,0.0),vec3(pl,0.0),vec3(dirl,0.0));
}

-- cone
bool isPointInCone(vec3 x, vec3 apex, vec3 direction, float halfConeAngleCos)
{
    vec3 pointDirection = normalize(apex-x);
    float pointAngleCos = dot(pointDirection,-direction);
    return pointAngleCos > halfConeAngleCos;
}


