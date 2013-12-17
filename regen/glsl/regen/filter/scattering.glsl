
--------------------------------
--------------------------------
----- Computes a sky scattering cube map.
----- Code based on: http://codeflow.org/entries/2011/apr/13/advanced-webgl-part-2-sky-rendering/
--------------------------------
--------------------------------
-- vs
in vec3 in_pos;
out vec2 out_pos;

void main(void) {
    out_pos = vec2(in_pos.x, -in_pos.y);
    gl_Position = vec4(in_pos.xy, 0.0, 1.0);
}

-- gs
#extension GL_EXT_geometry_shader4 : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=15) out;

out vec3 out_pos;
in vec2 in_pos[3];

vec3 getCubePoint(vec2 p, int i)
{
    vec3 cubePoints[5] = vec3[](
        vec3( 1.0, p.y,-p.x), // +X
        vec3(-1.0, p.y, p.x), // -X
        vec3( p.x, 1.0,-p.y), // +Y
        vec3( p.x, p.y, 1.0), // +Z
        vec3(-p.x, p.y,-1.0)  // -Z
    );
    return cubePoints[i];
}

void main(void) {
    for(int i=0; i<5; ++i) {
        // select framebuffer layer
        gl_Layer = i + int(i>2);
        
        out_pos = getCubePoint(in_pos[0],i);
        gl_Position = gl_PositionIn[0];
        EmitVertex();
        out_pos = getCubePoint(in_pos[1],i);
        gl_Position = gl_PositionIn[1];
        EmitVertex();
        out_pos = getCubePoint(in_pos[2],i);
        gl_Position = gl_PositionIn[2];
        EmitVertex();
        EndPrimitive();
    }
}

-- fs
out vec4 out_color;
in vec3 in_pos;

uniform vec3 in_sunDir;
// brightness, strength, collection power
uniform vec3 in_rayleigh;
// brightness, strength, collection power, distribution
uniform vec4 in_mie;
uniform float in_spotBrightness;
uniform float in_scatterStrength;

const int stepCount = 16;

uniform vec3 in_skyAbsorbtion;
const float surfaceHeight = 0.99;
const float intensity = 1.8;

#include regen.filter.scattering.computeHorizonExtinction
#include regen.filter.scattering.computeEyeExtinction
#include regen.filter.scattering.computeAtmosphericDepth
#include regen.filter.scattering.computeEyeDepth
#include regen.filter.scattering.phase
#include regen.filter.scattering.absorb

void main(void)
{
    vec3 eyedir = normalize(in_pos);
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    float eyeExtinction = computeEyeExtinction(eyedir);
    float eyeDepth = computeEyeDepth(eyedir);

    float stepLength = eyeDepth/float(stepCount);

    vec3 rayleighCollected = vec3(0.0, 0.0, 0.0);
    vec3 mieCollected = vec3(0.0, 0.0, 0.0);

    for(int i=0; i<stepCount; i++)
    {
        float sampleDistance = stepLength*float(i);
        vec3 position = eyePosition + eyedir*sampleDistance;
        float sampleExtinction = computeHorizonExtinction(position, in_sunDir, surfaceHeight-0.35);
        float sampleDepth = computeAtmosphericDepth(position, in_sunDir);
        
        vec3 influx = absorb(sampleDepth, vec3(intensity), in_scatterStrength)*sampleExtinction;
        rayleighCollected += absorb(sampleDistance, in_skyAbsorbtion*influx, in_rayleigh.y);
        mieCollected += absorb(sampleDistance, influx, in_mie.y);
    }

    float collectedFactor = eyeExtinction/float(stepCount);
    rayleighCollected *= pow(eyeDepth, in_rayleigh.z)*collectedFactor;
    mieCollected      *= pow(eyeDepth, in_mie.z)*collectedFactor;

    float alpha = dot(eyedir, in_sunDir);
    out_color.rgb  = smoothstep(0.0, 15.0, phase(alpha, 0.9995))*in_spotBrightness* mieCollected;
    out_color.rgb += phase(alpha, in_mie.w)*in_mie.x* mieCollected;
    out_color.rgb += phase(alpha, -0.01)*in_rayleigh.x * rayleighCollected;

    out_color.a = smoothstep(0.0,0.05,length(out_color.rgb));
}

-----------------------------------
-----------------------------------
-------------- Utility functions

-- computeHorizonExtinction
#ifndef __computeHorizonExtinction_vec3_vec3_float_INCLUDED
#define2 __computeHorizonExtinction_vec3_vec3_float_INCLUDED
float computeHorizonExtinction(vec3 position, vec3 dir, float radius)
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
#endif

-- computeEyeExtinction
#ifndef __computeEyeExtinction_vec3_INCLUDED
#define2 __computeEyeExtinction_vec3_INCLUDED
#include regen.filter.scattering.computeHorizonExtinction
float computeEyeExtinction(vec3 eyedir)
{
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    return computeHorizonExtinction(eyePosition, eyedir, surfaceHeight-0.15);
}
#endif

-- computeAtmosphericDepth
#ifndef __computeAtmosphericDepth_vec3_vec3__INCLUDED
#define2 __computeAtmosphericDepth_vec3_vec3__INCLUDED
float computeAtmosphericDepth(vec3 position, vec3 dir)
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
#endif

-- computeEyeDepth
#ifndef __computeEyeDepth_vec3__INCLUDED
#define2 __computeEyeDepth_vec3__INCLUDED
#include regen.filter.scattering.computeAtmosphericDepth
float computeEyeDepth(vec3 eyedir)
{
    vec3 eyePosition = vec3(0.0, surfaceHeight, 0.0);
    return computeAtmosphericDepth(eyePosition, eyedir);
}
#endif

-- phase
#ifndef __phase_float_float__INCLUDED
#define2 __phase_float_float__INCLUDED
float phase(float alpha, float g)
{
    float a = 3.0*(1.0-g*g);
    float b = 2.0*(2.0+g*g);
    float c = 1.0+alpha*alpha;
    float d = pow(1.0+g*g-2.0*g*alpha, 1.5);
    return (a/b)*(c/d);
}
#endif

-- absorb
#ifndef __absorb_float_vec3_float__INCLUDED
#define2 __absorb_float_vec3_float__INCLUDED
vec3 absorb(float dist, vec3 color, float factor)
{
    return color-color*pow(in_skyAbsorbtion, vec3(factor/dist));
}
#endif
