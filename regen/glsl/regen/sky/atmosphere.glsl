
--------------------------------
--------------------------------
----- Computes atmosphere cube map.
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
        gl_Position = gl_in[0].gl_Position;
        EmitVertex();
        out_pos = getCubePoint(in_pos[1],i);
        gl_Position = gl_in[1].gl_Position;
        EmitVertex();
        out_pos = getCubePoint(in_pos[2],i);
        gl_Position = gl_in[2].gl_Position;
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

#include regen.sky.utility.computeHorizonExtinction
#include regen.sky.utility.computeEyeExtinction
#include regen.sky.utility.computeAtmosphericDepth
#include regen.sky.utility.computeEyeDepth
#include regen.sky.utility.phase
#include regen.sky.utility.absorb

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
