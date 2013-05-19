
-- all
uniform vec3 in_skyAbsorbtion;
const float surfaceHeight = 0.99;
const float intensity = 1.8;

float getHorizonExtinction(vec3 position, vec3 dir, float radius)
{
    float u = dot(dir, -position);
    if(u<0.0){
        return 1.0;
    }
    vec3 near = position + u*dir;
    if(length(near) < radius){
        return 0.0;
    }
    else{
        vec3 v2 = normalize(near)*radius - position;
        float diff = acos(dot(normalize(v2), dir));
        return smoothstep(0.0, 1.0, pow(diff*2.0, 3.0));
    }
}
float getEyeExtinction(vec3 eyedir)
{
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    return getHorizonExtinction(eyePosition, eyedir, surfaceHeight-0.15);
}

float getAtmosphericDepth(vec3 position, vec3 dir)
{
    float a = dot(dir, dir);
    float b = 2.0*dot(dir, position);
    float c = dot(position, position)-1.0;
    float det = b*b-4.0*a*c;
    float detSqrt = sqrt(det);
    float q = (-b - detSqrt)/2.0;
    float t1 = c/q;
    return t1;
}
float getEyeDepth(vec3 eyedir)
{
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    return getAtmosphericDepth(eyePosition, eyedir);
}

float phase(float alpha, float g)
{
    float a = 3.0*(1.0-g*g);
    float b = 2.0*(2.0+g*g);
    float c = 1.0+alpha*alpha;
    float d = pow(1.0+g*g-2.0*g*alpha, 1.5);
    return (a/b)*(c/d);
}

vec3 absorb(float dist, vec3 color, float factor)
{
    return color-color*pow(in_skyAbsorbtion, vec3(factor/dist));
}
