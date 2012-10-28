-- texco_cube
vec3 texco_cube()
{
    return reflect(-gl_FragCoord, in_norWorld);
}

-- texco_sphere
vec2 texco_sphere()
{
   vec3 r = reflect(normalize(gl_FragCoord), in_norWorld);
   float m = 2.0 * sqrt( r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0) );
   return vec2(r.x/m + 0.5, r.y/m + 0.5);
}

-- texco_tube
vec2 texco_tube()
{
    float PI = 3.14159265358979323846264;
    vec3 r = reflect(normalize(gl_FragCoord), in_norWorld);
    float u,v;
    float len = sqrt(r.x*r.x + r.y*r.y);
    v = (r.z + 1.0f) / 2.0f;
    if(len > 0.0f) u = ((1.0 - (2.0*atan(r.x/len,r.y/len) / PI)) / 2.0);
    else u = 0.0f;
    return vec2(u,v);
}

-- texco_flat
vec2 texco_flat()
{
    vec3 r = reflect(normalize(gl_FragCoord), in_norWorld);
    return vec2( (r.x + 1.0)/2.0, (r.y + 1.0)/2.0);
}

-- texco_refraction
vec3 texco_refraction()
{
    vec3 incident = normalize(in_posWorld - in_cameraPosition.xyz );
    return refract(incident, in_norWorld, in_matRefractionIndex);
}

-- texco_reflection
vec3 texco_reflection()
{
    vec3 incident = normalize(in_posWorld - in_cameraPosition.xyz );
    return reflect(incident.xyz, in_norWorld);
}

